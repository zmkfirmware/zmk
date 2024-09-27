---
title: RGB Underglow
sidebar_label: RGB Underglow
---

RGB underglow is a feature used to control "strips" of RGB LEDs. Most of the time this is called underglow and creates a glow underneath the board using a ring of LEDs around the edge, hence the name. However, this can be extended to be used to control anything from a single LED to a long string of LEDs anywhere on the keyboard.

:::info
RGB underglow can also be used for per-key lighting. If you have RGB LEDs on your keyboard, this is what you want. For PWM/single color LEDs, see [Backlight](backlight.mdx).
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

See [Configuration Overview](/docs/config) for more instructions on how to use Kconfig.

If your board or shield does not have RGB underglow configured, refer to [Adding RGB Underglow to a Board](#adding-rgb-underglow-to-a-board).

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

See [RGB underglow configuration](/docs/config/underglow).

## Adding RGB Underglow to a Board

Support for RGB underglow is always added to a board, not a shield. This is because the LED strip drivers rely on hardware-specific interfaces (e.g. SPI, I2S) and configurations, which shields do not control.

Shields written for boards which support RGB underglow should add a `boards/` folder underneath the shield folder. Inside this `boards/` folder, create a `<board>.overlay` for any of the boards the shield can be used with. Place all hardware-specific configurations in these `.overlay` files.

For example: the `kyria` shield has a [`boards/nice_nano_v2.overlay`](https://github.com/zmkfirmware/zmk/blob/main/app/boards/shields/kyria/boards/nice_nano_v2.overlay) and a [`boards/nrfmicro_13.overlay`](https://github.com/zmkfirmware/zmk/blob/main/app/boards/shields/kyria/boards/nrfmicro_13.overlay), which configure a WS2812 LED strip for the `nice_nano_v2` and `nrfmicro_13` boards respectively.

### nRF52-Based Boards

Using an SPI-based LED strip driver on the `&spi3` interface is the simplest option for nRF52-based boards. If possible, avoid using pins which are limited to low-frequency I/O for this purpose. The resulting interference may result in poor wireless performance.

:::info

The list of low frequency I/O pins for the nRF52840 can be found [here](https://docs.nordicsemi.com/bundle/ps_nrf52840/page/pin.html).

:::

The following example uses `P0.06` as the "Data In" pin of a WS2812-compatible LED strip:

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

### Final Steps

Once the `led_strip` is properly defined, add it to the `chosen` node under the root devicetree node:

```dts
/ {
    chosen {
        zmk,underglow = &led_strip;
    };
};
```
