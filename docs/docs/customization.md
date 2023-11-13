---
title: Customizing ZMK/zmk-config folders
sidebar_label: Customizing ZMK
---

After verifying you can successfully flash the default firmware, you will probably want to begin customizing your keymap and other keyboard options.
[In the initial setup tutorial](user-setup.md), you created a Github repository called `zmk-config`. This repository is a discrete filesystem which works
with the main `zmk` firmware repository to build your desired firmware. The main advantage of a discrete configuration folder is ensuring that the
working components of ZMK are kept separate from your personal keyboard settings, reducing the amount of file manipulation in the configuration process.
This makes flashing ZMK to your keyboard much easier, especially because you don't need to keep an up-to-date copy of zmk on your computer at all times.

By default, the `zmk-config` folder should contain two files:

- `<keyboard>.conf`
- `<keyboard>.keymap`

However, your config folder can also be modified to include a `boards/` directory for keymaps and configurations for multiple boards/shields
outside of the default keyboard setting definitions.

## Configuration Changes

The setup script creates a `config/<keyboard>.conf` file that allows you to add additional configuration options to
control what features and options are built into your firmware. Opening that file with your text editor will allow you to see the
various config settings that can be commented/uncommented to modify how your firmware is built.

Refer to the [Configuration](/docs/config) documentation for more details on this file.

## Keymap

Once you have the basic user config completed, you can find the keymap file in `config/<keyboard>.keymap` and customize from there.
Refer to the [Keymap](features/keymaps.md) documentation to learn more.

## Publishing

After making any changes you want, you should commit the changes and then push them to GitHub. That will trigger a new
GitHub Actions job to build your firmware which you can download once it completes.

:::note
If you need to, a review of [Learn The Basics Of Git In Under 10 Minutes](https://www.freecodecamp.org/news/learn-the-basics-of-git-in-under-10-minutes-da548267cc91/) will help you get these steps right.
:::

:::note
It is also possible to build firmware locally on your computer by following the [toolchain setup](development/setup.md) and
[building instructions](development/build-flash.md), which includes pointers to
[building using your `zmk-config` folder](development/build-flash.md#building-from-zmk-config-folder).
:::

## Flashing Your Changes

For normal keyboards, follow the same flashing instructions as before to flash your updated firmware.

For split keyboards, only the central (left) side will need to be reflashed if you are just updating your keymap.
More troubleshooting information for split keyboards can be found [here](troubleshooting.md#split-keyboard-halves-unable-to-pair).

## Building Additional Keyboards

You can build additional keyboards with GitHub actions by appending them to `build.yml` in your `zmk-config` folder. For instance assume that we have set up a Corne shield with nice!nano during [initial setup](user-setup.md) and we want to add a Lily58 shield with nice!nano v2. The following is an example `build.yaml` file that would accomplish that:

```yaml
include:
  - board: nice_nano
    shield: corne_left
  - board: nice_nano
    shield: corne_right
  - board: nice_nano_v2
    shield: lily58_left
  - board: nice_nano_v2
    shield: lily58_right
```

In addition to updating `build.yaml`, Lily58's shield files should also be added into the `config` sub-folder inside `zmk-config` together with your Corne files, e.g.:

```
corne.conf
corne.keymap
lily58.conf
lily58.keymap
```
