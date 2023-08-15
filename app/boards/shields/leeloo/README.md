# Clickety Split | Leeloo v2

![Leeloo v2](https://github.com/ClicketySplit/build-guides/blob/main/leeloo/images/gallery/Leeloo-v2-ZMK.jpg)

Keyboard Designer: [clicketysplit.ca](https://clicketysplit.ca)
GitHub: [ClicketySplit](https://github.com/ClicketySplit)
Hardware Supported: Pro Micro, Elite-C, and nice!nano v2

Leeloo v2 has been designed from scratch—again. Everything from the wiring schematic to its case. Leeloo v2 still keeps the column stagger that it's known for, along with its low profile design.

## Features/Differences from Leeloo v1

- Support for Kailh Low Profile Choc switches with 18mm x 18mm spacing.
  - A version for Kailh Box/MX switches with 19.05mm x 19.05mm spacing will be available in the future.
- All switch locations are socketed.
- Rotary encoder locations are socketed.
  - One of two locations on each side can be used for a rotary encoder.
- OLED Displays and nice!view Displays are natively supported, socketed, and no extra wiring is required.
- Support for per-switch RGB underglow.
- Better location for 110mAh or 700mAh batteries.
  - Different location for soldering battery leads.
- Support for Alps Alpine Micro On/off switches.

# Leeloo v1

![Leeloo](https://github.com/ClicketySplit/build-guides/blob/main/leeloo/images/gallery/Leeloo-v1.jpg)

## Features

- 4x6x5m Split Keyboard
- Support for both Low Profile Choc switches, and Box/MX switches; 19.05mm x 19.05mm spacing.
- 90% of the switches are socketed; with the exception to the rotary encoder positions.
- Support for Alps Alpine EC11 Rotary Encoders—one on each side, in one of three locations.
- Support for OLED Displays or nice!view Displays.
  - nice!view displays require a wire to be soldered from the CS Pin on nice!view display to P0.22 or D4 on the nice!nano.
- Support for both 110mAh or 700mAh batteries.
- Solder pads for battery leads.
- Support for Alps Alpine Micro On/off switches.

# Building Leeloo's ZMK Firmware

ZMK Firmware: [Introduction to ZMK](https://zmk.dev/docs/)
Installation: [Installing ZMK](https://zmk.dev/docs/user-setup)
Customization: [Customizing ZMK](https://zmk.dev/docs/customization)
Development Environment: [Basic Setup](https://zmk.dev/docs/development/setup)

Build commands for the default keymap of Leeloo v1:

```
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD=leeloo_left
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=leeloo_right
```

Build commands for the default keymap of Leeloo v2:

```
west build -d build/left_v2 -p -b nice_nano_v2 -- -DSHIELD=leeloo_rev2_left
west build -d build/right_v2 -p -b nice_nano_v2 -- -DSHIELD=leeloo_rev2_right
```

Build commands for your custom keymap of Leeloo v1:

```
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD=leeloo_right -DZMK_CONFIG="C:/dev/zmk/[yourName]/leeloo/config"
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD=leeloo_left -DZMK_CONFIG="C:/dev/zmk/[yourName]/leeloo/config"
```

Build commands for your custom keymap of Leeloo v2:

```
west build -d build/right_v2 -p -b nice_nano_v2 -- -DSHIELD=leeloo_rev2_right -DZMK_CONFIG="C:/dev/zmk/[yourName]/leeloo_v2/config"
west build -d build/left_v2 -p -b nice_nano_v2 -- -DSHIELD=leeloo_rev2_left -DZMK_CONFIG="C:/dev/zmk/[yourName]/leeloo_v2/config"
```

## Building Leeloo's ZMK Firmware with nice!view Displays

There are a couple of files that need to be adjusted before the build commands can be run.

### Edit the leeloo[_rev2].keymap File

Near the top 3rd of the leeloo[_rev2].keymap file, locate the following code block:

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

Build commands for the default keymap of Leeloo v1:

```
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD="leeloo_left nice_view_adapter nice_view"
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD="leeloo_right nice_view_adapter nice_view"
```

Build commands for the default keymap of Leeloo v2:

```
west build -d build/left_v2 -p -b nice_nano_v2 -- -DSHIELD="leeloo_rev2_left nice_view_adapter nice_view"
west build -d build/right_v2 -p -b nice_nano_v2 -- -DSHIELD="leeloo_rev2_right nice_view_adapter nice_view"
```

Build commands for your custom keymap of Leeloo v2:

```
west build -d build/left -p -b nice_nano_v2 -- -DSHIELD="leeloo_rev2_left nice_view_adapter nice_view" -DZMK_CONFIG="/workspaces/zmk-config/[yourName]/leeloo_v2/config"
west build -d build/right -p -b nice_nano_v2 -- -DSHIELD="leeloo_rev2_right nice_view_adapter nice_view" -DZMK_CONFIG="/workspaces/zmk-config/[yourName]/leeloo_v2/config"
```

# Support

If you have any questions with regards to Leeloo, please [Contact Us](https://clicketysplit.ca/pages/contact-us).

Clickety Split
For the love of split keyboards.
