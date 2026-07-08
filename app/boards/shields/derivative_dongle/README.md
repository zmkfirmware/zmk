# Derivative Dongle (Phase 0)

Split "dongle" setup for the derivative keyboard: a Seeed XIAO nRF52840 acts as
the BLE **central** and USB HID endpoint, and the derivative keyboard runs as a
BLE **split peripheral**.

This is Phase 0 of the dongle work — two separate images, chosen at flash time.
It proves the split link before any single-image / boot-time role switching
(Phase 1) is attempted.

## Why the keyboard normally can't do this

In ZMK, central vs peripheral is a **compile-time** decision. `app/CMakeLists.txt`
compiles the whole keymap / HID / behavior / endpoint stack only for
central-or-standalone builds; a peripheral image doesn't even contain
`keymap.c` or `hog.c`. So "dongle mode" is a different binary, not a runtime flag.
Both roles are BLE *peripherals* though (host↔keyboard HID, or dongle↔keyboard
split), so the keyboard never needs `BT_CENTRAL` — which is what makes a future
single-image build tractable.

## Builds (see `app/core-coverage.yml`)

| Image | Target | Role |
| --- | --- | --- |
| Keyboard | `derivative-rev-a/nrf52840/zmk` + `derivative_peripheral` shield | split peripheral |
| Dongle | `xiao_ble/nrf52840/zmk` + `derivative_dongle` shield | split central + HID |

The dongle mirrors the keyboard's 48-key physical layout and matrix transform
verbatim (`derivative_dongle.overlay`) so incoming key positions map to the same
bindings. The keymap itself is shared — both images `#include`
`boards/hanks/derivative-rev-a/derivative_keymap.dtsi`.

## Flashing / pairing

1. Flash the **keyboard** with the `derivative_peripheral` image.
2. Flash the **XIAO** with the `derivative_dongle` image.
3. Power both. They auto-pair over BLE (clear bonds with a settings reset on
   each side if they don't).
4. Plug the XIAO into the host — it enumerates as the "derivative" keyboard.

To go back to standalone, reflash the keyboard with the plain
`derivative-rev-a/nrf52840/zmk` image.

## Known limitations (Phase 0)

- **RGB / LED map:** the physical strip is on the keyboard (peripheral), but the
  keymap runs on the dongle (central). `ZMK_LED_MAP` is intentionally *off* on
  the dongle, and the shared keymap's `&led_map` bindings compile to `&trans`
  there. Keymap-driven LED changes therefore do **not** reach the strip in
  dongle mode yet. The strip still runs its default effect. Relaying LED/underglow
  state central→peripheral is future work.
- **No boot-time switching yet:** role is chosen by which image you flash.
  Single-image boot-time selection is Phase 1.
- **Battery:** the dongle is USB-powered; the keyboard reports its own battery
  over the split connection (standard ZMK behavior).
