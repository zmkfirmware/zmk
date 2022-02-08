# [Stream Cheap 2x4 Macropad](https://www.partsnotincluded.com/diy-stream-deck-mini-macro-keyboard/)

A small 8-key macropad as a cheap alternative to the stream deck.

## Features

- Direct GPIO wiring, no GPIO matrix at all.
- Simple PCB design for hotswap switches.

## Build

<https://www.thingiverse.com/thing:5238367>

west build -p -b nice_nano_v2 -- -DSHIELD=stream_cheap_2x4
