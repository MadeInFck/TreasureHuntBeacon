# Enclosure

**English** · [Français](README.fr.md)

Parametric sealed case for a beacon, in OpenSCAD. Open `enclosure.scad`, set
`part`, press F6, export STL. Ready-made STLs are in `stl/`.

## The design idea

The case has **no wall penetrations**. The LED reads through a thinned window,
the switch and USB-C stay inside, and you open the box to charge. The lid joint
is therefore the only thing that needs to seal.

A hole for a status LED or a charging port is what usually makes a printed
enclosure leak. Not making one removes the problem instead of solving it.

## Dimensions

Defaults are sized for a 4000 mAh cell measured at 65 × 40 × 12 mm and a LOLIN
C3 Pico.

| | |
|---|---|
| Envelope | 101 × 76 × 35 mm |
| Cavity | 69 × 44 × 29 mm |

That is a large box, and the battery is why: the cell fixes the cavity, the
sealing rim adds 14 mm, the screw ears another 18. A 1000 mAh cell at
35 × 25 mm brings it down to roughly 70 × 60 mm — change `bat_x`, `bat_y` and
`bat_z` and everything else recalculates.

**Measure your cell with calipers, including the JST connector and its heat
shrink.** That bundle is often 2–3 mm taller than the pouch and it is the real
high point.

## Features

- Gasket groove, 2.5 × 1.8 mm, running around the rim
- Four M3 heat-set inserts in corner bosses, hulled into the shell
- Counterbored screw holes in the lid
- LED window: a blind pocket taken from the inside, leaving 0.6 mm of wall
- NFC tag pocket, on the outer face by default (`nfc_inside = true` moves it
  inside and adds an engraved ring to mark the tap point)
- Board cradle the C3 Pico drops into — no screws, so no need to guess the
  board's hole pattern. A foam pad under the lid holds it down.
- Low ribs stop the cell sliding

## Gasket

Cast it in place rather than buying an O-ring. Fill the groove with **neutral
cure** silicone (not acetic — acetic acid attacks copper and solder joints),
smear the lid face with petroleum jelly as a release agent, close, and leave it
24 hours. You get a gasket moulded to your own print tolerances, which is far
more forgiving than an O-ring on an FDM groove where a few tenths of drift
cause a leak.

## Printing

Both parts open face down, no supports. The lid prints on its outer face with
the spigot pointing up.

Watertightness comes from layer adhesion, not from the gasket:

- **5 perimeters** minimum, 0.15 mm layers
- Extrusion temperature at the top of the recommended range, reduced cooling
- White or natural filament — a dark box in direct sun exceeds what PLA
  tolerates, and white diffuses the LED far better than clear filament, which
  prints milky and streaked

PETG is better than PLA if you have it: it softens near 80 °C instead of 60.

## Before printing the full part

Print a **test rim** first — just the top 5 mm of the body. Ten minutes, and it
tells you whether `lip_gap` (0.35 mm by default) suits your machine and whether
the groove came out clean. Too tight and the lid will not seat; too loose and
it wanders while you tighten the screws.

## Still open

The switch has no dedicated housing yet. There is 12 mm of clear height between
the board and the lid, but it currently floats on its wires. Add a cradle once
you have chosen the part.

The ceramic antenna orientation depends on which edge of the C3 Pico carries
it. The cradle places the board against the right-hand wall — check your board
and flip the sign in `board_tray()` if the antenna ends up pointing inward at
the battery.

## Thermal breathing

A fully sealed box going from 20 to 55 °C in the sun sees its internal pressure
rise about 12 %. Cooling overnight creates a partial vacuum that draws water
past the gasket. This is the classic failure mode of sealed outdoor cases.

Irrelevant for a cache placed for a few hours. If you ever leave one out for
days, add a Gore-Tex vent patch over a small hole on the inside face.
