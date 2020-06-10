---
id: dev-setup
title: Basic Setup
sidebar_label: Basic Setup
---

## Preprequisites

A unix-like environment with the following base packages installed:

- Python 3
- `pip`
- `wget`
- devicetree compiler
- CMake
- `dfu-util`
- Various build essentials, e.g. gcc, automake, autoconf

### Debian/Ubuntu

On Debian and Ubuntu, this can be accomplished with:

```bash
apt-get install -y \
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
    cmake \
	ninja-build \
	python3-dev \
	python3-pip \
	python3-setuptools \
	xz-utils
```

### Fedora

TODO

### macOS

TODO

### WSL

TODO

## Setup

### West Build Command

`west` is the [Zephyr™ meta-tool](https://docs.zephyrproject.org/latest/guides/west/index.html) used to configure and build Zephyr™ applications. It can be installed by using the `pip` python package manager:

```bash
pip3 install --user west
```

:::note
If you don't already have it configured, you may need to update your
`PATH` to include the pip install path. See [User Installs](https://pip.pypa.io/en/stable/user_guide/#user-installs) and (Stack Overflow)[https://stackoverflow.com/questions/38112756/how-do-i-access-packages-installed-by-pip-user] for more details.
:::

### Zephyr™ ARM SDK

To build firmwares for the ARM architecture (all supported MCUs/keyboards at this point), you'll need to install the Zephyr™ ARM SDK to your system:

```
export ZSDK_VERSION=0.11.3
wget -q "https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZSDK_VERSION}/zephyr-toolchain-arm-${ZSDK_VERSION}-setup.run" && \
	sh "zephyr-toolchain-arm-${ZSDK_VERSION}-setup.run" --quiet -- -d /opt/toolchains/zephyr-sdk-${ZSDK_VERSION} && \
	rm "zephyr-toolchain-arm-${ZSDK_VERSION}-setup.run"
```

The installation will prompt with several questions about installation location, and creating a default `~/.zephyrrc` for you with various variables. The defaults shouldn normally work as expected.

### Source Code

Next, you'll need to clone the ZMK source repository if you haven't already:

```
git clone https://github.com/zmkfirmware/zmk.git
```

### Initialize & Update Zephy Workspace

Since ZMK is built as a Zephyr™ application, the next step is
to use `west` to initialize and update your workspace. The ZMK
Zephyr™ application is in the `app/` source directory:

#### Initialize West

```bash
west init -l app/
```

#### Update To Fetch Modules

```bash
west update
```

#### Export Zephyr™ Core

```bash
west zephyr-export
```

## Build

Actually building the ZMK firmware occurs within the `app/` subdirectory
of the ZMK repository. To build for your particular keyboard, the behaviour varies slightly depending on if you are building for a keyboard with
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
