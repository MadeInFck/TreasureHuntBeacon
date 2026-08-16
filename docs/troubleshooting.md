# Troubleshooting

**English** · [Français](troubleshooting.fr.md)

## nRF Connect on iPhone shows nothing

**This is expected. It is not a fault in your beacon.**

iOS filters Apple manufacturer data (`0x004C`) out of CoreBluetooth. Any iOS
app scanning that way sees an empty packet — no UUID, no major, no minor, no
iBeacon badge. The same beacon shown in nRF Connect for Android displays
everything.

On iPhone, **CoreLocation is the only way to see an iBeacon**. Do not use an
iOS scanner app to debug a beacon.

To inspect raw frames, use one of:

- **A Mac.** Macs do not apply the same filter. LightBlue works.
- **`firmware/scanner`** on a second ESP32.
- **An Android phone** with nRF Connect.

## Nothing detected, in order of likelihood

**1. UUID mismatch.** By far the most common cause, and it fails silently —
CoreLocation reports nothing rather than an error. Compare the UUID in the app
against bytes 4–19 of the advertised payload, in that exact order.

**2. Missing Info.plist key.** `NSLocationWhenInUseUsageDescription` must be
present. Without it iOS refuses silently: no crash, no error, nothing.

**3. Authorisation not granted.** `requestWhenInUseAuthorization()` must be
called and the dialog accepted. Check in Settings → your app → Location.

**4. Monitoring instead of ranging.** They are different APIs.
`startRangingBeacons(satisfying:)` is the one that yields RSSI and distance.

**5. CLLocationManager not retained.** A manager declared as a local variable
is deallocated when the function returns, and ranging stops with no message.
Hold it in a property.

**6. Filtering on a full minor.** Constrain on UUID and major only. The low
byte of minor carries battery level and changes as the cell drains — a filter
on the complete minor will stop matching.

## Verifying the beacon side

Open the serial monitor at 115200. The firmware prints the payload before
advertising starts:

```
AD mfg (25 bytes): 1A FF 4C 00 02 15 A4 C2 ...
Advertising total: 30 / 31 bytes
Advertising: YES
```

If `4C 00 02 15` follows `1A FF`, the frame is correct. If `Advertising` says
`NO`, the stack refused to start and the problem is on the board.

If the dump is correct **and** advertising is on, the beacon is almost
certainly fine and the problem is in the app.

## Compilation errors

### `'ADV_TYPE_NONCONN_IND' was not declared in this scope`

That constant belongs to Bluedroid. On NimBLE targets such as the ESP32-C6 it
does not exist. Delete the `setAdvertisementType()` line — the shipped firmware
already omits it.

The beacon then advertises as connectable rather than non-connectable. Apple's
specification calls for non-connectable, but CoreLocation ranges both
identically and commercial beacons ship connectable.

### Board not in the list

C6 and H2 targets require ESP32 core **3.x**. Update the core, not your search.

### No serial port

Native-USB boards only enumerate once running, and the port disappears at each
reset. Double-press RESET to force the bootloader. Check the cable is a data
cable — charge-only USB cables are the classic time sink.

### Everything compiles but the serial monitor goes quiet

Arduino-ESP32 builds RISC-V targets against **newlib-nano**, which does not
implement floating point formatting. A single `%f` in a `Serial.printf` can
swallow the whole line or print garbage — and the failure looks like the board
has stopped rather than a formatting bug.

The shipped firmware uses integer maths everywhere for this reason:
`DIVIDER_X100` and `BAT_CAL_X1000` are scaled integers printed with
division and modulo. Keep it that way.

### `redefinition of 'setup()'` and a dozen similar errors

The Arduino IDE concatenates **every `.ino` in a sketch folder** before
compiling. Two firmware variants side by side means two `setup()`, two
`BEACON_UUID`, and so on.

One sketch per folder, and the folder name must match the `.ino` name. Avoid
spaces in sketch names while you are at it.

## Distances are wrong

Consistently **too short** means `TX_POWER_1M` is too low. Consistently too
long means it is too high. Do not tune by trial and error — measure at 1 m, see
[calibration](calibration.md).

Errors growing with distance are inherent to the logarithmic model, not a
calibration fault. Beyond 5 m an iBeacon estimate is indicative at best.
