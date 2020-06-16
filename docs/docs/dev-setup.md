---
id: dev-setup
title: Basic Setup
sidebar_label: Basic Setup
---

## Prerequisites

A unix-like environment with the following base packages installed:

- Git
- Python 3
- `pip`
- `wget`
- devicetree compiler
- CMake
- `dfu-util`
- Various build essentials, e.g. gcc, automake, autoconf

### Debian/Ubuntu

On Debian and Ubuntu, we'll use apt to install our base dependencies:

#### Apt Update

First, if you haven't updated recently, or if this is a new install from WSL,
you should update to get the latest package information:

```bash
sudo apt update
```

#### Install Dependencies

With the latest package information, you can now install the base dependencies:

```bash
sudo apt install -y \
	git \
	wget \
	autoconf \
	automake \
	build-essential \
	ccache \
	device-tree-compiler \
	dfu-util \
	g++ \
	gcc \
	gcc-multilib \
	libtool \
	make \
	ninja-build \
	cmake \
	python3-dev \
	python3-pip \
	python3-setuptools \
	xz-utils
```

:::note
Ubuntu 18.04 LTS release packages a version of CMake that is too old. Please upgrade to Ubuntu 20.04 LTS
or download and install CMake version 3.13.1 or newer manually.
:::

### Fedora

TODO

### macOS

TODO

### WSL

Windows Subsystem for Linux can use various Linux distributions. Find a WSL installation on the [Windows Store](https://aka.ms/wslstore).

After installing your preferred flavor, follow the directions above on [Debian/Ubuntu](#debianubuntu) or [Fedora](#fedora).

:::note
On WSL2 don't put the project files into `/mnt/c/` as file I/O speeds are extremely slow. Instead, run everything in the Linux system and use `cp` to move files over to `/mnt/c/` as needed.
:::

## Setup

### West Build Command

`west` is the [Zephyr™ meta-tool](https://docs.zephyrproject.org/latest/guides/west/index.html) used to configure and build Zephyr™ applications. It can be installed by using the `pip` python package manager:

```bash
pip3 install --user west
```

:::note
If you don't already have it configured, you may need to update your
`PATH` to include the pip install path. See [User Installs](https://pip.pypa.io/en/stable/user_guide/#user-installs) and [Stack Overflow](https://stackoverflow.com/questions/38112756/how-do-i-access-packages-installed-by-pip-user) for more details.
:::

### Zephyr™ ARM SDK

To build firmwares for the ARM architecture (all supported MCUs/keyboards at this point), you'll need to install the Zephyr™ ARM SDK to your system:

```
export ZSDK_VERSION=0.11.2
wget -q "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZSDK_VERSION}/zephyr-toolchain-arm-${ZSDK_VERSION}-setup.run" && \
	sh "zephyr-toolchain-arm-${ZSDK_VERSION}-setup.run" --quiet -- -d ~/.local/zephyr-sdk-${ZSDK_VERSION} && \
	rm "zephyr-toolchain-arm-${ZSDK_VERSION}-setup.run"
```

The installation will prompt with several questions about installation location, and creating a default `~/.zephyrrc` for you with various variables. The defaults shouldn normally work as expected.

### Source Code

Next, you'll need to clone the ZMK source repository if you haven't already:

```
git clone https://github.com/zmkfirmware/zmk.git
```

### Initialize & Update Zephyr Workspace

Since ZMK is built as a Zephyr™ application, the next step is
to use `west` to initialize and update your workspace. The ZMK
Zephyr™ application is in the `app/` source directory:

#### Step into the repository

```bash
cd zmk
```

#### Initialize West

```bash
west init -l app/
```

:::note
If you encounter errors like `command not found: west` then your `PATH` environment variable is likely
missing the Python 3 user packages directory. See the [West Build Command](#west-build-command)
section again for links to how to do this
:::

#### Update To Fetch Modules

```bash
west update
```

#### Export Zephyr™ Core

```bash
west zephyr-export
```

#### Install Zephyr Python Dependencies

```bash
pip3 install --user -r zephyr/scripts/requirements-base.txt
```

### Environment Variables

By default, the Zephyr™ SDK will create a file named `~/.zephyrrc` with the correct environment variables to build ZMK.
We suggest two main [options](https://docs.zephyrproject.org/latest/guides/env_vars.html?highlight=zephyrrc) for how to load those settings.

#### Per Shell

To load the Zephyr environment properly for just one transient shell, run the following from your ZMK checkout directory:

```
source zephyr/zephyr-env.sh
```

#### All Shells

To load the environment variables for your shell every time,
append the existing `~/.zephyrrc` file to your shell's RC file and then start a new shell.

##### Bash

```
cat ~/.zephyrrc >> ~/.bashrc
```

##### ZSH

```
cat ~/.zephyrrc >> ~/.zshrc
```

## Build

From here on, building and flashing ZMK should all be done from the `app/` subdirectory of the ZMK checkout:

```bash
cd app
```

To build for your particular keyboard, the behaviour varies slightly depending on if you are building for a keyboard with
an onboard MCU, or one that uses a MCU board addon.

### Keyboard (Shield) + MCU Board

ZMK treats keyboards that take a MCU addon board as [shields](https://docs.zephyrproject.org/latest/guides/porting/shields.html), and treats the smaller MCU board as the true [board](https://docs.zephyrproject.org/latest/guides/porting/board_porting.html)

Given the following:

- MCU Board: Proton-C
- Keyboard PCB: kyria
- Keymap: default

You can build ZMK with the following:

```bash
west build -b proton_c -- -DSHIELD=kyria -DKEYMAP=default
```

### Keyboard With Onboard MCU

Keyboards with onboard MCU chips are simply treated as the [board](https://docs.zephyrproject.org/latest/guides/porting/board_porting.html) as far as Zephyr™ is concerned.

Given the following:

- Keyboard: Planck
- Keymap: default

you can build ZMK with the following:

```bash
west build -b planck -- -DKEYMAP=default
```

## Flashing

Once built, the previously supplied parameters will be remember, so you can simply run the following to flash your
board, with it in bootloader mode:

```
west flash
```
