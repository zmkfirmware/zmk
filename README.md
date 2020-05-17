# Zephyr Mechanical Keyboard (ZMK) Firmware

This project is a complete work in progress, with absolutely nothing functioning yet. The goal is to explore a new MK firmware
with a less restritive license and better BLE support, built on top of the [Zephyr Project](https://www.zephyrproject.org/)

## TODO

- Debouncing in the kscan driver itself? Only some GPIO drivers in Zephyr support it "natively"
- Document boards/shields/keymaps usage.
- Custom keymap code via `zephyr_library()`>
- Move most Kconfig setings to the board/keymap defconfigs and out of the toplevel `prj.conf` file.
- Merge the Kscan GPIO driver upstream, or integrate it locally, to avoid use of Zephyr branch.
- BLE SC by typing in the # prompted on the host.
  - Store the connection being authenticated.
  - Hook into endpoint flow to detect keypresses and store/send them to the connection once 6 are typed.
- Display support, including displaying BLE SC auth numbers for "numeric comparison" mode if we have the screen.
- Fix BT settings to work w/ Zephyr. Do we really need them?
- Tests?
- Update the kscan GPIO driver to use interrupts.
  - Try disabling callbacks for the read pins temporarily when scanning, then re-enabling them.

# Missing Features

- Mod Tap
- One Shot
- Shell over BLE?
- Split support
  - custom kscan driver? that recieves events from other side over serial/BLE notifications?

## Long Term

- Tool to convert keymap `info.json` files into a DTS keymap file?
- Firmware build service?
