---
id: dev-posix-board
title: Native Posix board target
---

In order to iterate quickly on firmware features, it can
be helpful to build and run the firmware on your local
workstation, with generated virtual press/release events
flowing into the handler functions.

To do this, you can build ZMK targetting the
`native_posix` board.

```
west build --pristine --board native_posix
```

Once built, you can run the firmware locally:

```
./build/zephyr/zephyr.exe
```

## Virtual Key Events

The virtual key presses are hardcoded in `boards/native_posix.overlay` file. should you want to change the sequence to test various actions like Mod-Tap, etc.
