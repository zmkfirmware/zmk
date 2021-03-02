---
title: Customizing ZMK/`zmk-config` folders
sidebar_label: Customizing ZMK
---

After verifying you can successfully flash the default firmware, you will probably want to begin customizing your keymap and other keyboard options.
[In the initial setup tutorial](user-setup), you created a Github repository called `zmk-config`. This repository is a discrete filesystem which works
with the main `zmk` firmware repository to build your desired firmware. The main advantage of a discrete configuration folder is ensuring that the
working components of ZMK are kept separate from your personal keyboard settings, reducing the amount of file manipulation in the configuration process.
This makes flashing ZMK to your keyboard much easier, especially because you don't need to keep an up-to-date copy of zmk on your computer at all times.

On default `zmk-config` folder should contain two files:

- `<shield>.conf`
- `<shield>`.keymap

However, your config folder can also be modified to include a `boards/` directory for keymaps and configurations for multiple boards/shields
outside of the default keyboard setting definitions.

## Configuration Changes

The setup script creates a `config/<shield>.conf` file that allows you to add additional configuration options to
control what features and options are built into your firmware. Opening that file with your text editor will allow you to see the
various config settings that can be commented/uncommented to modify how your firmware is built.

## Keymap

Once you have the basic user config completed, you can find the keymap file in `config/<shield>.keymap` and customize from there.
Refer to the [Keymap](/docs/features/keymaps) documentation to learn more.

## Publishing

After making any changes you want, you should commit the changes and then push them to GitHub. That will trigger a new
GitHub Actions job to build your firmware which you can download once it completes.

:::note
If you need to, a review of [Learn The Basics Of Git In Under 10 Minutes](https://www.freecodecamp.org/news/learn-the-basics-of-git-in-under-10-minutes-da548267cc91/) will help you get these steps right.
:::

## Building from a local `zmk` fork using `zmk-config`

[As outlined here](development/build-flash), firmware comes in the form of .uf2 files, which can be built locally using the command `west build`. Normally,
`west build` will default to using the in-tree .keymap and .conf files found in your local copy of the `zmk` repository. However, you can append the command, `-DZMK_CONFIG="C:/the/absolute/path/config"` to `west build` in order to use the contents of your `zmk-config` folder instead of the
default keyboard settings.
**Notice that this path should point to the folder labelled `config` within your `zmk-config` folder.**

For instance, building kyria firmware from a user `myUser`'s `zmk-config` folder on Windows 10 may look something like this:

```
west build -b nice_nano -- -DSHIELD=kyria_left -DZMK_CONFIG="C:/Users/myUser/Documents/Github/zmk-config/config"
```

## Flashing Your Changes

For normal keyboards, follow the same flashing instructions as before to flash your updated firmware.

For split keyboards, only the central (left) side will need to be reflashed if you are just updating your keymap.
More troubleshooting information for split keyboards can be found [here](troubleshooting#split-keyboard-halves-unable-to-pair).
