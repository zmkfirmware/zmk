---
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

To build for your particular keyboard, the behavior varies slightly depending on if you are building for a keyboard with
an onboard MCU, or one that uses an MCU board addon.

### Keyboard (Shield) + MCU Board

ZMK treats keyboards that take an MCU addon board as [shields](https://docs.zephyrproject.org/2.5.0/guides/porting/shields.html), and treats the smaller MCU board as the true [board](https://docs.zephyrproject.org/2.5.0/guides/porting/board_porting.html)

Given the following:

- MCU Board: Proton-C
- Keyboard PCB: kyria_left
- Keymap: default

You can build ZMK with the following:

```sh
west build -b proton_c -- -DSHIELD=kyria_left
```

### Keyboard With Onboard MCU

Keyboards with onboard MCU chips are simply treated as the [board](https://docs.zephyrproject.org/2.5.0/guides/porting/board_porting.html) as far as Zephyr™ is concerned.

Given the following:

- Keyboard: Planck (rev6)
- Keymap: default

you can build ZMK with the following:

```sh
west build -b planck_rev6
```

### Pristine Building

When building for a new board and/or shield after having built one previously, you may need to enable the pristine build option. This option removes all existing files in the build directory before regenerating them, and can be enabled by adding either --pristine or -p to the command:

```sh
west build -p -b proton_c -- -DSHIELD=kyria_left
```

### Building For Split Keyboards

:::note
For split keyboards, you will have to build and flash each side separately the first time you install ZMK.
:::

By default, the `build` command outputs a single .uf2 file named `zmk.uf2` so building left and then right immediately after will overwrite your left firmware. In addition, you will need to pristine build each side to ensure the correct files are used. To avoid having to pristine build every time and separate the left and right build files, we recommend setting up separate build directories for each half. You can do this by using the `-d` parameter and first building left into `build/left`:

```
west build -d build/left -b nice_nano -- -DSHIELD=kyria_left
```

and then building right into `build/right`:

```
west build -d build/right -b nice_nano -- -DSHIELD=kyria_right
```

This produces `left` and `right` subfolders under the `build` directory and two separate .uf2 files. For future work on a specific half, use the `-d` parameter again to ensure you are building into the correct location.

:::tip
Build times can be significantly reduced after the initial build by omitting all build arguments except the build directory, e.g. `west build -d build/left`. The additional options and intermediate build outputs from your initial build are cached and reused for unchanged files.
:::

### Building from `zmk-config` Folder

Instead of building .uf2 files using the default keymap and config files, you can build directly from your [`zmk-config` folder](../user-setup.md#github-repo) by adding
`-DZMK_CONFIG="C:/the/absolute/path/config"` to your `west build` command. **Notice that this path should point to the folder labelled `config` within your `zmk-config` folder.**

For instance, building kyria firmware from a user `myUser`'s `zmk-config` folder on Windows 10 may look something like this:

```
west build -b nice_nano -- -DSHIELD=kyria_left -DZMK_CONFIG="C:/Users/myUser/Documents/Github/zmk-config/config"
```

:::caution
The above command must still be invoked from the `zmk/app` directory as noted above, rather than the config directory. Otherwise, you will encounter errors such as `ERROR: source directory "." does not contain a CMakeLists.txt; is this really what you want to build?`. Alternatively you can add the `-s /path/to/zmk/app` flag to your `west` command.
:::

In order to make your `zmk-config` folder available when building within the VSCode Remote Container, you need to create a docker volume named `zmk-config`
by binding it to the full path of your config directory. If you have run the VSCode Remote Container before, it is likely that docker has created this
volume automatically -- we need to delete the default volume before binding it to the correct path. Follow the following steps:

1. Stop the container by exiting VSCode. You can verify no container is running via the command `docker ps`.
1. Remove all the containers that are not running via the command `docker container prune`. We need to remove the ZMK container before we can delete the default `zmk-config` volume referenced by it. If you do not want to delete all the containers that are not running, you can find the id of the ZMK container and use `docker rm` to delete that one only.
1. Remove the default volume via the command `docker volume rm zmk-config`.

Then you can bind the `zmk-config` volume to the correct path pointing to your local [zmk-config](customization.md) folder:

```
docker volume create --driver local -o o=bind -o type=none -o \
    device="/full/path/to/your/zmk-config/" zmk-config
```

Now start VSCode and rebuild the container after being prompted. You should be able to see your zmk-config mounted to `/workspaces/zmk-config` inside the container. So you can build your custom firmware with `-DZMK_CONFIG="/workspaces/zmk-config/config"`.

## Flashing

The above build commands generate a UF2 file in `build/zephyr` (or
`build/left|right/zephyr` if you followed the instructions for splits) and is by
default named `zmk.uf2`. If your board supports USB Flashing Format (UF2), copy
that file onto the root of the USB mass storage device for your board. The
controller should flash your built firmware and automatically restart once
flashing is complete.

Alternatively, if your board supports flashing and you're not developing from
within a Dockerized environment, enable Device Firmware Upgrade (DFU) mode on
your board and run the following command to flash:

```
west flash
```
