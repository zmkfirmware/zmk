---
title: Installing ZMK
sidebar_label: Installing ZMK
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

Unlike other keyboard firmwares, ZMK Firmware has been built from the ground up to allow users to manage
their own keyboard configurations, including keymaps, specific hardware details, etc. all outside of the
core ZMK Firmware source repository.

In addition to this, most users will not need to install any complicated toolchains or tools to build ZMK. GitHub Actions is used instead to automatically build the user's configured firmware for them.

## Summary

The following steps can be taken to obtain an installable firmware image for your device, without the need
to install any compiler or specialized toolchain. This is possible by leveraging [GitHub Actions](https://github.com/features/actions)
to build your firmware for you in the cloud, which you can then download and flash to your device.

Following the steps in this guide, you will:

1. Create a new repository in GitHub that will contain your user config.
1. Download and run a small shell script that will automate most of the setup. The script will:
   1. Prompt you for which board (e.g. nice!nano) and shield (e.g. Lily58 or Kyria) you want to create a configuration for.
   1. Prompt you for your GitHub username and repo where the config should be pushed.
   1. Use `git` to clone a template repository that will be the basis for your user's config repository.
   1. Use the given board and shield to update the included GitHub Action to build for the correct hardware.
   1. Commit the initial version.
   1. (If info was provided) Push the local repo to your GitHub repository.
1. Add your own keymap overlay (`keymap.overlay`) file to the repository, and change your keymap.
1. Update the configuration flags for your user config, to enable/disable any ZMK features you would like to include.
1. Commit and push the updates to trigger a new GitHub Action run.

## Prerequisites

The remainder of this guide assumes the following prerequisites:

1. You have an active, working [GitHub](https://github.com/) account.
1. You have installed and configured the [`git`](https://git-scm.com/) version control tool.
1. You have locally configured git to access your github account. If using [personal access tokens](https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/creating-a-personal-access-token), please be sure it was created with the "workflow" scope option selected.

:::note
If you need to, a quick read of [Learn The Basics Of Git In Under 10 Minutes](https://www.freecodecamp.org/news/learn-the-basics-of-git-in-under-10-minutes-da548267cc91/) will help you get started.
:::

## GitHub Repo

Before running the setup script, you will first need to create a new GitHub repository to host the config.

1. Navigate to [https://github.com/new](https://github.com/new)
1. When prompted for the repo name, enter `zmk-config`.
1. The repository can be public or private
1. Do **not** check any of the options to initialize the repository with a README or other files.
1. Click "Create repository"

## User Config Setup Script

To start the setup process, run the following from your command line prompt:

<Tabs
defaultValue="curl"
values={[
{label: 'Using curl', value: 'curl'},
{label: 'Using wget', value: 'wget'},
{label: 'Using PowerShell', value: 'PowerShell'},
]}>
<TabItem value="curl">

```
bash -c "$(curl -fsSL https://zmk.dev/setup.sh)"
```

</TabItem>
<TabItem value="wget">

```
bash -c "$(wget https://zmk.dev/setup.sh -O -)" '' --wget
```

</TabItem>
<TabItem value="PowerShell">

```
powershell -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://zmk.dev/setup.ps1'))"
```

</TabItem>
</Tabs>

### Keyboard Selection

When prompted, enter the number for the corresponding keyboard you would like to target:

```
Keyboard Selection:
 1) 2% Milk                   19) Ferris 0.2                37) Nibble
 2) A. Dux                    20) Fourier Rev. 1            38) nice!60
 3) BAT43                     21) Helix                     39) Osprette
 4) BDN9 Rev2                 22) Hummingbird               40) Pancake
 5) BFO-9000                  23) Iris                      41) Planck Rev6
 6) Boardsource 3x4 Macropad  24) etc...
Pick an keyboard:
```

:::note For a keyboard not in the included list:
If you are building firmware for a new keyboard that is not included in the built-in
list of keyboards, you can choose any keyboard from the list that is similar to yours (e.g. in terms of unibody/split and [onboard controller](hardware.mdx#onboard)/[composite](hardware.mdx#composite)) to generate the repository,
and edit / add necessary files. You can follow the [new shield guide](development/new-shield.md) if you are adding support for a composite keyboard.
:::

### MCU Board Selection

If the keyboard selected uses an onboard controller you will skip this step.
When prompted, enter the number for the corresponding MCU board you would like to target:

```
MCU Board Selection:
1) BlueMicro840 v1           5) nRF52840 M.2 Module      9) QMK Proton-C
2) Mikoto 5.20               6) nRFMicro 1.1 (flipped)  10) Seeeduino XIAO
3) nice!nano v1              7) nRFMicro 1.1/1.2        11) Seeeduino XIAO BLE
4) nice!nano v2              8) nRFMicro 1.3/1.4        12) Quit
Pick an MCU board:
```

### Keymap Customization

At the next prompt, you have an opportunity to decide if you want the stock keymap file copied in
for further customization:

```
Copy in the stock keymap for customization? [Yn]:
```

Hit `Enter` or type `yes`/`y` to accept this. If you want to keep the stock keymap, or write a keymap
from scratch, type in `no`/`n`.

### GitHub Details

In order to have your new configuration automatically pushed, and then built using GitHub Actions, enter
some information about your particular GitHub info:

```
GitHub Username (leave empty to skip GitHub repo creation): petejohanson
GitHub Repo Name: zmk-config
GitHub Repo: https://github.com/petejohanson/zmk-config.git
```

Only the GitHub username is required; if you are happy with the defaults offered in the square brackets, you can simply hit `Enter`.

:::note
If you are using SSH keys for git push, change GitHub Repo field to the SSH scheme, e.g. `git@github.com:petejohanson/zmk-config.git`.
:::

### Confirming Selections

The setup script will confirm all of your selections one last time, before performing the setup:

```
Preparing a user config for:
* MCU Board: nice_nano
* Shield: kyria
* GitHub Repo To Push (please create this in GH first!): https://github.com/petejohanson/zmk-config.git

Continue? [Yn]:
```

After hitting `Enter` or typing `y`, the script will create an initial config in a directory named after the repo name,
update the GitHub Action YAML file, commit the initial version, and then push to your repo.

:::info

If you used the default GitHub repo URL using the `https://` scheme, you may be prompted for your username + password in order to
push the initial commit.

:::

## Installing The Firmware

### Download The Archive

Once the setup script is complete and the new user config repository has been pushed, GitHub will automatically run the action
to build your keyboard firmware files. You can view the actions by clicking on the "Actions" tab on your GitHub repository.

![](./assets/user-setup/github-actions-link.png)

Once you have loaded the Actions tab, select the top build from the list. Once you load it, the right side panel will include
a link to download the `firmware` upload:

![](./assets/user-setup/firmware-archive.png)

Once downloaded, extract the zip and you can verify it should contain one or more `uf2` files, which will be copied to
your keyboard.

### Flashing UF2 Files

To flash the firmware, first put your board into bootloader mode by double clicking the reset button (either on the MCU board itself,
or the one that is part of your keyboard). The controller should appear in your OS as a new USB storage device.

Once this happens, copy the correct UF2 file (e.g. left or right if working on a split), and paste it onto the root of that USB mass
storage device. Once the flash is complete, the controller should automatically restart, and load your newly flashed firmware. It is
recommended that you test your keyboard works over USB first to rule out hardware issues, before trying to connect to it wirelessly.

:::caution Split keyboards

For split keyboards, only the central half (typically the left side) will send keyboard outputs over USB or advertise to other devices
over bluetooth. Peripheral half will only send keystrokes to the central once they are paired and connected. For this reason it is
recommended to test the left half of a split keyboard first.

:::

## Wirelessly Connecting Your Keyboard

ZMK will automatically advertise itself as connectable if it is not currently connected to a device. You should be able to see your keyboard from the bluetooth scanning view of your computer or phone / tablet. It is reported by some users that the connections with Android / iOS devices are generally smoother than with laptops, so if you have trouble connecting, you could try to connect from your phone or tablet first to eliminate any potential hardware issues with bluetooth receivers.

ZMK supports multiple BLE “profiles”, which allows you to connect to and switch among multiple devices. Please refer to the [Bluetooth behavior](behaviors/bluetooth.md) section for detailed explanations on how to use them. If you don't make use of the mentioned behaviors you will have issues pairing your keyboard to other devices.

### Connecting Split Keyboard Halves

For split keyboards, after flashing each half individually you can connect them together by resetting them at the same time. Within a few seconds of resetting, both halves should automatically connect to each other.

:::note

If you have issues connecting the halves, make sure that both sides are getting powered properly through USB or batteries, then follow the
[recommended troubleshooting procedure](troubleshooting.md#split-keyboard-halves-unable-to-pair). This is typically necessary if you
swapped firmware sides between controllers, like flashing left side firmware to the same controller after flashing the right, or vice versa.

:::
