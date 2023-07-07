# Clickety Split | Leeloo-Micro

![Leeloo-Micro v1 Wireless](https://github.com/ClicketySplit/build-guides/blob/main/leeloo/images/gallery/Leeloo-Micro-v1-ZMK.jpg)

Keyboard Designer: [clicketysplit.ca](https://clicketysplit.ca)
GitHub: [ClicketySplit](https://github.com/ClicketySplit)
Hardware Supported: nice!nano v2, nice!view v1

Leeloo-Micro is a 3x5x5m derivative of Leeloo v2; inheriting the column stagger and modifiers row, yet, reducing the number of switches by removing the top row and outside columns. With Leeloo-Micro's inaugural release being wireless, it leverages nice!nanos and nice!views for its microcontrollers and displays.

## Features

- 3x5x5m Split Keyboard
- Support for Kailh Low Profile Choc switches with 18mm x 18mm spacing.
- All switch locations are socketed.
- Support for Alps Alpine EC11 Rotary Encoders—one on each side, in one of two locations.
  - Rotary encoder locations are socketed.
- nice!view Displays are inherently supported, socketed, and no extra wiring is required.
- Support for per-switch RGB underglow.
- Support for both 110mAh or 700mAh batteries.
- Support for Alps Alpine Micro On/off switches.

# Building Leeloo-Micro ZMK Firmware

ZMK Firmware: [Introduction to ZMK](https://zmk.dev/docs/)
Installation: [Installing ZMK](https://zmk.dev/docs/user-setup)
Customization: [Customizing ZMK](https://zmk.dev/docs/customization)
Development Environment: [Basic Setup](https://zmk.dev/docs/development/setup)

Build commands for the default keymap of Leeloo-Micro:

```
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD=leeloo_micro_left
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=leeloo_micro_right
```

Build commands for your custom keymap of Leeloo-Micro:

```
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=leeloo_micro_right -DZMK_CONFIG="C:/dev/zmk/[yourName]/leeloo_micro/config"
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD=leeloo_micro_left -DZMK_CONFIG="C:/dev/zmk/[yourName]/leeloo_micro/config"
```

## Building Leeloo-Micro's ZMK Firmware with nice!view Displays

There are a couple of files that need to be adjusted before the build commands can be run.

### Edit the leeloo_micro.keymap File

Near the top 3rd of the leeloo_micro.keymap file, locate the following code block:

```
//nice_view_spi: &spi0 {
//  cs-gpios = <&pro_micro 4 GPIO_ACTIVE_HIGH>;
//};
```

Remove the forward slashes to resemble the following:

```
nice_view_spi: &spi0 {
    cs-gpios = <&pro_micro 4 GPIO_ACTIVE_HIGH>;
};
```

Save your changes and close the file.

### Sample Build Commands for nice!view Displays

Build commands for the default keymap of Leeloo-Micro:

```
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD="leeloo_micro_left nice_view_adapter nice_view"
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD="leeloo_micro_right nice_view_adapter nice_view"
```

Build commands for your custom keymap of Leeloo-Micro:

```
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD="leeloo_micro_left nice_view_adapter nice_view" -DZMK_CONFIG="/workspaces/zmk-config/[yourName]/leeloo_micro/config"
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD="leeloo_micro_right nice_view_adapter nice_view" -DZMK_CONFIG="/workspaces/zmk-config/[yourName]/leeloo_micro/config"
```

# Support

If you have any questions with regards to Leeloo-Micro, please [Contact Us](https://clicketysplit.ca/pages/contact-us).

Clickety Split
For the love of split keyboards.
