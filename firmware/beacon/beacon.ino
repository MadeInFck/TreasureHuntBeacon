// =====================================================================
//  Treasure Hunt - Beacon firmware
//  https://github.com/MadeInFck/TreasureHuntBeacon
//
//  Single source for every supported board. Set TARGET_BOARD below,
//  then HUNT_ID and STEP_ID, then calibrate TX_POWER_1M.
//
//  ONE SKETCH PER FOLDER: the Arduino IDE concatenates every .ino in a
//  sketch folder before compiling. Keep only this file in firmware/beacon/,
//  or you get "redefinition of setup()" and friends.
//
//  ---------------------------------------------------------------------
//  Hard-won notes from bringing this up. Do not undo these.
//
//  * The manufacturer payload is built byte by byte, not via BLEBeacon.
//    That class applies ENDIAN_CHANGE_U16 internally and which fields it
//    touches has varied across core versions. Hand-built bytes are also
//    immune to the Bluedroid/NimBLE split.
//
//  * No setAdvertisementType(ADV_TYPE_NONCONN_IND). That constant is
//    Bluedroid-only and does not compile on NimBLE targets such as the
//    ESP32-C6. Apple's spec wants non-connectable, but CoreLocation
//    ranges connectable beacons identically.
//
//  * BLEDevice::init("") with an EMPTY name. Flags plus the manufacturer
//    structure already use 30 of the 31 available bytes; any name
//    overflows and the beacon silently disappears.
//
//  * NO %f ANYWHERE in Serial.printf. Arduino-ESP32 builds RISC-V targets
//    against newlib-nano, which does not implement floating point
//    formatting. A single %f can swallow the whole line or print garbage.
//    All ratios below are printed with integer maths.
//
//  * nRF Connect on iPhone will NEVER show this beacon. iOS filters
//    Apple manufacturer data out of CoreBluetooth. Verify with a Mac,
//    an Android phone, firmware/scanner, or your own CoreLocation app.
//
//  * TX_POWER_1M is a MEASUREMENT, not a setting. See docs/calibration.md.
//
//  * LOLIN C3 Pico VBAT: the divider IS on the board, but WEMOS ships it
//    DISCONNECTED so GPIO3 stays free. Bridge the two solder pads on the
//    underside to enable it. Measured after bridging: 2010 mV at the pin
//    for a 4.01 V cell -- a 1.995:1 ratio, so DIVIDER_X100 = 200 is right.
//
//  * The cutoff reads the DIVIDER, never the USB rail. With no cell
//    plugged in, the divider is fed by nothing and the pin floats around
//    400 mV, which looks exactly like a flat cell and used to put the
//    board straight into deep sleep -- taking the native USB port with
//    it. BAT_ABSENT_MV is the floor that tells the two cases apart.
// =====================================================================

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <esp_sleep.h>

// ---- BOARD SELECTION ------------------------------------------------
#define BOARD_LOLIN_C3_PICO  1
#define BOARD_M5_NANO_C6     2
#define BOARD_GENERIC        3

#define TARGET_BOARD  BOARD_LOLIN_C3_PICO     // <<< SET THIS

// ---- PER-BEACON SETTINGS --------------------------------------------
#define HUNT_ID      1        // hunt number, MAJOR. Same across one hunt.
#define STEP_ID      2        // step number, high byte of MINOR. One per beacon.
#define TX_POWER_1M  (-62)    // raw RSSI an iPhone reads at 1 m. MEASURE IT,
                              // per unit, in the final enclosure.

// ---- FEATURES -------------------------------------------------------
#define ENABLE_BATTERY_TELEMETRY  0   // 1 only once a VBAT divider is confirmed
#define ENABLE_ADC_PROBE          0   // 1 to hunt for a divider on BAT_ADC_PIN
#define ENABLE_LOW_VOLTAGE_CUTOFF 1   // stop and sleep before the cell is ruined

// The cutoff needs a voltage reading. Without telemetry there is nothing
// to act on, so it disables itself rather than pretend to protect.
#if ENABLE_LOW_VOLTAGE_CUTOFF && !ENABLE_BATTERY_TELEMETRY
  #undef  ENABLE_LOW_VOLTAGE_CUTOFF
  #define ENABLE_LOW_VOLTAGE_CUTOFF 0
  #define CUTOFF_UNAVAILABLE        1
#endif

// ---- BATTERY --------------------------------------------------------
#define BAT_NOMINAL_MV  4000  // your measured cell voltage, used by the probe
#define DIVIDER_X100    200   // divider ratio x100. Verified 1.995:1 -> 200
                              // on a bridged LOLIN C3 Pico.
#define BAT_CAL_X1000   1000  // trim x1000. Measured within 0.5%: no correction.
#define BAT_SAMPLES     9     // odd, median filtered
#define BAT_PERIOD_MS   60000 // frame refresh cadence; 10000 while testing
#define BAT_HYSTERESIS  2     // 2 steps = 20 mV before the frame is rewritten

#define BAT_LOW_MV      3500  // heartbeat turns yellow below this
#define BAT_CRIT_MV     3300  // heartbeat turns red below this
#define BAT_CUTOFF_MV   3200  // below this the beacon shuts itself down
#define BAT_ABSENT_MV   2500  // BELOW this, assume NO CELL rather than a dead
                              // one, and do not cut off. A LiPo genuinely at
                              // 2.5 V is scrap anyway, so this floor hides no
                              // real case -- but it keeps a USB-only board out
                              // of deep sleep, which is how you develop.
#define CUTOFF_STRIKES  3     // consecutive readings before acting, so a single
                              // sag during a radio burst cannot trigger it
#define CUTOFF_GRACE_MS 10000 // warning window at boot before sleeping, so the
                              // board stays reachable

// ---- ADC PROBE ------------------------------------------------------
#define PROBE_PERIOD_MS 3000  // fast enough to watch a cell being plugged in
#define PROBE_FLOOR_MV  150   // below this the pin reads as floating
#define PROBE_SPREAD_PC 2     // spread over this % of the reading = floating

// ---- STATUS LED -----------------------------------------------------
#define LED_NONE        0
#define LED_SIMPLE      1     // plain on/off LED via digitalWrite
#define LED_RGB         2     // WS2812 via neopixelWrite

// One colour per meaning, no colour used twice. RED IS RESERVED FOR
// ALARM: never reuse it for a count.
// Violet on a WS2812 is red+blue mixed; 180/0/255 keeps it clearly
// distinct from the pure blue of the step count through a diffuser.
// The low-battery colour needs its green channel high: below about 150
// the red dominates and it reads as orange, or as red at low brightness.
#define COL_BOOT     255, 255, 255   // white  - power-up flash
#define COL_HUNT     180,   0, 255   // violet - hunt number count
#define COL_STEP       0,   0, 255   // blue   - step number count
#define COL_ALIVE      0, 255,   0   // green  - heartbeat, all well
#define COL_LOW      255, 200,   0   // yellow - heartbeat, battery low
#define COL_CRIT     255,   0,   0   // red    - heartbeat, battery critical

#define LED_BRIGHTNESS  24    // identify phase. Full tilt draws ~60 mA/channel.
#define BEAT_BRIGHTNESS 6     // heartbeat: dim, but the colour stays readable
#define BLINK_ON_MS     250   // one count blink
#define BLINK_OFF_MS    350   // gap between blinks of the same group
#define GROUP_GAP_MS    1000  // gap BETWEEN groups: makes 1 violet + 1 blue legible
#define SETTLE_MS       2000  // before the heartbeat starts, or the first
                              // pulse gets counted as an extra blink
#define HEARTBEAT_MS    5000
#define BEAT_ON_MS      40

// ---- BOARD MAP ------------------------------------------------------
#if   TARGET_BOARD == BOARD_LOLIN_C3_PICO
  #define LED_TYPE       LED_RGB
  #define LED_PIN        7     // WS2812, confirmed on this board
  #define LED_POWER_PIN  (-1)  // none
  #define BAT_ADC_PIN    3     // GPIO3 = A3 = ADC1_CH3. NEVER GPIO2 (strapping).
                               // Requires the underside solder bridge, see header.
#elif TARGET_BOARD == BOARD_M5_NANO_C6
  #define LED_TYPE       LED_RGB
  #define LED_PIN        20    // VERIFY against your board pinout
  #define LED_POWER_PIN  19    // VERIFY: RGB power enable, driven HIGH
  #define BAT_ADC_PIN    1     // Grove white wire, ADC1. No on-board divider.
#else
  #define LED_TYPE       LED_NONE
  #define LED_PIN        (-1)
  #define LED_POWER_PIN  (-1)
  #define BAT_ADC_PIN    1     // pick any free ADC1 pin
#endif

// Project UUID A4C27B10-5F3E-4E2A-9D8C-1B6F0E3D7A52, big-endian on air.
// Fixed for the whole project: the app filters on this exact value.
static const uint8_t BEACON_UUID[16] = {
  0xA4, 0xC2, 0x7B, 0x10, 0x5F, 0x3E, 0x4E, 0x2A,
  0x9D, 0x8C, 0x1B, 0x6F, 0x0E, 0x3D, 0x7A, 0x52
};

static uint8_t         mfg[25];   // library prepends length (0x1A) and type (0xFF)
static BLEAdvertising *pAdv      = NULL;
static uint8_t         lastBat   = 0x00;
static uint8_t         beatR = 0, beatG = 255, beatB = 0;   // heartbeat colour
static uint32_t        lastCheck = 0, beatStart = 0;
static bool            ledOn     = false;
static uint8_t         strikes   = 0;

#if ENABLE_ADC_PROBE
static uint16_t probeMin = 0xFFFF, probeMax = 0;   // spread since boot
#endif

// ---------------------------------------------------------------------
//  Status LED
// ---------------------------------------------------------------------
static void ledInit() {
#if LED_TYPE == LED_RGB
  if (LED_POWER_PIN >= 0) { pinMode(LED_POWER_PIN, OUTPUT); digitalWrite(LED_POWER_PIN, HIGH); }
#elif LED_TYPE == LED_SIMPLE
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
#endif
}

// Full-strength colour, used by the boot flash and the two counts.
static void ledSet(uint8_t r, uint8_t g, uint8_t b) {
#if LED_TYPE == LED_RGB
  neopixelWrite(LED_PIN, (r * LED_BRIGHTNESS) / 255,
                         (g * LED_BRIGHTNESS) / 255,
                         (b * LED_BRIGHTNESS) / 255);
#elif LED_TYPE == LED_SIMPLE
  digitalWrite(LED_PIN, (r || g || b) ? HIGH : LOW);
#else
  (void)r; (void)g; (void)b;
#endif
}

static void ledOff() { ledSet(0, 0, 0); }

// Dimmed heartbeat, four times fainter than the counts and six times
// shorter, so the two can never be mistaken for one another.
static void ledBeat() {
#if LED_TYPE == LED_RGB
  neopixelWrite(LED_PIN, (beatR * BEAT_BRIGHTNESS) / 255,
                         (beatG * BEAT_BRIGHTNESS) / 255,
                         (beatB * BEAT_BRIGHTNESS) / 255);
#elif LED_TYPE == LED_SIMPLE
  digitalWrite(LED_PIN, HIGH);
#endif
}

// Heartbeat colour follows the cell. mv == 0 means "no telemetry", which
// is not a fault, so it stays green.
static void setBeatColor(uint16_t mv) {
  uint8_t c[3];
  if      (mv == 0)          { uint8_t t[] = {COL_ALIVE}; memcpy(c, t, 3); }
  else if (mv < BAT_CRIT_MV) { uint8_t t[] = {COL_CRIT};  memcpy(c, t, 3); }
  else if (mv < BAT_LOW_MV)  { uint8_t t[] = {COL_LOW};   memcpy(c, t, 3); }
  else                       { uint8_t t[] = {COL_ALIVE}; memcpy(c, t, 3); }
  beatR = c[0]; beatG = c[1]; beatB = c[2];
}

static void blinkCount(uint8_t n, uint8_t r, uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < n; i++) {
    ledSet(r, g, b);
    delay(BLINK_ON_MS);
    ledOff();
    delay(BLINK_OFF_MS);
  }
}

// White flash, then HUNT_ID in violet, then STEP_ID in blue. Reads the
// whole identity of the beacon without a phone. GROUP_GAP_MS between the
// two counts is what makes "one violet, one blue" legible.
static void identify() {
  ledSet(COL_BOOT);
  delay(400);
  ledOff();
  delay(600);

  blinkCount(HUNT_ID, COL_HUNT);
  delay(GROUP_GAP_MS - BLINK_OFF_MS);

  blinkCount(STEP_ID, COL_STEP);
  delay(SETTLE_MS);
}

// ---------------------------------------------------------------------
//  ADC -- all integer maths, no floating point anywhere
// ---------------------------------------------------------------------
static uint16_t readPinMv(uint8_t pin) {
  uint32_t s[BAT_SAMPLES];
  for (int i = 0; i < BAT_SAMPLES; i++) {
    s[i] = analogReadMilliVolts(pin);           // applies eFuse calibration
    delay(2);                                   // radio bursts sag the rail
  }
  for (int i = 1; i < BAT_SAMPLES; i++) {       // insertion sort, n = 9
    uint32_t v = s[i]; int j = i - 1;
    while (j >= 0 && s[j] > v) { s[j + 1] = s[j]; j--; }
    s[j + 1] = v;
  }
  return (uint16_t)s[BAT_SAMPLES / 2];          // median, not mean
}

static uint16_t pinToBattery(uint16_t pinMv) {
  return (uint16_t)(((uint32_t)pinMv * DIVIDER_X100 * BAT_CAL_X1000) / 100000UL);
}

static uint16_t readBatteryMv() {
  return pinToBattery(readPinMv(BAT_ADC_PIN));
}

static uint16_t expectedPinMv() {
  return (uint16_t)(((uint32_t)BAT_NOMINAL_MV * 100) / DIVIDER_X100);
}

#if ENABLE_ADC_PROBE
// Spread since boot, judged as a PERCENTAGE of the reading. A fixed
// millivolt threshold called 26 mV on a 430 mV reading "steady" when it
// was really 6% of noise. Proportional is the honest test.
static void probeAdc() {
  uint16_t mv = readPinMv(BAT_ADC_PIN);

  if (mv < probeMin) probeMin = mv;
  if (mv > probeMax) probeMax = mv;
  uint16_t spread   = probeMax - probeMin;
  uint16_t expected = expectedPinMv();

  const char *verdict;
  if (mv < PROBE_FLOOR_MV)                                verdict = "floating (near zero)";
  else if (spread * 100 > (uint32_t)mv * PROBE_SPREAD_PC) verdict = "noisy -> floating";
  else if (abs((int)mv - (int)expected) < 200)            verdict = "MATCH -> divider!";
  else                                                    verdict = "steady but wrong level";

  Serial.printf("GPIO%d : %4u mV | spread %3u mV (%2u%%) | expect %4u mV | %s\n",
                BAT_ADC_PIN, mv, spread,
                mv ? (uint16_t)(((uint32_t)spread * 100) / mv) : 0, expected, verdict);
}
#endif

// Spec: voltage_mV = 2500 + byte * 10, byte 0x00 means "no telemetry".
// Clamp to 1 so a flat cell never emits the sentinel by accident.
static uint8_t encodeBattery(uint16_t mv) {
  if (mv < 2510) return 1;
  uint16_t v = (mv - 2500) / 10;
  return v > 255 ? 255 : (uint8_t)v;
}

// ---------------------------------------------------------------------
//  Low-voltage cutoff
//
//  A beacon left switched on after a hunt would otherwise discharge its
//  cell until the ESP32 browns out, well past the point where a LiPo
//  takes permanent damage. This stops advertising and enters deep sleep
//  with NO wake source: the beacon stays off until it is power-cycled,
//  by which time you will be charging it anyway.
//
//  Deep sleep does not reach microamps on these boards -- the WS2812 and
//  the regulator keep drawing on the order of a milliamp. It still cuts
//  the drain by roughly fifty times, which is the point. The switch in
//  series with the cell remains the only true zero.
//
//  It fires ONLY between BAT_ABSENT_MV and BAT_CUTOFF_MV. Below the
//  floor there is no cell on the divider, and sleeping would just make
//  the board unreachable for no benefit.
// ---------------------------------------------------------------------
#if ENABLE_LOW_VOLTAGE_CUTOFF
static bool cellIsFlat(uint16_t mv) {
  return mv >= BAT_ABSENT_MV && mv < BAT_CUTOFF_MV;
}

static void shutdownFlat(uint16_t mv, bool grace) {
  Serial.printf("\nCUTOFF: %u mV is below %u mV.\n", mv, BAT_CUTOFF_MV);
  if (grace)
    Serial.printf("Sleeping in %u s -- double-tap RESET now to keep the port.\n",
                  CUTOFF_GRACE_MS / 1000);
  Serial.println("Charge the cell and power-cycle to restart.");
  Serial.flush();

  if (pAdv) pAdv->stop();

  // Deep sleep takes the native USB port down with it, so warn visibly
  // and leave a window before it happens.
  uint32_t blinks = grace ? (CUTOFF_GRACE_MS / 800) : 3;
  for (uint32_t i = 0; i < blinks; i++) {
    ledSet(COL_CRIT);
    delay(400);
    ledOff();
    delay(400);
  }
  esp_deep_sleep_start();                     // no wake source configured
}
#endif

// ---------------------------------------------------------------------
//  iBeacon payload
// ---------------------------------------------------------------------
static void buildPayload(uint8_t bat) {
  uint16_t minor = ((uint16_t)STEP_ID << 8) | bat;

  mfg[0]  = 0x4C;  mfg[1]  = 0x00;            // Apple company ID, little-endian
  mfg[2]  = 0x02;  mfg[3]  = 0x15;            // iBeacon subtype + remaining length
  memcpy(&mfg[4], BEACON_UUID, 16);           // UUID, big-endian
  mfg[20] = (HUNT_ID >> 8) & 0xFF;            // MAJOR, big-endian
  mfg[21] =  HUNT_ID       & 0xFF;
  mfg[22] = (minor   >> 8) & 0xFF;            // MINOR, big-endian
  mfg[23] =  minor         & 0xFF;
  mfg[24] = (uint8_t)(int8_t)TX_POWER_1M;     // -59 -> 0xC5
}

static void publish(uint8_t bat) {
  buildPayload(bat);

  BLEAdvertisementData adv;
  adv.setFlags(0x04);                         // BR/EDR not supported
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  adv.setManufacturerData(String((char *)mfg, sizeof(mfg)));
#else
  adv.setManufacturerData(std::string((char *)mfg, sizeof(mfg)));
#endif

  pAdv->stop();                               // required before rewriting
  pAdv->setAdvertisementData(adv);
  pAdv->start();
}

static void dumpPayload() {
  Serial.printf("AD mfg (%u bytes): 1A FF", sizeof(mfg));
  for (size_t i = 0; i < sizeof(mfg); i++) Serial.printf(" %02X", mfg[i]);
  Serial.printf("\nAdvertising total: %u / 31 bytes\n", 3 + 2 + sizeof(mfg));
  Serial.println("Expect 1A FF 4C 00 02 15 ... -- anything else and iOS ignores it.");
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Treasure Hunt beacon ===");

  ledInit();
  ledOff();

#if ENABLE_BATTERY_TELEMETRY || ENABLE_ADC_PROBE
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);      // ~0 to 3.1 V range
#endif

#if ENABLE_BATTERY_TELEMETRY
  uint16_t pinMv = readPinMv(BAT_ADC_PIN);
  uint16_t batMv = pinToBattery(pinMv);
  lastBat = encodeBattery(batMv);
  setBeatColor(batMv);

  Serial.printf("ADC GPIO%d : %4u mV at the pin\n", BAT_ADC_PIN, pinMv);
  Serial.printf("Divider    : %u.%02u  x BAT_CAL %u.%03u\n",
                DIVIDER_X100 / 100, DIVIDER_X100 % 100,
                BAT_CAL_X1000 / 1000, BAT_CAL_X1000 % 1000);
  Serial.printf("VBAT       : %4u mV -> code %u\n", batMv, lastBat);
  Serial.println("--> Compare VBAT against a multimeter on the cell.");
  Serial.println("    Off by a few %: set BAT_CAL_X1000 to");
  Serial.println("    multimeter * 1000 / VBAT, then reflash.");

  #if ENABLE_LOW_VOLTAGE_CUTOFF
  if (batMv < BAT_ABSENT_MV) {
    Serial.printf("Cutoff     : INACTIVE -- %u mV is under the %u mV floor,\n",
                  batMv, BAT_ABSENT_MV);
    Serial.println("             so no cell is connected to the divider.");
  } else if (cellIsFlat(batMv)) {
    shutdownFlat(batMv, true);                // refuse to finish off a flat cell
  } else {
    Serial.printf("Cutoff     : armed at %u mV\n", BAT_CUTOFF_MV);
  }
  #endif
#else
  lastBat = 0x00;                             // spec sentinel: no telemetry
  setBeatColor(0);                            // green: nothing to report
  Serial.println("Battery telemetry: OFF (MINOR low byte = 0x00)");
  #ifdef CUTOFF_UNAVAILABLE
  Serial.println("Low-voltage cutoff: DISABLED -- it needs a voltage reading.");
  Serial.println("  Bridge or wire the VBAT divider, then set");
  Serial.println("  ENABLE_BATTERY_TELEMETRY to 1. Until then the cell is");
  Serial.println("  protected only by its own pack circuit, if it has one.");
  #endif
#endif

#if ENABLE_ADC_PROBE
  Serial.printf("Probing GPIO%d every %u s, cell assumed at %u mV.\n",
                BAT_ADC_PIN, PROBE_PERIOD_MS / 1000, BAT_NOMINAL_MV);
  Serial.printf("A correct divider would read ~%u mV at the pin.\n",
                expectedPinMv());
#endif

  Serial.println();
  buildPayload(lastBat);
  dumpPayload();

  BLEDevice::init("");                        // EMPTY name, see header notes
  BLEDevice::setPower(ESP_PWR_LVL_P9);        // max TX power, stable

  pAdv = BLEDevice::getAdvertising();
  pAdv->setScanResponse(false);               // beacons are not scanned
  pAdv->setMinInterval(0x00A0);               // 160 * 0.625 ms = 100 ms
  pAdv->setMaxInterval(0x00A0);
  publish(lastBat);

  uint16_t minor = ((uint16_t)STEP_ID << 8) | lastBat;
  Serial.printf("\nmajor=%u  step=%u  minor=0x%04X (%u)  txPower=%d dBm\n",
                HUNT_ID, STEP_ID, minor, minor, TX_POWER_1M);
  Serial.printf("LED: white, then %u violet, then %u blue, then green pulse.\n",
                HUNT_ID, STEP_ID);
  Serial.println("App side: step = minor >> 8, battery = minor & 0xFF.");
  Serial.println("Range on UUID + major ONLY, never on a full minor.");
  Serial.printf("Advertising: %s\n\n", pAdv->isAdvertising() ? "YES" : "NO");

  identify();
  beatStart = millis();
}

// ---------------------------------------------------------------------
void loop() {
  uint32_t now = millis();

#if ENABLE_ADC_PROBE
  if (now - lastCheck >= PROBE_PERIOD_MS) {
    lastCheck = now;
    probeAdc();
  }
#elif ENABLE_BATTERY_TELEMETRY
  if (now - lastCheck >= BAT_PERIOD_MS) {
    lastCheck = now;

    uint16_t pinMv = readPinMv(BAT_ADC_PIN);
    uint16_t batMv = pinToBattery(pinMv);
    uint8_t  bat   = encodeBattery(batMv);

    Serial.printf("[%6us] pin %4u mV | VBAT %4u mV | code %3u | minor 0x%04X",
                  (unsigned)(now / 1000), pinMv, batMv, bat,
                  (STEP_ID << 8) | bat);

    // Hysteresis keeps MINOR from flickering on ADC noise, which would
    // make the app's device list churn.
    if (abs((int)bat - (int)lastBat) >= BAT_HYSTERESIS) {
      lastBat = bat;
      setBeatColor(batMv);
      publish(bat);
      Serial.print("  <- frame updated");
    }

  #if ENABLE_LOW_VOLTAGE_CUTOFF
    if (cellIsFlat(batMv)) {
      strikes++;
      Serial.printf("  <- below cutoff (%u/%u)", strikes, CUTOFF_STRIKES);
      if (strikes >= CUTOFF_STRIKES) { Serial.println(); shutdownFlat(batMv, false); }
    } else {
      strikes = 0;
    }
  #endif

    Serial.println();
  }
#endif

  if (ledOn) {
    if (now - beatStart >= BEAT_ON_MS) { ledOff(); ledOn = false; }
  } else if (now - beatStart >= HEARTBEAT_MS) {
    ledBeat();
    ledOn = true;
    beatStart = now;
  }

  delay(10);   // advertising runs in the BLE stack, independent of loop()
}

// =====================================================================
//  LED CODE
//
//  At power-up, once:
//    1 white flash        beacon is booting
//    N violet blinks      hunt number   (HUNT_ID / MAJOR)
//      -- one second pause --
//    M blue blinks        step number   (STEP_ID)
//
//  Then, forever, one brief pulse every 5 s:
//    green                alive, battery fine or not measured
//    yellow               battery below BAT_LOW_MV
//    red                  battery below BAT_CRIT_MV
//
//  Slow red blinks with no identify sequence: the low-voltage cutoff
//  fired. Charge the cell and power-cycle. Recover the board with a
//  double-tap on RESET -- the ROM bootloader runs regardless of deep
//  sleep, and the port comes back.
//
//  Arm the beacon, count violet, count blue, check the pulse colour,
//  close the box. That is the whole pre-flight check, no phone needed.
//
//  RED IS RESERVED FOR ALARM and appears nowhere else.
//
//  ---------------------------------------------------------------------
//  BRINGING UP A NEW BEACON
//
//  1. Set TARGET_BOARD, HUNT_ID and STEP_ID. Flash. Label the board
//     physically right now -- two identical boards are indistinguishable
//     later.
//
//  2. Serial monitor at 115200. The dump must show 1A FF 4C 00 02 15,
//     and Advertising must say YES. If not, stop here.
//
//  3. Confirm the frame with a Mac (LightBlue), an Android phone, or
//     firmware/scanner. NOT nRF Connect on iPhone -- it cannot see
//     iBeacons at all.
//
//  4. Range it from your app. If nothing appears, the UUID is the first
//     suspect, then Info.plist, then a CLLocationManager that was not
//     retained, then a filter on the full minor.
//
//  5. Calibrate TX_POWER_1M: raw RSSI at exactly 1 m, median over 30 s,
//     body out of the line, beacon in its closed enclosure. Per unit.
//
//  6. Battery, LOLIN C3 Pico: bridge the two solder pads on the
//     underside to connect the cell to the on-board divider on GPIO3.
//     WEMOS ships it open so the pin stays free. Once bridged the
//     divider draws current continuously while a cell is connected,
//     which is why the switch belongs in series with the battery.
//
//     On boards with no divider at all, wire your own:
//
//        switched battery + ---- R1 100k ----+---- R2 100k ---- GND
//                                            |
//                                            +---- C1 100nF --- GND
//                                            |
//                                            +---- BAT_ADC_PIN
//
//  7. Before hiding a beacon outdoors, consider setting LED_TYPE to
//     LED_NONE. A blinking box under a rock is a clue you did not mean
//     to give.
// =====================================================================
