// =====================================================================
//  Treasure Hunt - Beacon firmware
//  https://github.com/MadeInFck/TreasureHuntBeacon
//
//  Advertises an iBeacon frame that the Treasure Hunt iOS app can range.
//  Flash one board per hunt step, changing STEP_ID each time.
//
//  The manufacturer payload is built byte by byte rather than through the
//  BLEBeacon class. That class applies ENDIAN_CHANGE_U16 internally, and
//  which fields it touches has varied between core versions. Building the
//  25 bytes by hand is immune to that, and to the Bluedroid/NimBLE split.
//
//  See CONFIGURATION.md for per-board settings.
// =====================================================================

#include <BLEDevice.h>
#include <BLEUtils.h>

// ---- PER-BEACON SETTINGS --------------------------------------------
#define STEP_ID      1        // step number, high byte of MINOR (1-255)
#define HUNT_ID      1        // hunt number, MAJOR (1-65535)
#define TX_POWER_1M  (-59)    // RSSI an iPhone reads at 1 m -- MEASURE IT
                              // see docs/calibration.md

// ---- OPTIONAL FEATURES ----------------------------------------------
#define ENABLE_BATTERY_TELEMETRY  0   // 1 requires a voltage divider
#define ENABLE_STATUS_LED         1   // brief flash every 3 s

#if ENABLE_BATTERY_TELEMETRY
  #define BAT_ADC_PIN     1     // ADC1 pin, see CONFIGURATION.md
  #define DIVIDER_RATIO   2.0f  // 100k / 100k
  #define BAT_CAL         1.000f// multimeter reading / measured value
  #define BAT_SAMPLES     9     // odd, median filtered
  #define BAT_PERIOD_MS   60000 // how often the frame is refreshed
  #define BAT_HYSTERESIS  2     // 2 steps = 20 mV, stops MINOR flickering
#endif

#if ENABLE_STATUS_LED
  #ifndef LED_BUILTIN
    #define LED_BUILTIN 7       // M5Stack NanoC6 blue LED; adjust per board
  #endif
  #define LED_ACTIVE_HIGH  1    // set to 0 if the LED is inverted
#endif

// Project UUID A4C27B10-5F3E-4E2A-9D8C-1B6F0E3D7A52, big-endian on air.
// Do not change: the app filters on this exact value.
static const uint8_t BEACON_UUID[16] = {
  0xA4, 0xC2, 0x7B, 0x10, 0x5F, 0x3E, 0x4E, 0x2A,
  0x9D, 0x8C, 0x1B, 0x6F, 0x0E, 0x3D, 0x7A, 0x52
};

// 25 bytes. The library prepends the length (0x1A) and type (0xFF) itself.
static uint8_t         mfg[25];
static BLEAdvertising *pAdv;
static uint8_t         lastBat  = 0xFF;
static uint32_t        lastCheck = 0;

// ---------------------------------------------------------------------
#if ENABLE_BATTERY_TELEMETRY
static uint16_t readBatteryMv() {
  uint32_t s[BAT_SAMPLES];
  for (int i = 0; i < BAT_SAMPLES; i++) {
    s[i] = analogReadMilliVolts(BAT_ADC_PIN);   // applies eFuse calibration
    delay(2);
  }
  for (int i = 1; i < BAT_SAMPLES; i++) {       // insertion sort, n <= 9
    uint32_t v = s[i]; int j = i - 1;
    while (j >= 0 && s[j] > v) { s[j + 1] = s[j]; j--; }
    s[j + 1] = v;
  }
  return (uint16_t)(s[BAT_SAMPLES / 2] * DIVIDER_RATIO * BAT_CAL);
}

// Spec: voltage_mV = 2500 + byte * 10. Byte 0x00 means "no telemetry".
static uint8_t encodeBattery(uint16_t mv) {
  if (mv < 2510) return 1;                      // clamp: never emit the sentinel
  uint16_t v = (mv - 2500) / 10;
  return v > 255 ? 255 : (uint8_t)v;
}
#endif

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

static void dumpPayload() {
  Serial.printf("AD mfg (%u bytes): 1A FF", sizeof(mfg));
  for (size_t i = 0; i < sizeof(mfg); i++) Serial.printf(" %02X", mfg[i]);
  Serial.printf("\nAdvertising total: %u / 31 bytes\n", 3 + 2 + sizeof(mfg));
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

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);

#if ENABLE_STATUS_LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_ACTIVE_HIGH ? LOW : HIGH);
#endif

#if ENABLE_BATTERY_TELEMETRY
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);   // ~0 to 3.1 V range
  uint16_t mv = readBatteryMv();
  lastBat = encodeBattery(mv);
  Serial.printf("Battery: %u mV -> code %u\n", mv, lastBat);
#else
  lastBat = 0x00;                             // spec sentinel: no telemetry
#endif

  buildPayload(lastBat);
  dumpPayload();

  BLEDevice::init("");                        // empty name: 30 of 31 bytes used
  BLEDevice::setPower(ESP_PWR_LVL_P9);        // max TX power, stable

  pAdv = BLEDevice::getAdvertising();
  pAdv->setScanResponse(false);               // beacons are not scanned
  pAdv->setMinInterval(0x00A0);               // 160 * 0.625 ms = 100 ms
  pAdv->setMaxInterval(0x00A0);

  // Apple's spec calls for non-connectable advertising (ADV_TYPE_NONCONN_IND).
  // That constant is Bluedroid-only and does not exist on NimBLE targets such
  // as the ESP32-C6. Connectable advertising is ranged identically by
  // CoreLocation, so it is omitted here for portability.

  publish(lastBat);

  Serial.printf("Beacon live: major=%d step=%d minor=0x%04X txPower=%d dBm\n",
                HUNT_ID, STEP_ID, (STEP_ID << 8) | lastBat, TX_POWER_1M);
  Serial.printf("Advertising: %s\n", pAdv->isAdvertising() ? "YES" : "NO");
}

void loop() {
#if ENABLE_BATTERY_TELEMETRY
  if (millis() - lastCheck >= BAT_PERIOD_MS) {
    lastCheck = millis();
    uint16_t mv  = readBatteryMv();
    uint8_t  bat = encodeBattery(mv);
    if (abs((int)bat - (int)lastBat) >= BAT_HYSTERESIS) {
      lastBat = bat;
      publish(bat);
      Serial.printf("Battery update: %u mV -> code %u\n", mv, bat);
    }
  }
#endif

#if ENABLE_STATUS_LED
  digitalWrite(LED_BUILTIN, LED_ACTIVE_HIGH ? HIGH : LOW);
  delay(60);
  digitalWrite(LED_BUILTIN, LED_ACTIVE_HIGH ? LOW : HIGH);
  delay(2940);                                // ~2% duty, negligible drain
#else
  delay(1000);
#endif
}
