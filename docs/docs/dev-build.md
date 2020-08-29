---
id: dev-build-flash
title: Building and Flashing
sidebar_label: Building and Flashing
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

export const OsTabs = (props) => (<Tabs
groupId="operating-systems"
defaultValue="debian"
values={[
{label: 'Debian/Ubuntu', value: 'debian'},
{label: 'Raspberry OS', value: 'raspberryos'},
{label: 'Fedora', value: 'fedora'},
{label: 'Windows', value: 'win'},
{label: 'macOS', value: 'mac'},
]
}>{props.children}</Tabs>);

## Building

From here on, building and flashing ZMK should all be done from the `app/` subdirectory of the ZMK checkout:

```sh
cd app
```

To build for your particular keyboard, the behaviour varies slightly depending on if you are building for a keyboard with
an onboard MCU, or one that uses an MCU board addon.

### Keyboard (Shield) + MCU Board

ZMK treats keyboards that take an MCU addon board as [shields](https://docs.zephyrproject.org/2.3.0/guides/porting/shields.html), and treats the smaller MCU board as the true [board](https://docs.zephyrproject.org/2.3.0/guides/porting/board_porting.html)

Given the following:

- MCU Board: Proton-C
- Keyboard PCB: kyria_left
- Keymap: default

You can build ZMK with the following:

```sh
west build -b proton_c -- -DSHIELD=kyria_left -DKEYMAP=default
```

### Keyboard With Onboard MCU

Keyboards with onboard MCU chips are simply treated as the [board](https://docs.zephyrproject.org/2.3.0/guides/porting/board_porting.html) as far as Zephyr™ is concerned.

Given the following:

- Keyboard: Planck (rev6)
- Keymap: default

you can build ZMK with the following:

```sh
west build -b planck_rev6 -- -DKEYMAP=default
```

### Pristine Building
When building for a new board and/or shield after having built one previously, you may need to enable the pristine build option. This option removes all existing files in the build directory and regenerates them, and can be enabled by adding either --pristine or -p to the command:

```sh
west build -p -b proton_c -- -DSHIELD=kyria_left -DKEYMAP=default
```

## Flashing

Once built, the previously supplied parameters will be remembered so you can run the following to flash your
board with it in bootloader mode:

```
west flash
```

For boards that have drag and drop .uf2 flashing capability, the .uf2 file to flash can be found in `build\zephyr` and is by default named `zmk.uf2`.

:::note
For split keyboards, you will have to build and flash each side separately the first time you install ZMK. By default the `build` command outputs a single .uf2 file named `zmk.uf2`, so you will have to
1. Build the left side with the `build` command provided above
2. Flash `zmk.uf2` to the left side
3. Replace DSHIELD with the right side (for the above example, this would be `kyria_right`) and build again
4. Flash `zmk.uf2` the right side
:::
