# Hardware

**English** · [Français](README.fr.md)

## Choosing a board

Any ESP32 with BLE will advertise correctly. What separates them is how much
external circuitry you have to add for battery operation.

| Board | Size (mm) | Charging | VBAT sense | u.FL antenna | External parts |
|---|---|---|---|---|---|
| M5Stack NanoC6 | 23.5 × 12 × 9.5 | no | no | no | 5 |
| XIAO ESP32C3 | 21 × 17.5 | yes | no | **only** u.FL | 3 |
| XIAO ESP32C6 | 21 × 17.5 | yes | no | ceramic + u.FL, software switch | 3 |
| Unexpected Maker TinyC6 | 35 × 17.8 × 4.3 | yes | **yes** | chosen at purchase | 1 |
| ESP32 classic | varies | no | no | no | best kept as the bench scanner |

If you are prototyping on USB only, none of this matters — pick whatever you
have. The differences appear the moment you add a battery.

**On antennas.** The XIAO ESP32C6 selects between its ceramic antenna and the
u.FL connector at runtime (GPIO3 low enables the RF switch, GPIO14 high selects
external). That keeps the choice open after the enclosure is printed. The
ESP32C3 has no on-board antenna at all: the supplied external antenna is
mandatory, and a u.FL connector that works loose degrades range silently
without an obvious failure.

## Power

A beacon only needs to run for the duration of a hunt — two or three hours.
Put a switch in series with the battery positive lead and quiescent current
stops being a design concern entirely: an open switch draws nothing, which no
firmware can beat.

Consequence worth knowing: with the switch open, the battery is disconnected
from the charger, so **you cannot charge with the switch off**.

### Option A — board with charging and VBAT sense (TinyC6)

```
Battery + ── Switch ── BAT
Battery − ─────────── GND
```

Three components. Charging, regulation and the divider are already on the PCB.

### Option B — board with charging only (XIAO C3/C6)

```
Battery + ── Switch ──┬── BAT+
                      │
                      R1 100k
                      ├────────── A0/A1  (ADC)
                      R2 100k     └─ C1 100nF ─ GND
                      │
                     GND
Battery − ─────────────── BAT−
```

Six components. Tap the divider **after** the switch, never before, or it
draws ~18 µA continuously and drains the cell over weeks while you believe
everything is off.

Place C1 physically against the board, not near the resistors. Its job is to
supply the charge the ADC sample-and-hold demands within microseconds; at the
end of a 15 cm wire, cable inductance cancels the effect.

### Option C — board with a 5 V-only Grove port (NanoC6)

```
Battery ── TP4056 ── Switch ──┬── MT3608 boost 5V ── Grove 5V
(with protection)             │
                              └── R1/R2/C1 divider ── Grove G1
                                                      Grove GND
```

Eight components. The boost exists only because the Grove port supplies 5 V:
a LiPo at 3.7 V cannot pass through an LDO and come out regulated at 3.3 V as
it discharges. This also means a noisier battery reading than option A or B,
for purely topological reasons.

**Grove wire colours:** black GND, red 5 V, white G1, yellow G2.

## Bill of materials — option B

| Ref | Part | Note |
|---|---|---|
| BT1 | 1S LiPo, 500–1000 mAh | JST PH 2.0 |
| SW1 | SPST slide switch | on the positive lead |
| R1, R2 | 100 kΩ 1% | 0.25 W |
| C1 | 100 nF ceramic | as close to the ADC pin as possible |

## Warnings

**Lithium cells.** An on-board charger is a LiPo charger, constant-voltage at
4.2 V. A LiFePO4 cell charges at 3.6 V — putting one on a LiPo charger
overcharges it. If you want USB-C charging, use LiPo.

**Set an MT3608 to 5.0 V before connecting it.** These modules ship with the
trimmer in an arbitrary position and can output over 20 V. Power it alone,
measure, adjust, then wire it to the board.

**Use a TP4056 module with protection.** Bare modules expose only B+/B− with no
low-voltage cutoff. A LiPo taken below 2.5 V is scrap.

**Temperature.** The NanoC6 is rated 0–40 °C. A dark sealed enclosure in direct
sun exceeds that easily. Print in a light filament, place caches in shade, and
treat a swelling LiPo in a closed box as a safety matter, not a longevity one.

## Enclosure notes

**NFC tags and metal do not mix.** An NTAG213 works by inductive coupling at
13.56 MHz. A LiPo pouch or a PCB ground plane directly behind it detunes the
antenna and often kills read range completely. Design an 8–10 mm air gap
between the tag and the electronics, on a face away from the cell, or use an
"on-metal" tag with a ferrite layer.

**Antenna keep-out.** Leave 10–15 mm clear around the module's antenna end,
1.5 mm maximum wall thickness in front of it, and no wiring crossing it. A
ceramic antenna is more sensitive to nearby dielectric than a PCB trace.

**Mounting height.** 2.4 GHz is absorbed by water, so by soil, wet rock and
vegetation. A beacon lying on the ground reads much weaker and its distance
estimate becomes erratic. Aim for 30–50 cm above ground, and calibrate at the
height you will actually use.
