---
title: MIDI Behavior
sidebar_label: MIDI
---

## Summary

The MIDI feature allows a keyboard to send MIDI messages to host.

Unlike other behaviors, MIDI only works over usb. Bluetooth MIDI is not supported.

Currently, only sending MIDI messages is supported. Boards cannot receive MIDI messages.

## Enabling MIDI support

MIDI support has been tested on both the `bluemicro840_v1` and the `nice_nano_v2`

1. add the config option to the boards `.conf`

```ini
CONFIG_ZMK_MIDI=y
```

2. include the dt-binding header file at the top of the boards `.keymap`

```dts
#include <dt-bindings/zmk/midi.h>
```

enabling MIDI support adds two new USB endpoints to the board. On linux, these get picked up by the `snd-usb-audio` driver

```
sudo cat /sys/kernel/debug/usb/devices
...
...
T:  Bus=03 Lev=01 Prnt=01 Port=00 Cnt=01 Dev#= 17 Spd=12   MxCh= 0
D:  Ver= 2.00 Cls=00(>ifc ) Sub=00 Prot=00 MxPS=64 #Cfgs=  1
P:  Vendor=1d50 ProdID=615e Rev= 3.05
S:  Manufacturer=ZMK Project
S:  Product=btrfld
S:  SerialNumber=DF4A1D9720CD8BD8
C:* #Ifs= 3 Cfg#= 1 Atr=e0 MxPwr=100mA
I:* If#= 0 Alt= 0 #EPs= 0 Cls=01(audio) Sub=01 Prot=00 Driver=snd-usb-audio
I:* If#= 1 Alt= 0 #EPs= 2 Cls=01(audio) Sub=03 Prot=00 Driver=snd-usb-audio
E:  Ad=01(O) Atr=02(Bulk) MxPS=  64 Ivl=0ms
E:  Ad=81(I) Atr=02(Bulk) MxPS=  64 Ivl=0ms
I:* If#= 2 Alt= 0 #EPs= 1 Cls=03(HID  ) Sub=00 Prot=00 Driver=usbhid
E:  Ad=82(I) Atr=03(Int.) MxPS=  16 Ivl=1ms
```

## MIDI keycodes

MIDI keycodes are defined in the header [`dt-bindings/zmk/midi.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/midi.h)

The majority of the keycode defines are the Note On/Off messages, which are denoted by `NOTE_*`There is one for each note in a standard octave, with 10 octaves available.

There is also support for control change messages, with `SUSTAIN`, `PORTAMENTO`, and `SOSTENUTO` currently implemented.

The following documents can be used to learn about all of the possible MIDI messages:
https://midi.org/summary-of-midi-1-0-messages
https://www.cs.cmu.edu/~music/cmsip/readings/MIDI%20tutorial%20for%20programmers.html

## Behavior Binding

- Reference: `&midi`
- Parameter #1: The midi keycode, e.g. `NOTE_C_5` or `SUSTAIN`

### Examples

1. while pressed, sends the E9 Note

   ```dts
   &midi NOTE_E_9
   ```

2. while pressed, presses the sustain pedal

   ```dts
   &midi SUSTAIN
   ```

MIDI keycodes can be combined with the other zmk behaviors to create interesting instruments.
