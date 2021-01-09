---
title: Supported Hardware
sidebar_label: Supported Hardware
---

:::warning

ZMK Firmware is still an early stage project. Many features are still waiting to be implemented, and only a select few keyboards
have had their hardware details codified in boards/shields for ZMK.

:::

With the solid technical foundation of Zephyr™ RTOS, ZMK can support a wide diversity of hardware targets.
That being said, there are currently only a few specific [boards](/docs/faq#what-is-a-board)/[shields](/docs/faq#what-is-a-shield) that have been written and tested by the ZMK contributors.

## Boards

- [nice!nano](https://nicekeyboards.com/products/nice-nano-v1-0) (`nice_nano`)
- [nrfMicro](https://github.com/joric/nrfmicro) (`nrfmicro_13`, `nrfmicro_11`, `nrfmicro_11_flipped`)
- [BlueMicro840](https://store.jpconstantineau.com/#/group/bluemicro) (`bluemicro840_v1`)
- [QMK Proton-C](https://qmk.fm/proton-c/) (`proton_c`)
- [BDN9 Rev2](https://keeb.io/products/bdn9-rev-2-3x3-9-key-macropad-rotary-encoder-and-rgb) (`bdn9_rev2`)

## Keyboard Shields

- [Kyria](https://splitkb.com/products/kyria-pcb-kit) (`kyria_left` and `kyria_right`)
- [Corne](https://github.com/foostan/crkbd) (`corne_left` and `corne_right`)
- [Helix](https://github.com/mcmadhatter/helix) (`helix_left` and `helix_right`)
- [Lily58](https://github.com/kata0510/Lily58) (`lily58_left` and `lily58_right`)
- [Sofle](https://github.com/josefadamcik/SofleKeyboard) (`sofle_left` and `sofle_right`)
- [Splitreus62](https://github.com/Na-Cly/splitreus62) (`splitreus62_left` and `splitreus62_right`)
- [Jorne](https://github.com/joric/jorne) (`jorne_left` and `jorne_right`)
- [Jian](https://github.com/KGOH/Jian-Info) (`jian_left` and `jian_right`)
- [Reviung41](https://github.com/gtips/reviung/tree/master/reviung41) (`reviung41`)
- [RoMac+ v4](https://www.littlekeyboards.com/products/romac) (`romac_plus`)
- [RoMac v2](https://mechboards.co.uk/shop/kits/romac-macro-pad/) (`romac`)
- [Boardsource 3x4 Macro](https://boardsource.xyz/store/5ecc2008eee64242946c98c1) (`boardsource3x4`)
- [QAZ](https://www.cbkbd.com/product/qaz-keyboard-kit) (`qaz`)
- [CRBN](https://keygem.store/collections/group-buys/products/group-buy-featherlight-40-kit) (`crbn`)
- [tidbit](https://nullbits.co/tidbit/) (`tidbit`)
- [Eek!](https://www.cbkbd.com/product/eek-keyboard) (`eek`)
- [BFO-9000](https://keeb.io/products/bfo-9000-keyboard-customizable-full-size-split-ortholinear) (`bfo9000_left` and `bfo9000_right`)

## Other Hardware

In addition to the basic keyboard functionality, there is some initial support for additional keyboard hardware:

- Encoders
- OLEDs
- RGB Underglow

Until detailed documentation is available, feel free to ask questions about how these are supported in the [Discord server](https://zmkfirmware.dev/community/discord/invite).

## Contributing

If you'd like to add support for a new keyboard shield, head over to the [New Keyboard Shield](development/new-shield) documentation.
