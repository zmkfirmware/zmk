# Zephyr™ Mechanical Keyboard (ZMK) Firmware

[![Discord](https://img.shields.io/discord/719497620560543766)](https://zmk.dev/community/discord/invite)
[![Build](https://github.com/afayge/zmk/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/afayge/zmk/actions/workflows/build.yml)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](CODE_OF_CONDUCT.md)

[ZMK Firmware](https://zmk.dev/) is an open source ([MIT](LICENSE)) keyboard firmware built on the [Zephyr™ Project](https://www.zephyrproject.org/) Real Time Operating System (RTOS). ZMK's goal is to provide a modern, wireless, and powerful firmware free of licensing issues.

## What this fork adds to ZMK

This fork adds per-key lighting, Studio controls, and support for BMK keyboards. The comparison uses [upstream ZMK at bb2d711d1](https://github.com/zmkfirmware/zmk/tree/bb2d711d192f3cca83dff69fe1fc5119418a0102), the upstream revision merged into this branch.

| Feature | What you can do |
| --- | --- |
| LED Matrix | Map keys to LEDs, choose from 16 main effect renderers, set layer colors, and give individual lighting zones their own presets. Use selected LEDs to show battery level, USB or Bluetooth state, and host lock indicators. [Corne54 configuration guide](docs/docs/development/hardware-integration/lighting/led-matrix.mdx) |
| Mixed lighting hardware | Drive an addressable RGB strip or a Zephyr LED device directly. Use `led_composite` to map multiple LED controllers, single-color LEDs, and strips into one logical LED space. [Channel mapping and power reference](docs/docs/led-matrix-led-composite-reference.mdx) |
| Studio lighting and encoder controls | A compatible client can list lighting presets, edit zones, and assign clockwise and counterclockwise encoder actions per layer. Persistent lighting edits require settings and the LED Matrix editor. Encoder editing requires Studio and sensors defined in the keymap. [Firmware settings](docs/docs/config/studio.md) · [Companion Studio client](https://github.com/afayge/zmk-studio) |
| Trackball and pointer speed | Connect a BlackBerry trackball, tune acceleration, switch speed profiles, and configure its push button and four PWM LEDs. [Wiring and tuning guide (Chinese)](docs/docs/hardware-integration/blackberry-trackball.mdx) |
| BMK boards and displays | Build firmware for the Corne54 family and BMK Numpad. Supported status screens show Num Lock, Caps Lock, and Scroll Lock. The Numpad can also evaluate an expression and type its result into the active USB or Bluetooth host. [Board definitions](app/boards/bmk) · [Calculator key bindings (Chinese)](app/boards/bmk/numpad/calculator/README.md) |

## Upstream ZMK resources

Check out the website to learn more: https://zmk.dev/.

You can also come join our [ZMK Discord Server](https://zmk.dev/community/discord/invite).

To review features, check out the [feature overview](https://zmk.dev/docs/). ZMK is under active development, and new features are listed with the [enhancement label](https://github.com/zmkfirmware/zmk/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement) in GitHub. Please feel free to add 👍 to the issue description of any requests to upvote the feature.
