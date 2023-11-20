---
title: USB Logging
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

## Overview

If you are developing ZMK on a device that does not have a built in UART for debugging and log/console output,
Zephyr can be configured to create a USB CDC ACM device and the direct all `printk`, console output, and log
messages to that device instead.

:::warning Battery Life Impact

Enabling logging increases the power usage of your keyboard, and can have a non-trivial impact to your time on battery.
It is recommended to only enable logging when needed, and not leaving it on by default.

:::

## Kconfig

The `CONFIG_ZMK_USB_LOGGING` Kconfig enables USB logging. This can be set at the keyboard level, typically in the `config/<your_keyboard>.conf`
file if you are using a [user config repository](user-setup.md). It can also be enabled at the ZMK level using the `app/prj.conf` file, or other
search locations described in the [configuration overview](config/index.md#config-file-locations).

Logging can be further configured using Kconfig described in [the Zephyr documentation](https://docs.zephyrproject.org/3.2.0/services/logging/index.html).
For instance, setting `CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS` to a large value such as `8000` might help catch issues that happen near keyboard
boot, before you can connect to view the logs.

:::note
In Github Actions, you can check the `<Keyboard> Kconfig file` step output to verify the options above have been enabled
for you successfully.
:::

```ini
# Turn on logging, and set ZMK logging to debug output
CONFIG_ZMK_USB_LOGGING=y
```

## Viewing Logs

After flashing the updated ZMK image, the board should expose a USB CDC ACM device that you can connect to and view the logs.

<Tabs
defaultValue="linux"
values={[
{label: 'Linux', value: 'linux'},
{label: 'Windows', value: 'win'},
{label: 'MacOS', value: 'macos'}
]}>
<TabItem value="linux">

On Linux, this should be a device like `/dev/ttyACM0` and you can connect with `minicom` or `tio` as usual, e.g.:

```sh
sudo tio /dev/ttyACM0
```

</TabItem>
<TabItem value="win">

On Windows, you can use [PuTTY](https://www.putty.org/). Once installed, use Device Manager to figure out which COM port your controller is communicating on (listed under 'Ports (COM & LPT)') and specify that as the 'Serial line' in PuTTY.

![Controller COM port](../assets/usb-logging/com.jpg)

![PuTTY settings](../assets/usb-logging/putty.jpg)

If you already have the Ardunio IDE installed you can also use its built-in Serial Monitor.

</TabItem>
<TabItem value="macos">

On macOS, the device name is something like `/dev/tty.usbmodemXXXXX` where `XXXXX` is some numerical ID.
You can connect to the device with [tio](https://tio.github.io/) (can be installed via [Homebrew](https://formulae.brew.sh/formula/tio)):

```sh
sudo tio /dev/tty.usbmodem14401
```

You should see tio printing `Disconnected` or `Connected` when you disconnect or reconnect the USB cable.
</TabItem>
</Tabs>

From there, you should see the various log messages from ZMK and Zephyr, depending on which systems you have set to what log levels.
