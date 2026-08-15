# TX power calibration

**English** · [Français](calibration.fr.md)

The `TX_POWER_1M` byte is the single value that turns a raw RSSI into metres.
It is **the RSSI an iPhone reads at exactly 1 metre from the beacon**. Not a
setting to copy, not a datasheet figure — a measurement.

Get it wrong and every distance the app reports is wrong, in a way that looks
like a bug in your ranging code.

## Procedure

Do this once per beacon, in its final enclosure.

1. **Beacon in its closed enclosure**, with its battery in place. Plastic and a
   LiPo pouch easily cost 4–8 dB.
2. **About 1 m above ground**, on a non-metallic support, in the clear.
3. **iPhone at exactly 1 m**, held vertically, screen facing the beacon,
   nothing in between and **your body out of the line**. A human body absorbs
   10–15 dB at 2.4 GHz.
4. **Record the raw RSSI for 30 seconds** and take the **median**, not the
   mean. A single reflection skews an average.
5. Write that median into `TX_POWER_1M`. Reflash.
6. **Verify**: the app should now report roughly 1.0 m at 1 m.

## Use raw RSSI, not accuracy

`CLBeacon.accuracy` is *derived from* the TX power you are trying to set.
Calibrating against it is circular. You need `beacon.rssi`, in dBm.

Two implementation details: `rssi` returns 0 when iOS cannot measure — discard
those samples. And ranging runs at about 1 Hz with wide scatter, so display a
rolling median over 10 readings, or you will be reading values that jump by
±10 dB with no idea which one to write down.

## Calibrate each unit separately

Two boards of the same model will differ by several dB. If you calibrate one
and copy the value to the other, the second will consistently appear further
away at equal distance — and a child searching for it will notice.

A gap larger than about 5 dB between two units suggests a hardware problem
rather than normal variation.

## What good looks like

After calibration, expect roughly:

| Real distance | Reported |
|---|---|
| 1 m | ~1 m |
| 3 m | 2–4 m |
| 10 m | anywhere from 5 to 20 m |

**This is normal.** The conversion is logarithmic: at close range a few dB are
a few centimetres, at 10 m the same few dB are several metres. An iBeacon is
not a distance measuring instrument. Beyond about 5 m the estimate is
indicative at best.

Calibrate for the 0–3 m band — the one where the search actually happens — and
accept that the rest is approximate. Longer distances are the GPS stage's job.

## Path loss in the field

The distance model assumes a path loss exponent. Free space is 2.0; dense
vegetation runs 2.5 to 3.5. If your app exposes this, measure it outdoors in
the terrain you will actually use: it tells you how noisy your proximity
feedback will be before you find out with children in tow.
