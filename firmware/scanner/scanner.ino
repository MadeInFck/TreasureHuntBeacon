// =====================================================================
//  Treasure Hunt - Bench scanner
//
//  Decodes iBeacon frames, filters RSSI with a rolling median and
//  estimates distance. Use it to validate a beacon's payload and to
//  match two units against each other.
//
//  Why this exists: iOS strips Apple manufacturer data from CoreBluetooth,
//  so nRF Connect on iPhone cannot show iBeacon frames at all. This scanner
//  is not subject to that. A Mac or an Android phone also works.
//
//  IMPORTANT: do not calibrate TX_POWER_1M with this scanner. That field is
//  the RSSI a *phone* reads at 1 m, and antenna gains differ by several dB.
//  Use it to verify payloads and compare units, nothing more.
//
//  Flash on any ESP32 with BLE. Keep it on USB, consumption is irrelevant.
// =====================================================================

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define SCAN_SECONDS   1
#define WIN            7      // rolling median window (odd)
#define MAX_BEACONS    8
#define PATH_LOSS_N    2.0f   // 2.0 free space; 2.5-3.5 in vegetation
#define STALE_MS       5000   // after this, the beacon is reported lost

struct Beacon {
  bool     used;
  uint8_t  uuid[16];
  uint16_t major, minor;
  int8_t   tx;
  int8_t   ring[WIN];
  uint8_t  n, idx;
  uint32_t lastSeen;
  uint32_t hits;
};

static Beacon  table[MAX_BEACONS];
static BLEScan *pScan;

// ---------------------------------------------------------------------
static int8_t median(const Beacon &b) {
  int8_t t[WIN];
  uint8_t n = b.n < WIN ? b.n : WIN;
  memcpy(t, b.ring, n);
  for (uint8_t i = 1; i < n; i++) {
    int8_t v = t[i]; int8_t j = i - 1;
    while (j >= 0 && t[j] > v) { t[j + 1] = t[j]; j--; }
    t[j + 1] = v;
  }
  return t[n / 2];
}

static float distance(int8_t tx, int8_t rssi) {
  if (rssi == 0) return -1.0f;
  return powf(10.0f, (float)(tx - rssi) / (10.0f * PATH_LOSS_N));
}

static Beacon *slotFor(uint16_t major, uint16_t minor) {
  // Match on major + step only: the low byte of minor carries battery
  // level and changes as the cell drains.
  uint8_t step = minor >> 8;
  for (int i = 0; i < MAX_BEACONS; i++)
    if (table[i].used && table[i].major == major && (table[i].minor >> 8) == step)
      return &table[i];
  for (int i = 0; i < MAX_BEACONS; i++)
    if (!table[i].used) { table[i] = Beacon{}; table[i].used = true; return &table[i]; }
  return nullptr;
}

// ---------------------------------------------------------------------
static void handleDevice(BLEAdvertisedDevice &d) {
  if (!d.haveManufacturerData()) return;

  auto md = d.getManufacturerData();
  const uint8_t *p = (const uint8_t *)md.c_str();
  if (md.length() < 25) return;

  // iBeacon prefix: Apple (4C 00) + subtype 02 + remaining length 15
  if (!(p[0] == 0x4C && p[1] == 0x00 && p[2] == 0x02 && p[3] == 0x15)) return;

  uint16_t major = ((uint16_t)p[20] << 8) | p[21];
  uint16_t minor = ((uint16_t)p[22] << 8) | p[23];

  Beacon *b = slotFor(major, minor);
  if (!b) return;

  bool isNew = (b->hits == 0);
  memcpy(b->uuid, &p[4], 16);
  b->major = major; b->minor = minor;
  b->tx    = (int8_t)p[24];
  b->ring[b->idx] = (int8_t)d.getRSSI();
  b->idx  = (b->idx + 1) % WIN;
  if (b->n < WIN) b->n++;
  b->lastSeen = millis();
  b->hits++;

  if (isNew) {                                  // raw dump on first sighting
    Serial.printf("\n[NEW] %s  payload: ", d.getAddress().toString().c_str());
    for (int i = 0; i < 25; i++) Serial.printf("%02X ", p[i]);
    Serial.print("\n      UUID: ");
    for (int i = 0; i < 16; i++) {
      Serial.printf("%02X", b->uuid[i]);
      if (i == 3 || i == 5 || i == 7 || i == 9) Serial.print("-");
    }
    Serial.println();
  }
}

static void printTable() {
  Serial.println("\n MAJOR  STEP  BATTERY     tx  RSSI   med    dist   frames");
  Serial.println("-----------------------------------------------------------");
  uint32_t now = millis();
  for (int i = 0; i < MAX_BEACONS; i++) {
    Beacon &b = table[i];
    if (!b.used) continue;

    uint8_t step = b.minor >> 8;
    uint8_t bat  = b.minor & 0xFF;
    char batStr[12];
    if (bat == 0) snprintf(batStr, sizeof(batStr), "   n/a");
    else          snprintf(batStr, sizeof(batStr), "%5u mV", 2500 + bat * 10);

    bool   stale = (now - b.lastSeen) > STALE_MS;
    int8_t med   = median(b);
    int8_t last  = b.ring[(b.idx + WIN - 1) % WIN];

    Serial.printf(" %5u %5u  %8s  %4d %5d  %4d  %6.2fm  %6u%s\n",
                  b.major, step, batStr, b.tx, last, med,
                  distance(b.tx, med), b.hits, stale ? "  [LOST]" : "");
  }
}

// ---------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nTreasure Hunt - bench scanner");

  BLEDevice::init("");
  pScan = BLEDevice::getScan();
  pScan->setActiveScan(false);   // passive: iBeacon lives in ADV, no scan req
  pScan->setInterval(100);
  pScan->setWindow(99);          // window < interval, or the radio saturates
}

void loop() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  BLEScanResults *r = pScan->start(SCAN_SECONDS, false);
  for (int i = 0; i < r->getCount(); i++) { BLEAdvertisedDevice d = r->getDevice(i); handleDevice(d); }
#else
  BLEScanResults r = pScan->start(SCAN_SECONDS, false);
  for (int i = 0; i < r.getCount(); i++) { BLEAdvertisedDevice d = r.getDevice(i); handleDevice(d); }
#endif
  pScan->clearResults();
  printTable();
}
