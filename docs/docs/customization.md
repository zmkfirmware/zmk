---
id: customization
title: Customizing ZMK
sidebar_label: Customizing ZMK
---

After verifying you can successfully flash the default firmware, you will probably want to begin customizing your keymap and other keyboard options.

## Configuration Changes

The setup script creates a `config/<shield>.conf` file that allows you to add additional configuration options to
control what features and options are built into your firmware. Opening that file with your text editor will allow you to see the
various config settings that can be commented/uncommented to modify how your firmware is built.

## Keymap

Once you have the basic user config completed, you can find the keymap file in `config/<shield>.keymap` and customize from there.
Refer to the [Keymap](/docs/feature/keymaps) documentation to learn more.

## Publishing

After making any changes you want, you should commit the changes and then push them to GitHub. That will trigger a new
GitHub Actions job to build your firmware which you can download once it completes.

:::note
If you need to, a review of [Learn The Basics Of Git In Under 10 Minutes](https://www.freecodecamp.org/news/learn-the-basics-of-git-in-under-10-minutes-da548267cc91/) will help you get these steps right.
:::

## Flashing Your Changes

For normal keyboards, follow the same flashing instructions as before to flash your updated firmware.

For split keyboards, only the central (left) side will need to be reflashed if you are just updating your keymap.
