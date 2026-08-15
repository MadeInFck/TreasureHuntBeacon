---
name: Beacon not detected
about: The app or a scanner does not see the beacon
title: 'Not detected: '
labels: 'not detected'
---

Please read [docs/troubleshooting.md](../../docs/troubleshooting.md) first —
it covers the six most common causes in order of likelihood.

**Note:** nRF Connect on iPhone showing nothing is expected behaviour, not a
bug. iOS filters Apple manufacturer data out of CoreBluetooth. Use a Mac, an
Android phone, or `firmware/scanner` to inspect raw frames.

## Serial output

Paste the full startup dump (115200 baud), including the `Advertising:` line:

```

```

## Setup

- Board:
- ESP32 core version:
- Arduino IDE version:
- STEP_ID / HUNT_ID:
- Detected by another scanner (Mac / Android / ESP32)?

## What you observe
