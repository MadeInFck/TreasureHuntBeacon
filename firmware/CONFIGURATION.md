# Configuration

**English** · [Français](CONFIGURATION.fr.md)

Everything you change lives at the top of `beacon/beacon.ino`.

## Per-beacon settings

| Constant | Meaning |
|---|---|
| `STEP_ID` | Step number, high byte of MINOR. **Change for each beacon.** |
| `HUNT_ID` | Hunt number, MAJOR. Same across all beacons of one hunt. |
| `TX_POWER_1M` | RSSI at 1 m. **Measure per unit** — see [calibration](../docs/calibration.md). |

Flash the same file once per step, changing only `STEP_ID`.

Label each board physically as you flash it. Two identical boards are
impossible to tell apart afterwards, and an RSSI attributed to the wrong
beacon invalidates the whole comparison.

## Optional features

| Flag | Default | Requires |
|---|---|---|
| `ENABLE_BATTERY_TELEMETRY` | `0` | A voltage divider — see [hardware](../hardware/README.md) |
| `ENABLE_STATUS_LED` | `1` | An on-board LED |

With telemetry off, the low byte of MINOR is `0x00`, which the specification
defines as "no telemetry". Turn it on only once the divider is wired, or the
app will display a false battery level.

## ADC pin for battery telemetry

Set `BAT_ADC_PIN` to a pin on **ADC1**. ADC2 is unusable while the radio is
active on several ESP32 variants.

| Board | Suggested pin | Note |
|---|---|---|
| M5Stack NanoC6 | `1` | Grove white wire; only G1 and G2 are exposed |
| XIAO ESP32C6 | `0` (A0) | GPIO3 and GPIO14 are reserved for the RF switch |
| XIAO ESP32C3 | `3` (A1) | **Avoid A0/GPIO2**, it is a strapping pin |
| ESP32 classic | `32`–`39` | ADC1 range |

Calibrate `BAT_CAL` once per board: measure the real cell voltage with a
multimeter, divide by what the serial console prints, write the quotient.
1% resistors alone introduce up to 2% error.

## Status LED

`LED_BUILTIN` is defined by the core for recognised boards. If the LED stays
dark, check your board's pinout and set the number explicitly. If it stays lit
and blinks off instead, set `LED_ACTIVE_HIGH` to `0`.

Remove or disable the LED before hiding a beacon outdoors. A blinking box under
a rock is a clue you did not intend to give.

## Arduino IDE

- ESP32 core **3.x** required for C6 and H2 targets
- Select the exact board in the Tools menu
- Serial monitor at **115200**
- On native-USB boards the port disappears and reappears at each reset;
  double-press RESET to force the bootloader if it does not show up
