---
title: Native Posix board target
---

In order to iterate quickly on firmware features, it can
be helpful to build and run the firmware on your local
workstation, with generated virtual press/release events
flowing into the handler functions.

## Prerequisites

In order to build targeting the `native_posix` board, you need to setup your system
with a compiler that can target 32-bit POSIX.

On Debian, you can do this with:

```sh
apt install -y gcc-multilib
```

## Building

To do this, you can build ZMK targeting the `native_sim` board.

```sh
west build --pristine --board native_sim/native/64 -- -DZMK_CONFIG=tests/none/normal/
```

Once built, you can run the firmware locally:

```
./build/zephyr/zmk.exe
```

## Virtual Key Events

The virtual key presses are hardcoded in `boards/extensions/native_sim/native_sim_64.overlay` file, should you want to change the sequence to test various actions like Mod-Tap, etc.
