# Treasure Hunt — Beacon

**English** · [Français](README.fr.md)

BLE beacon specification and DIY build for the **Treasure Hunt** iOS app.

**Frame specification version 1.0.** Beacons and app releases are compatible
within the same major version.

Each hunt step can be marked by an iBeacon. The app ranges nearby beacons to
guide players over the last 30 metres, where GPS alone is too imprecise.

You have two options:

- **Buy** a commercial iBeacon and reconfigure it with the values below.
- **Build** your own from an ESP32 board — see [Build your own](#build-your-own).

---

## Beacon specification

This is the contract. Any beacon that advertises the following frame will be
recognised by the app, regardless of how it was made.

### Identifiers

| Field | Value |
|---|---|
| **UUID** | `A4C27B10-5F3E-4E2A-9D8C-1B6F0E3D7A52` |
| **MAJOR** | Hunt number, `1`–`65535` |
| **MINOR** | `(step << 8) \| battery` — see below |

The UUID is **fixed for the whole project**. It is a namespace, not a device
identifier — Apple's model is one UUID per application, with major and minor
carrying everything else. Do not change it: the app filters on this exact value
and will not see a beacon advertising anything else.

### MINOR encoding

The 16-bit minor carries two fields:

```
 15                    8 7                     0
┌───────────────────────┬───────────────────────┐
│      step (1-255)     │    battery (0-255)    │
└───────────────────────┴───────────────────────┘
```

| Byte | Meaning |
|---|---|
| High | Step number within the hunt, `1`–`255` |
| Low | Battery level, or `0x00` for "no telemetry" |

Battery byte to voltage:

```
voltage_mV = 2500 + (minor & 0xFF) * 10
```

This gives 10 mV steps from 2.50 V to 5.05 V, covering both LiPo and LiFePO4.
`0x00` would decode as 2.50 V — a cell that is already dead — so it is safe to
use as the "no telemetry" sentinel.

Commercial beacons cannot produce this telemetry. Set their minor to
`step << 8` (for example `0x0100` = 256 for step 1) and the app will show the
step with an unknown battery level.

### Radio parameters

| Parameter | Value |
|---|---|
| Advertising interval | 100 ms |
| Advertising flags | `0x04` (BR/EDR not supported) |
| Scan response | Disabled |
| TX power field | **Measured per beacon** — see [calibration](docs/calibration.md) |

The TX power byte is the RSSI an iPhone reads at exactly 1 metre. It is not a
setting you copy — a wrong value makes every distance estimate wrong. Measure it
for each unit, in its final enclosure.

### Full manufacturer data

25 bytes, following the standard `0x1A 0xFF` length and type prefix:

```
4C 00                                            Apple company ID, little-endian
02 15                                            iBeacon subtype + remaining length
A4 C2 7B 10 5F 3E 4E 2A 9D 8C 1B 6F 0E 3D 7A 52  UUID, big-endian
00 01                                            MAJOR = 1, big-endian
01 00                                            MINOR = step 1, no telemetry
C5                                               TX power = -59 dBm
```

Total advertising payload: 3 bytes of flags + 27 bytes of manufacturer data
structure = **30 of the 31 available bytes**. There is no room for a device
name. This is why the firmware calls `BLEDevice::init("")` with an empty string.


### Status LED

Boards with an RGB LED signal their identity at power-up, once:

| Signal | Meaning |
|---|---|
| 1 white flash | booting |
| N violet blinks | hunt number (MAJOR) |
| M blue blinks | step number |

Then a brief pulse every 5 s: **green** alive, **amber** battery low, **red**
battery critical. Red appears nowhere else, so a red pulse always means the
cell needs attention.

Arm the beacon, count violet, count blue, check the pulse colour, close the
box. That is the whole pre-flight check, no phone needed.

---

## Using a commercial beacon

Most commercial iBeacons are reconfigurable through their manufacturer's app.
Set:

- UUID → `A4C27B10-5F3E-4E2A-9D8C-1B6F0E3D7A52`
- Major → your hunt number
- Minor → `step << 8` (step 1 → `256`, step 2 → `512`, step 3 → `768`)
- Advertising interval → 100 ms if adjustable

**Check before buying.** Some low-cost models ship with a factory-locked UUID.
Those cannot be used. The product page must state that the UUID is
user-configurable.

---

## Build your own

| | |
|---|---|
| Firmware | [`firmware/beacon/`](firmware/beacon/) |
| Board choice and wiring | [`hardware/`](hardware/) |
| Enclosure | [`enclosure/`](enclosure/) |
| TX power calibration | [`docs/calibration.md`](docs/calibration.md) |
| Battery and autonomy | [`docs/battery.md`](docs/battery.md) |
| When nothing is detected | [`docs/troubleshooting.md`](docs/troubleshooting.md) |

Minimum to get a beacon on the air: an ESP32 board with BLE, a USB cable, and
the Arduino IDE with the ESP32 core 3.x. Battery and enclosure come later.

---

## Repository layout

```
firmware/
  beacon/          Beacon firmware — flash one per hunt step
  scanner/         Bench scanner — decodes frames, useful for validation
  vbat_probe/      Differential probe — finds a board's VBAT divider
  CONFIGURATION.md Per-beacon settings
hardware/          Boards, wiring diagrams, bill of materials
enclosure/         Parametric OpenSCAD case, ready-made STLs
docs/              Calibration, battery, troubleshooting
```

Code comments are in English only, to keep a single source of truth.
Documentation is available in English and French.

---

## Safety

This project involves lithium cells. Read the warnings in
[`hardware/README.md`](hardware/README.md) before wiring a battery. Sealed
enclosures left in direct sunlight can exceed the safe operating range of both
the board and the cell.

---

## License

MIT — see [LICENSE](LICENSE).
