---
id: dev-guide-add-encoders
title: Adding Encoders
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

EC11 encoder support can be added to your board or shield by adding the appropriate lines to your board/shield's .conf, .dtsi, and .overlay files.

<Tabs
defaultValue="conf"
values={[
{label: '.conf', value: 'conf'},
{label: '.dtsi', value: 'dtsi'},
{label: '.overlay', value: 'overlay'},
]}>
<TabItem value="conf">

In your .conf file you will need to add the following lines so that the EC11 drivers can be enabled:

```
# Uncomment to enable encoder
# CONFIG_EC11=y
# CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

These should be commented by default if encoders are optional, but can be uncommented if encoders are part of the original design.

</TabItem>
<TabItem value = "dtsi">
In your .dtsi file you will need to add the following lines to define the encoder sensor:


```
left_encoder: encoder_left {
		compatible = "alps,ec11";
		label = "LEFT_ENCODER";
		a-gpios = <PIN_A (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
		b-gpios = <PIN_B (GPIO_ACTIVE_HIGH | GPIO_PULL_UP)>;
		resolution = <4>;
	};
```
Here you will have to replace PIN_A and PIN_B with the appropriate pins that your PCB utilizes for the encoder(s). 

For keyboards that use the Pro Micro or any of the Pro Micro replacements, Sparkfun's [Pro Micro Hookup Guide](https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/hardware-overview-pro-micro) has a pinout diagram that can be useful to determine the right pins. Reference either the blue numbers labeled "Arduino" (digital pins) or the green numbers labeled "Analog" (analog pins). For pins that are labeled as both digital and analog, refer to your specific board's .dtsi file to determine how you should refer to that pin.

Replace `left` with `right` to define a right-side encoder, although note that support for peripheral side sensors is still in progress.

</TabItem>
<TabItem value = "overlay">
</TabItem>
</Tabs>
