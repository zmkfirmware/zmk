# Zephyr™ Mechanical Keyboard (ZMK) Firmware

[![Build](https://github.com/zmkfirmware/zmk/workflows/Build/badge.svg)](https://github.com/zmkfirmware/zmk/actions)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](CODE_OF_CONDUCT.md)

This project is a complete work in progress, with absolutely nothing functioning yet. The goal is to explore a new MK firmware
with a less restritive license and better BLE support, built on top of the [Zephyr™ Project](https://www.zephyrproject.org/)

Check out the website to learn more: https://zmkfirmware.dev/

You can also come join our [ZMK Discord Server](https://zmkfirmware.dev/community/discord/invite)

## TODO

- Document boards/shields/keymaps usage.
- Display support, including displaying BLE SC auth numbers for "numeric comparison" mode if we have the screen.
- Fix BT settings to work w/ Zephyr. Do we really need them?
- Tests

# Missing Features

- Layer Tap
- One Shot
- Combos
- Tap Dance
- Shell over BLE/USB
- Split support
- Encoders
- Battery reporting.
- Low power mode, with wakeup from interrupt(s)

## Long Term

- Tool to convert keymap `info.json` files into a DTS keymap file?
- Firmware build service?
