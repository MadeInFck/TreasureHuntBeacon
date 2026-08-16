// =====================================================================
//  Treasure Hunt - Differential VBAT probe (ESP32-C3)
//  https://github.com/MadeInFck/TreasureHuntBeacon
//
//  Standalone diagnostic sketch. Does NOT advertise. Its only job is to
//  answer one question: does this board wire the battery to an ADC pin?
//
//  Single readings prove nothing here -- every unconnected pin floats
//  somewhere in the 400-600 mV range, and both a fixed and a
//  proportional stability threshold gave misleading verdicts. What does
//  prove something is a DIFFERENCE: a pin fed by a VBAT divider moves by
//  hundreds of millivolts when the cell is plugged in. A floating pin
//  stays inside its own noise.
//
//  ---------------------------------------------------------------------
//  HOW TO USE
//
//   1. Flash with the cell DISCONNECTED. Keep USB plugged in.
//   2. Open the serial monitor at 115200. Set the line ending to
//      "Newline" or "Both NL & CR" so the sketch sees your Enter.
//   3. Phase A samples for ~30 s on its own.
//   4. When prompted, plug the cell into the PH-2.0 port and press
//      Enter in the serial monitor.
//   5. Phase B samples for another ~30 s, then prints the verdict.
//
//  Only GPIO0..GPIO4 are scanned. Those are the ESP32-C3's ADC1
//  channels. GPIO5 is on ADC2, which is unreliable while the radio is
//  active, and every other pin returns meaningless values.
// =====================================================================

#define PHASE_SECONDS   30    // sampling window per phase
#define SAMPLE_EVERY_MS 500   // one sweep per pin at this cadence
#define BAT_NOMINAL_MV  4000  // measured cell voltage, for the expected level

#define DELTA_SIGNIFICANT 250 // mV of movement that cannot be noise
#define NOISE_BAND_MV      80 // expected spread of a floating pin

// ESP32-C3 ADC1 channels. Do not extend this list.
static const uint8_t PINS[] = {0, 1, 2, 3, 4};
static const uint8_t NPINS  = sizeof(PINS);

struct Stats {
  uint32_t sum;
  uint16_t n, lo, hi;
};

static Stats phaseA[5], phaseB[5];

// ---------------------------------------------------------------------
static void resetStats(Stats *s) {
  for (uint8_t i = 0; i < NPINS; i++) { s[i].sum = 0; s[i].n = 0; s[i].lo = 0xFFFF; s[i].hi = 0; }
}

static uint16_t medianOf9(uint8_t pin) {
  uint32_t v[9];
  for (uint8_t i = 0; i < 9; i++) { v[i] = analogReadMilliVolts(pin); delay(2); }
  for (uint8_t i = 1; i < 9; i++) {           // insertion sort
    uint32_t t = v[i]; int j = i - 1;
    while (j >= 0 && v[j] > t) { v[j + 1] = v[j]; j--; }
    v[j + 1] = t;
  }
  return (uint16_t)v[4];
}

static void sampleInto(Stats *s) {
  for (uint8_t i = 0; i < NPINS; i++) {
    uint16_t mv = medianOf9(PINS[i]);
    s[i].sum += mv;
    s[i].n++;
    if (mv < s[i].lo) s[i].lo = mv;
    if (mv > s[i].hi) s[i].hi = mv;
  }
}

static uint16_t meanOf(const Stats &s) {
  return s.n ? (uint16_t)(s.sum / s.n) : 0;
}

static void runPhase(const char *label, Stats *dest) {
  resetStats(dest);
  Serial.printf("\n--- Phase %s: sampling %u s ---\n", label, PHASE_SECONDS);

  uint32_t end = millis() + (uint32_t)PHASE_SECONDS * 1000UL;
  uint8_t  dots = 0;
  while (millis() < end) {
    sampleInto(dest);
    if (++dots % 4 == 0) { Serial.print('.'); }
    delay(SAMPLE_EVERY_MS);
  }
  Serial.println();

  for (uint8_t i = 0; i < NPINS; i++) {
    Serial.printf("  GPIO%u  mean %4u mV   range %4u-%4u mV\n",
                  PINS[i], meanOf(dest[i]), dest[i].lo, dest[i].hi);
  }
}

static void waitForEnter(const char *prompt) {
  Serial.printf("\n>>> %s\n>>> Then press Enter here.\n", prompt);
  while (Serial.available()) Serial.read();          // flush anything stale
  for (;;) {
    if (Serial.available()) {
      int c = Serial.read();
      if (c == '\n' || c == '\r') break;
    }
    delay(20);
  }
  while (Serial.available()) Serial.read();          // eat the rest of the line
}

// ---------------------------------------------------------------------
static void verdict() {
  uint16_t expected = BAT_NOMINAL_MV / 2;            // a 2:1 divider

  Serial.println("\n=====================================================");
  Serial.println(" GPIO   no cell   with cell    delta   verdict");
  Serial.println("-----------------------------------------------------");

  int8_t found = -1;

  for (uint8_t i = 0; i < NPINS; i++) {
    uint16_t a = meanOf(phaseA[i]);
    uint16_t b = meanOf(phaseB[i]);
    int16_t  d = (int16_t)b - (int16_t)a;

    const char *v;
    if (abs(d) < NOISE_BAND_MV)                 v = "unchanged -> not VBAT";
    else if (abs(d) < DELTA_SIGNIFICANT)        v = "small shift -> doubtful";
    else if (b > 3000)                          v = "!! TOO HIGH, UNPLUG !!";
    else                                        { v = "MOVED -> VBAT candidate"; found = i; }

    Serial.printf(" %4u   %5u mV   %5u mV   %+5d mV   %s\n",
                  PINS[i], a, b, d, v);
  }

  Serial.println("-----------------------------------------------------");
  Serial.printf(" A 2:1 divider on a %u mV cell reads ~%u mV.\n",
                BAT_NOMINAL_MV, expected);

  if (found >= 0) {
    uint16_t b = meanOf(phaseB[found]);
    Serial.printf("\n GPIO%u is your VBAT pin.\n", PINS[found]);
    Serial.printf(" Implied divider ratio: %u.%02u : 1\n",
                  (BAT_NOMINAL_MV * 100 / b) / 100,
                  (BAT_NOMINAL_MV * 100 / b) % 100);
    Serial.println(" Set BAT_ADC_PIN and DIVIDER_X100 in beacon.ino,");
    Serial.println(" then confirm against a multimeter on the cell.");
  } else {
    Serial.println("\n No pin moved when the cell was connected.");
    Serial.println(" This board has NO on-board VBAT divider. Wire one:");
    Serial.println();
    Serial.println("   switched battery + -- R1 100k --+-- R2 100k -- GND");
    Serial.println("                                   |");
    Serial.println("                                   +-- C1 100nF - GND");
    Serial.println("                                   |");
    Serial.println("                                   +-- GPIO3 or GPIO4");
    Serial.println();
    Serial.println(" Tap R1 AFTER the switch, or it drains the cell.");
    Serial.println(" Keep C1 physically against the board.");
  }
  Serial.println("=====================================================");
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=== Differential VBAT probe - ESP32-C3 ===");
  Serial.printf("Scanning GPIO0..GPIO4 (ADC1). Cell assumed at %u mV.\n",
                BAT_NOMINAL_MV);
  Serial.println("Serial monitor line ending must be Newline or NL & CR.");

  for (uint8_t i = 0; i < NPINS; i++)
    analogSetPinAttenuation(PINS[i], ADC_11db);      // ~0 to 3.1 V range

  waitForEnter("Make sure the cell is DISCONNECTED (USB only).");
  runPhase("A  (no cell)", phaseA);

  waitForEnter("Now PLUG THE CELL into the PH-2.0 port.");
  runPhase("B  (cell connected)", phaseB);

  verdict();
}

void loop() {
  delay(1000);
}
