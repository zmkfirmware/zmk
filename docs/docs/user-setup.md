---
id: user-setup
title: Installing ZMK
sidebar_label: Installing ZMK
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

Unlike other keyboard firmwares, ZMK Firmware has been built from the ground up to allow users to manage
their own keyboard configurations, including keymaps, specific hardware details, etc. all outside of the
core ZMK Firmware source repository.

In addition to this, most users do not need to install any complicated toolchains or tools to build ZMK,
instead using GitHub Actions to automatically build the user's configured firmware for them.

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

The remainder of this guide assumes the following prequisites:

1. You have an active, working [GitHub](https://github.com/) account.
1. You have installed and configured the [`git`](https://git-scm.com/) version control tool.

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
]}>
<Tab value="curl">

```
sh -c "$(curl -fsSL https://zmkfirmware.dev/setup.sh)"
```

</Tab>
<Tab value="wget">

```
sh -c "$(wget https://zmkfirmware.dev/setup.sh -O -)"
```

</Tab>
</Tabs>

### MCU Board Selection

When prompted, enter the number for the corresponding MCU board you would like to target:

```
MCU Board Selection:
1) nice!nano
2) QMK Proton-C
3) Quit
Pick an MCU board:
```

### Keyboard Shield Selection

When prompted, enter the number for the corresponding keyboard shield you would like to target:

```
Keyboard Shield Selection:
1) Kyria
2) Lily58
3) Quit
Pick an keyboard:
```

### GitHub Details

In order to have your new configuration automatically pushed, and then built using GitHub Actions, enter
some information about your particular GitHub info:

```
GitHub Username (leave empty to skip GitHub repo creation): petejohanson
GitHub Repo Name [zmk-config]:
GitHub Repo [https://github.com/petejohanson/zmk-config.git]:
```

Only the GitHub username is required; if you are happy with the defaults offered in the square brackets, you can simply hit `Enter`.

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

## Accessing Built Firmware

Once the setup script is complete and the new user config repository has been pushed, GitHub will automatically run the action
to build your keyboard firmware files. You can view the actions by clicking on the "Actions" tab on your GitHub repository.

## Keymap Changes

TODO: Document how to add your own keymap!
