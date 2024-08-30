---
title: Clean Room Implementation
sidebar_label: Clean Room
---

:::danger

Anyone wanting to contribute code to ZMK _MUST_ read this, and adhere to the steps outlines in order to not violate any licenses/copyright of other projects

:::

ZMK Firmware is a [clean room design](https://en.wikipedia.org/wiki/Clean_room_design) keyboard firmware, that
borrows/implements a lot of the features found in popular keyboard firmwares projects like [QMK](https://qmk.fm)
and [TMK](https://github.com/tmk/tmk_keyboard). However, in order for ZMK to use the MIT, it _must_ not
incorporate any of the GPL licensed code from those projects.

In order to achieve this, all code for ZMK has been implemented completely fresh, _without_ referencing, copying,
or duplicating any of the GPL code found in those other projects, even though they are open source software.

## Contributor Requirements

Contributors to ZMK must adhere to the following standard.

- Implementations of features for ZMK _MUST NOT_ reuse any existing code from any projects not licensed with the MIT license.
- Contributors _MUST NOT_ study or refer to any GPL licensed source code while working on ZMK.
- Contributors _MAY_ read the documentation from other GPL licensed projects, to gain a broad understanding of the behavior of certain features in order to implement equivalent features for ZMK.
- Contributors _MAY_ refer to the [QMK Configurator](https://config.qmk.fm/) to inspect existing layouts/keymaps for
  keyboards, and re-implement them for ZMK.
