---
title: Lighting
sidebar_label: Lighting
---

ZMK supports two distinct systems in order to control lighting hardware integrated into keyboards.
Your keyboard likely uses only one type, depending on the type of LED hardware it supports:

- [RGB underglow](#rgb-underglow) system controls LED strips composed of addressable RGB LEDs.
  Most keyboards that have multi-color lighting utilizes these.
- [Backlight](#backlight) system controls parallel-connected, non-addressable, single color LEDs.
  These are found on keyboards that have a single color backlight that only allows for brightness control.

:::warning

Although the naming of the systems might imply it, which system you use typically does _not_ depend on the physical location of the LEDs.
Instead, you should use the one that supports the LED hardware type that your keyboard has, as described above.

:::

## RGB Underglow

RGB underglow is a feature used to control "strips" of RGB LEDs. Most of the time this is called underglow and creates a glow underneath the board using a ring of LEDs around the edge, hence the name. However, this can be extended to be used to control anything from a single LED to a long string of LEDs anywhere on the keyboard.

:::info
RGB underglow can also be used for per-key lighting. If you have RGB LEDs on your keyboard, this is what you want. For PWM/single color LEDs, see [Backlight section below](#backlight).
:::

ZMK relies on Zephyr's `led-strip` drivers for this feature. The following LEDs/LED families have been implemented:

- WS2812 (includes WS2812B, WS2813, SK6812, and others)
- APA102
- LPD880x (includes LPD8803, LPD8806, and others)

The WS2812 LED family is by far the most popular of these types, so this page will primarily focus on working with it.

Here you can see the RGB underglow feature in action using WS2812 LEDs.

<figure class="video-container">
  <iframe src="//www.youtube.com/embed/2KJkq8ssDU0" frameborder="0" allowfullscreen width="100%"></iframe>
</figure>

### Enabling RGB Underglow

To enable RGB underglow on your board or shield, simply enable the `CONFIG_ZMK_RGB_UNDERGLOW` and `CONFIG_*_STRIP` configuration values in the `.conf` file for your board or shield.
For example:

```ini
CONFIG_ZMK_RGB_UNDERGLOW=y
# Use the STRIP config specific to the LEDs you're using
CONFIG_WS2812_STRIP=y
```

See [Configuration Overview](../config/index.md) for more instructions on how to use Kconfig.

If your board or shield does not have RGB underglow configured, refer to the [Adding RGB Underglow Support to a Keyboard](#adding-rgb-underglow-support-to-a-keyboard) section.

#### Modifying the Number of LEDs

The number of LEDs specified in the default configuration for your board or shield may not match the number you have installed. For example, the `corne` shield specifies only 10 LEDs per side while supporting up to 27. On a split keyboard, a good sign of this mismatch is if the lit LEDs on each half are symmetrical.

The `chain-length` property of the `led_strip` node controls the number of underglow LEDs. If it is incorrect for your build, [you can change this property](../config/index.md#changing-devicetree-properties) in your `<keyboard>.keymap` file by adding a stanza like this one outside of any other node (i.e. above or below the `/` node):

```dts
&led_strip {
    chain-length = <21>;
};
```

For split keyboards, set `chain-length` to the number of LEDs installed on each half.

### Configuring RGB Underglow

See [RGB underglow configuration](../config/lighting.md#rgb-underglow).

### Adding RGB Underglow Support to a Keyboard

See [RGB underglow hardware integration page](../development/hardware-integration/lighting/underglow.md) on adding underglow support to a ZMK keyboard.

## Backlight

Backlight is a feature used to control an array of LEDs, usually placed through or under switches.

:::info
Unlike [RGB underglow](#rgb-underglow), backlight can only control single color LEDs. Additionally, because backlight LEDs all receive the same power, it's not possible to dim individual LEDs.
:::

### Enabling Backlight

To enable backlight on your board or shield, add the following line to your `.conf` file of your user config directory as such:

```ini
CONFIG_ZMK_BACKLIGHT=y
```

If your board or shield does not have backlight configured, refer to [Adding Backlight to a board or a shield](#adding-backlight-to-a-board-or-a-shield).

### Configuring Backlight

There are various Kconfig options used to configure the backlight feature.
See [backlight configuration](../config/lighting.md#backlight) for details.

### Adding Backlight to a Board or a Shield

See [backlight hardware integration page](../development/hardware-integration/lighting/backlight.mdx) for information on adding backlight support to a ZMK keyboard.
