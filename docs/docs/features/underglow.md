---
title: RGB Underglow
sidebar_label: RGB Underglow
---

RGB underglow is a feature used to control "strips" of RGB LEDs. Most of the time this is called underglow and creates a glow underneath the board using a ring of LEDs around the edge, hence the name. However, this can be extended to be used to control anything from a single LED to a long string of LEDs anywhere on the keyboard.

:::info
RGB underglow can also be used for under-key lighting. If you have RGB LEDs on your keyboard, this is what you want. For PWM/single color LEDs, see [Backlight](backlight.md).
:::

ZMK supports all the RGB LEDs supported by Zephyr. Here's the current list supported:

- WS2812-ish (WS2812B, WS2813, SK6812, or compatible)
- APA102
- LPD880x (LPD8803, LPD8806, or compatible)

Of the compatible types, the WS2812 LED family is by far the most popular type. Currently each of these types of LEDs are expected to be run using SPI with a couple of exceptions.

Here you can see the RGB underglow feature in action using WS2812 LEDs.

<figure class="video-container">
  <iframe src="//www.youtube.com/embed/2KJkq8ssDU0" frameborder="0" allowfullscreen width="100%"></iframe>
</figure>

## Enabling RGB Underglow

To enable RGB underglow on your board or shield, simply enable the `CONFIG_ZMK_RGB_UNDERGLOW` and `CONFIG_*_STRIP` configuration values in the `.conf` file for your board or shield.
For example:

```
CONFIG_ZMK_RGB_UNDERGLOW=y
# Use the STRIP config specific to the LEDs you're using
CONFIG_WS2812_STRIP=y
```

See [Configuration Overview](/docs/config) for more instructions on how to
use Kconfig.

If your board or shield does not have RGB underglow configured, refer to [Adding RGB Underglow to a Board](#adding-rgb-underglow-to-a-board).

## Configuring RGB Underglow

See [RGB underglow configuration](/docs/config/underglow).

## Adding RGB Underglow to a Board

RGB underglow is always added to a board, not a shield. This is a consequence of needing to configure SPI to control the LEDs.
If you have a shield with RGB underglow, you must add a `boards/` directory within your shield folder to define the RGB underglow individually for each board that supports the shield.
Inside the `boards/` folder, you define a `<board>.overlay` for each different board.
For example, the Kyria shield has a `boards/nice_nano.overlay` file that defines the RGB underglow for the `nice_nano` board specifically.

### nRF52-based boards

With nRF52 boards, you can just use `&spi1` and define the pins you want to use.

To identify which pin number you need to put in the config you need do to a bit of math. You need the hardware port and run it through a function.
**32 \* X + Y** = `<Pin number>` where X is first part of the hardware port "PX.01" and Y is the second part of the hardware port "P1.Y".

(_P1.13_ would give you _32 \* 1 + 13_ = `<45>` and P0.15 would give you _32 \* 0 + 15_ = `<15>`)

Here's an example on a definition that uses P0.06:

```
#include <dt-bindings/led/led.h>

&spi1 {
  compatible = "nordic,nrf-spim";
  status = "okay";
  mosi-pin = <6>;
  // Unused pins, needed for SPI definition, but not used by the ws2812 driver itself.
  sck-pin = <5>;
  miso-pin = <7>;

  led_strip: ws2812@0 {
    compatible = "worldsemi,ws2812-spi";
    label = "WS2812";

    /* SPI */
    reg = <0>; /* ignored, but necessary for SPI bindings */
    spi-max-frequency = <4000000>;

    /* WS2812 */
    chain-length = <10>; /* number of LEDs */
    spi-one-frame = <0x70>;
    spi-zero-frame = <0x40>;
    color-mapping = <LED_COLOR_ID_GREEN
                          LED_COLOR_ID_RED
                          LED_COLOR_ID_BLUE>;
  };
};
```

:::info

If you are configuring SPI for an nRF52 based board, double check that you are using pins that aren't restricted to low frequency I/O.
Ignoring these restrictions may result in poor wireless performance. You can find the list of low frequency I/O pins for the nRF52840 [here](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fpin.html&cp=4_0_0_6_0).

:::

:::note

Standard WS2812 LEDs use a wire protocol where the bits for the colors green, red, and blue values are sent in that order.
If your board/shield uses LEDs that require the data sent in a different order, the `color-mapping` property ordering should be changed to match.

:::

### Other boards

For other boards, you must select an SPI definition that has the `MOSI` pin as your data pin going to your LED strip.

Here's another example for a non-nRF52 board on `spi1`:

```
#include <dt-bindings/led/led.h>

&spi1 {

  led_strip: ws2812@0 {
    compatible = "worldsemi,ws2812-spi";
    label = "WS2812";

    /* SPI */
    reg = <0>;
    spi-max-frequency = <5250000>;

    /* WS2812 */
    chain-length = <10>; /* number of LEDs */
    spi-one-frame = <0x70>; /* make sure to configure this properly for your SOC */
    spi-zero-frame = <0x40>; /* make sure to configure this properly for your SOC */
    color-mapping = <LED_COLOR_ID_GREEN
                          LED_COLOR_ID_RED
                          LED_COLOR_ID_BLUE>;
  };
};
```

Once you have your `led_strip` properly defined you need to add it to the root devicetree node `chosen` element:

```
/ {
	chosen {
		zmk,underglow = &led_strip;
	};
};
```

Finally you need to enable the `CONFIG_ZMK_RGB_UNDERGLOW` and `CONFIG_*_STRIP` configuration values in the `.conf` file of your board (or set a default in the `Kconfig.defconfig`):

```
CONFIG_ZMK_RGB_UNDERGLOW=y
# Use the STRIP config specific to the LEDs you're using
CONFIG_WS2812_STRIP=y
```
