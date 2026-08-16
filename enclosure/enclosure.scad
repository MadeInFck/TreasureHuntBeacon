// =====================================================================
//  Treasure Hunt - Beacon enclosure
//  https://github.com/MadeInFck/TreasureHuntBeacon
//
//  Sealed 3D-printed case for a LOLIN C3 Pico beacon.
//  No wall penetrations: the LED reads through a thinned window, the
//  switch and USB-C stay inside. The only seal is the lid joint.
//
//  RENDER: set part = "body" or "lid" below, F6, export STL.
//  Print both with the open face down, no supports.
//
//  Sized for a 4000 mAh cell measured at 65 x 40 x 12 mm.
// =====================================================================

part = "body";          // "body", "lid", or "both" for a preview

/* [Components] */
bat_x       = 65;       // battery length
bat_y       = 40;       // battery width
bat_z       = 12;       // battery thickness -- measured with calipers
board_x     = 25.4;     // LOLIN C3 Pico
board_y     = 25.4;
board_z     = 6;        // board + tallest component
clearance   = 2;        // around the battery

/* [Shell] */
wall        = 3;        // side and floor thickness
lid_t       = 3;        // lid plate thickness
rim         = 7;        // sealing rim width, holds the groove
corner_r    = 4;        // outer corner radius

/* [Gasket] */
groove_in   = 3.0;      // groove inner edge, from cavity wall
groove_w    = 2.5;      // groove width -- fill with neutral silicone
groove_d    = 1.8;      // groove depth

/* [Fasteners] */
boss_d      = 9;        // corner boss diameter
insert_d    = 4.2;      // M3 heat-set insert hole
insert_h    = 6;        // insert depth
screw_d     = 3.4;      // M3 clearance in the lid
head_d      = 6.4;      // socket head counterbore
head_h      = 3.2;

/* [Features] */
window_d    = 12;       // LED window diameter
window_t    = 0.6;      // remaining wall -- diffuses a WS2812 nicely
window_off  = 22;       // window centre, X from middle
nfc_d       = 27;       // NFC tag pocket (NTAG213 25 mm sticker)
nfc_t       = 0.8;      // pocket depth
nfc_off     = -22;      // tag centre, X from middle -- opposite the window
nfc_inside  = false;    // false: pocket on the outer face, tag visible and
                        //        replaceable, but exposed to rain, UV and
                        //        curious children.
                        // true:  pocket on the inner face, tag protected and
                        //        unpeelable; 13.56 MHz reads through PLA.
                        //        An engraved ring marks the tap point.
                        // Either way the LID is the right face: it is the
                        // only one with real clearance to the cell. A side
                        // or underside tag would sit 3 to 5 mm from the
                        // pouch foil and be detuned.
mark_d      = 24;       // engraved ring on the outer face, shows where to tap
mark_w      = 1.6;
mark_t      = 0.5;
lip_h       = 2;        // lid spigot, aligns the parts
lip_gap     = 0.35;     // print clearance on the spigot

/* [Derived] */
cav_x       = bat_x + 2*clearance;
cav_y       = bat_y + 2*clearance;
stand_h     = bat_z + 3;                      // board sits above the cell
cav_z       = stand_h + 4 + board_z + 4;      // + LED headroom
out_x       = cav_x + 2*rim;
out_y       = cav_y + 2*rim;
boss_x      = cav_x/2 + rim + boss_d/2;
boss_y      = cav_y/2 + rim + boss_d/2;
env_x       = 2*(boss_x + boss_d/2);
env_y       = 2*(boss_y + boss_d/2);

$fn = 64;

// ---------------------------------------------------------------------
module rrect(x, y, z, r) {
  hull() for (i = [-1, 1], j = [-1, 1])
    translate([i*(x/2 - r), j*(y/2 - r), 0])
      cylinder(h = z, r = r);
}

// Each boss is hulled back to the shell's rounded corner, otherwise the
// corner radius leaves it floating in space, tangent at best.
module bosses(h) {
  for (i = [-1, 1], j = [-1, 1])
    hull() {
      translate([i*boss_x, j*boss_y, 0]) cylinder(h = h, d = boss_d);
      translate([i*(out_x/2 - corner_r), j*(out_y/2 - corner_r), 0])
        cylinder(h = h, r = corner_r);
    }
}

module groove_cut() {
  difference() {
    rrect(cav_x + 2*(groove_in + groove_w), cav_y + 2*(groove_in + groove_w),
          groove_d + 1, corner_r);
    translate([0, 0, -0.5])
      rrect(cav_x + 2*groove_in, cav_y + 2*groove_in,
            groove_d + 2, corner_r);
  }
}

// Tray the board drops into. Avoids guessing the board's hole pattern:
// the lid presses it down through a foam pad.
module board_tray() {
  bx = board_x + 0.6;
  by = board_y + 0.6;
  translate([cav_x/2 - bx/2 - 4, 0, wall]) {
    for (i = [-1, 1], j = [-1, 1])
      translate([i*(bx/2 - 2), j*(by/2 - 2), 0]) cylinder(h = stand_h, d = 5);
    translate([0, 0, stand_h])
      difference() {
        rrect(bx + 4, by + 4, 3, 2);
        translate([0, 0, 1]) rrect(bx, by, 3, 1);
      }
  }
}

// Low ribs that stop the cell sliding without gripping it.
module battery_stops() {
  for (i = [-1, 1])
    translate([i*(bat_x/2 + 1), 0, wall])
      cube([2, bat_y*0.6, bat_z*0.7], center = true);
}

// ---------------------------------------------------------------------
module body() {
  h = wall + cav_z;
  difference() {
    union() {
      rrect(out_x, out_y, h, corner_r);
      bosses(h);
    }
    translate([0, 0, wall]) rrect(cav_x, cav_y, cav_z + 1, 3);
    translate([0, 0, h - groove_d]) groove_cut();
    for (i = [-1, 1], j = [-1, 1])
      translate([i*boss_x, j*boss_y, h - insert_h])
        cylinder(h = insert_h + 1, d = insert_d);
  }
  translate([0, 0, wall - 0.01]) board_tray();
  battery_stops();
}

// ---------------------------------------------------------------------
module lid() {
  difference() {
    union() {
      rrect(out_x, out_y, lid_t, corner_r);
      bosses(lid_t);
      translate([0, 0, -lip_h])
        rrect(cav_x - 2*lip_gap, cav_y - 2*lip_gap, lip_h + 0.01, 3);
    }
    // LED window: a blind pocket from inside, window_t of wall left
    translate([window_off, 0, -lip_h - 0.01])
      cylinder(h = lip_h + lid_t - window_t + 0.01, d = window_d);
    // NFC tag pocket, on whichever face nfc_inside selects
    if (nfc_inside) {
      translate([nfc_off, 0, -lip_h - 0.01])
        cylinder(h = nfc_t + 0.01, d = nfc_d);
      // Engraved ring so the tap point stays findable
      translate([nfc_off, 0, lid_t - mark_t])
        difference() {
          cylinder(h = mark_t + 1, d = mark_d);
          translate([0, 0, -0.5]) cylinder(h = mark_t + 2, d = mark_d - 2*mark_w);
        }
    } else {
      translate([nfc_off, 0, lid_t - nfc_t])
        cylinder(h = nfc_t + 1, d = nfc_d);
    }
    // screws
    for (i = [-1, 1], j = [-1, 1]) translate([i*boss_x, j*boss_y, -1]) {
      cylinder(h = lid_t + 2, d = screw_d);
      translate([0, 0, lid_t - head_h + 1]) cylinder(h = head_h + 1, d = head_d);
    }
  }
}

// ---------------------------------------------------------------------
if (part == "body") body();
else if (part == "lid") translate([0, 0, lid_t]) rotate([180, 0, 0]) lid();
else {
  body();
  translate([0, 0, wall + cav_z + 12]) lid();
}

echo(str("Envelope: ", env_x, " x ", env_y, " x ", wall + cav_z + lid_t, " mm"));
echo(str("Cavity:   ", cav_x, " x ", cav_y, " x ", cav_z, " mm"));
