---
title: RGB Underglow
sidebar_label: RGB Underglow
---

RGB underglow is a feature used to control "strips" of RGB LEDs. Most of the time this is called underglow and creates a glow underneath the board using a ring of LEDs around the edge, hence the name. However, this can be extended to be used to control anything from a single LED to a long string of LEDs anywhere on the keyboard.

:::info
RGB underglow can also be used for under-key lighting. If you have RGB LEDs on your keyboard, this is what you want. For PWM/single color LEDs, see [Backlight](backlight.mdx).
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

```ini
CONFIG_ZMK_RGB_UNDERGLOW=y
# Use the STRIP config specific to the LEDs you're using
CONFIG_WS2812_STRIP=y
```

See [Configuration Overview](/docs/config) for more instructions on how to
use Kconfig.

If your board or shield does not have RGB underglow configured, refer to [Adding RGB Underglow to a Board](#adding-rgb-underglow-to-a-board).

### Modifying the Number of LEDs

A common issue when enabling underglow is that some of the installed LEDs do not illuminate. This can happen when a board's default underglow configuration accounts only for either the downward facing LEDs or the upward facing LEDs under each key. On a split keyboard, a good sign that this may be the problem is that the unilluminated LEDs on each half are symmetrical.

The number of underglow LEDs is controlled by the `chain-length` property in the `led_strip` node. You can [change the value of this property](../config/index.md#changing-devicetree-properties) in the `<keyboard>.keymap` file by adding a stanza like this one outside of any other node (i.e. above or below the `/` node):

```dts
&led_strip {
    chain-length = <21>;
};
```

where the value is the total count of LEDs (per half, for split keyboards).

## Configuring RGB Underglow

See [RGB underglow configuration](/docs/config/underglow).

## Adding RGB Underglow to a Board

RGB underglow is always added to a board, not a shield. This is a consequence of needing to configure SPI to control the LEDs.
If you have a shield with RGB underglow, you must add a `boards/` directory within your shield folder to define the RGB underglow individually for each board that supports the shield.
Inside the `boards/` folder, you define a `<board>.overlay` for each different board.
For example, the Kyria shield has a `boards/nice_nano.overlay` file that defines the RGB underglow for the `nice_nano` board specifically.

### nRF52-Based Boards

With nRF52 boards, you can just use `&spi3` and define the pins you want to use.

Here's an example on a definition that uses P0.06:

```dts
#include <dt-bindings/led/led.h>

&pinctrl {
    spi3_default: spi3_default {
        group1 {
            psels = <NRF_PSEL(SPIM_MOSI, 0, 6)>;
        };
    };

    spi3_sleep: spi3_sleep {
        group1 {
            psels = <NRF_PSEL(SPIM_MOSI, 0, 6)>;
            low-power-enable;
        };
    };
};

&spi3 {
  compatible = "nordic,nrf-spim";
  status = "okay";

  pinctrl-0 = <&spi3_default>;
  pinctrl-1 = <&spi3_sleep>;
  pinctrl-names = "default", "sleep";

  led_strip: ws2812@0 {
    compatible = "worldsemi,ws2812-spi";

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

### Other Boards

Be sure to check the Zephyr documentation for the LED strip and necessary hardware bindings. Not every board has an `spi3` node, or configures `pinctrl` the same way. Reconcile this with any hardware restrictions found in the manufacturer's datasheet. Additional hardware interfaces may need to be enabled via Kconfig.

For example: the `sparkfun_pro_micro_rp2040` board can utilize SPI via PIO to run a WS2812 strip on `GP0`:

```dts
#include <dt-bindings/led/led.h>

&pinctrl {
    pio0_spi0_default: pio0_spi0_default {
        group1 {
            pinmux = <PIO0_P0>;
        };
    };
};

&pio0 {
    status = "okay";

    pio0_spi0: pio0_spi0 {
        pinctrl-0 = <&pio0_spi0_default>;
        pinctrl-names = "default";

        compatible = "raspberrypi,pico-spi-pio";
        #address-cells = <1>;
        #size-cells = <0>;
        clocks = <&system_clk>;
        clock-frequency = <4000000>;

        clk-gpios = <&gpio0 10 GPIO_ACTIVE_HIGH>;     /* Must be defined. Select a pin that is not used elsewhere. */
        mosi-gpios = <&pro_micro 1 GPIO_ACTIVE_HIGH>; /* Data In pin. */
        miso-gpios = <&pro_micro 1 GPIO_ACTIVE_HIGH>; /* Must be defined. Re-using the DI pin is OK for WS2812. */

        led_strip: ws2812@0 {
            compatible = "worldsemi,ws2812-spi";

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
};
```

Once you have your `led_strip` properly defined you need to add it to the root devicetree node `chosen` element:

```dts
/ {
    chosen {
        zmk,underglow = &led_strip;
    };
};
```

Finally you need to enable the `CONFIG_ZMK_RGB_UNDERGLOW` and `CONFIG_*_STRIP` configuration values in the `.conf` file of your board (or set a default in the `Kconfig.defconfig`):

```ini
CONFIG_ZMK_RGB_UNDERGLOW=y
# Use the STRIP config specific to the LEDs you're using
CONFIG_WS2812_STRIP=y
```
