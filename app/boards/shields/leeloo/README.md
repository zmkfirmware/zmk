# Clickety Split | Leeloo

![Leeloo](https://cdn.shopify.com/s/files/1/0599/3460/5491/files/Leeloo-rev1.0-w.jpg?v=1646798726)

Keyboard Designer: [clicketysplit.ca](https://clicketysplit.ca)
GitHub: [ClicketySplit](https://github.com/ClicketySplit)
Hardware Supported: Pro Micro, Elite-C, nice!nano v2

Albeit, there is no doubt where Leeloo's heritage is derived from—Lily58, and Corne.  It is not a copy-paste-modify implementation.

Leeloo has been designed from scratch; everything from the schematic to its PCB footprints, and column stagger. There are some subtle differences that may not be apparent; however, its subtle changes enable an interesting future.

Features:
* 4x6x5m Split Keyboard
* Support for MX/Box or Low Profile Choc switches.
* 90% of the switches are socketed; with the exception to the rotary encoder positions—6 positions require soldering.
* Support for 128x32 OLED Displays.
* The option to select one of three positions for an EC11 rotary encoder on each half.
* Support for Alps Alpine Micro Switch
* Support for 3.7v 301230 LiPo Battery

# Building Your Firmware
ZMK Firmware: [Introduction to ZMK](https://zmk.dev/docs/)
Installation: [Installing ZMK](https://zmk.dev/docs/user-setup)
Customization: [Customizing ZMK](https://zmk.dev/docs/customization)
Development Environment: [Basic Setup](https://zmk.dev/docs/development/setup)

Build command for the default keymap of Leeloo:

    west build -d build/left -p -b nice_nano_v2 -- -DSHIELD=leeloo_left
    west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=leeloo_right

Build command for your custom keymap of Leeloo:

    west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=leeloo_right -DZMK_CONFIG="C:/dev/zmk/[yourNmae]/leeloo/config"
    west build -d build/left -p -b nice_nano_v2 -- -DSHIELD=leeloo_left -DZMK_CONFIG="C:/dev/zmk/[yourName]/leeloo/config"

# Support
If you have any questions with regards to Leeloo, please [Contact Us](https://clicketysplit.ca/pages/contact-us).

Clickety Split
For the love of split keyboards.
