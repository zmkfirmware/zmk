# Zephyr™ Mechanical Keyboard (ZMK) Firmware

[![Build](https://github.com/zmkfirmware/zmk/workflows/Build/badge.svg)](https://github.com/zmkfirmware/zmk/actions)

[![Netlify Status](https://api.netlify.com/api/v1/badges/942d61a9-87c0-4c23-9b51-f5ed0bce495d/deploy-status)](https://app.netlify.com/sites/zmk/deploys)

This project is a complete work in progress, with absolutely nothing functioning yet. The goal is to explore a new MK firmware
with a less restritive license and better BLE support, built on top of the [Zephyr™ Project](https://www.zephyrproject.org/)

Basic WIP website to learn more: https://zmk.netlify.app/

## TODO

- Document boards/shields/keymaps usage.
- Display support, including displaying BLE SC auth numbers for "numeric comparison" mode if we have the screen.
- Fix BT settings to work w/ Zephyr. Do we really need them?
- Tests?

# Missing Features

- Layer Tap
- One Shot
- Combos
- Tap Dance
- Shell over BLE?
- Split support
  - custom kscan driver? that recieves events from other side over BLE notifications?
  - Need to figure out how to do "custom" GATT services. Custom UUID?
  - Do we need to send any state updates over from primary to secondary side?
- Encoders
- Battery reporting.
- Low power mode, with wakeup from interrupt?
- Better keymaps to avoid needing extra KC_NO for locations in the matrix that don't even exist!

## Long Term

- Tool to convert keymap `info.json` files into a DTS keymap file?
- Firmware build service?
