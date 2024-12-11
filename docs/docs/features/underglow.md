---
title: RGB Underglow
sidebar_label: RGB Underglow
---

RGB underglow is a feature used to control "strips" of RGB LEDs. Most of the time this is called underglow and creates a glow underneath the board using a ring of LEDs around the edge, hence the name. However, this can be extended to be used to control anything from a single LED to a long string of LEDs anywhere on the keyboard.

:::info
RGB underglow can also be used for per-key lighting. If you have RGB LEDs on your keyboard, this is what you want. For PWM/single color LEDs, see [Backlight](backlight.md).
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

## Enabling RGB Underglow

To enable RGB underglow on your board or shield, simply enable the `CONFIG_ZMK_RGB_UNDERGLOW` and `CONFIG_*_STRIP` configuration values in the `.conf` file for your board or shield.
For example:

```ini
CONFIG_ZMK_RGB_UNDERGLOW=y
# Use the STRIP config specific to the LEDs you're using
CONFIG_WS2812_STRIP=y
```

See [Configuration Overview](../config/index.md) for more instructions on how to use Kconfig.

If your board or shield does not have RGB underglow configured, refer to the [Adding RGB Underglow Support to a Keyboard](#adding-rgb-underglow-support-to-a-keyboard) section.

### Modifying the Number of LEDs

The number of LEDs specified in the default configuration for your board or shield may not match the number you have installed. For example, the `corne` shield specifies only 10 LEDs per side while supporting up to 27. On a split keyboard, a good sign of this mismatch is if the lit LEDs on each half are symmetrical.

The `chain-length` property of the `led_strip` node controls the number of underglow LEDs. If it is incorrect for your build, [you can change this property](../config/index.md#changing-devicetree-properties) in your `<keyboard>.keymap` file by adding a stanza like this one outside of any other node (i.e. above or below the `/` node):

```dts
&led_strip {
    chain-length = <21>;
};
```

For split keyboards, set `chain-length` to the number of LEDs installed on each half.

## Configuring RGB Underglow

See [RGB underglow configuration](../config/underglow.md).

## Adding RGB Underglow Support to a Keyboard

See [RGB underglow hardware integration page](../development/hardware-integration/lighting/underglow.md) on adding underglow support to a ZMK keyboard.
