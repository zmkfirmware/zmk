---
id: dev-guide-usb-logging
title: USB Logging
---

## Overview

If you are developing ZMK on a device that does not have a built in UART for debugging and log/console output,
Zephyr can be configured to create a USB CDC ACM device and the direct all `printk`, console output, and log
messages to that device instead.

## Kconfig

The following KConfig values need to be set, either by copy and paste into the `app/prj.conf` file, or by running
`west build -t menuconfig` and manually enabling the various settings in that UI.

```
# Turn on logging, and set ZMK logging to debug output
CONFIG_LOG=y
CONFIG_ZMK_LOG_LEVEL_DBG=y

# Turn on USB CDC ACM device
CONFIG_USB=y
CONFIG_USB_DEVICE_STACK=y
CONFIG_USB_CDC_ACM=y
CONFIG_USB_CDC_ACM_RINGBUF_SIZE=1024
CONFIG_USB_CDC_ACM_DEVICE_NAME="CDC_ACM"
CONFIG_USB_CDC_ACM_DEVICE_COUNT=1

# Enable serial console
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_UART_LINE_CTRL=y

# Enable USB UART, and set the console device
CONFIG_UART_CONSOLE=y
CONFIG_USB_UART_CONSOLE=y
CONFIG_UART_CONSOLE_ON_DEV_NAME="CDC_ACM_0"
CONFIG_USB_UART_DTR_WAIT=n
```

## Viewing Logs

After flashing the updated ZMK image, the board should expose a USB CDC ACM device, that you can connect to and view the logs.

On Linux, this should be a device like `/dev/ttyACM0` and you can connect with `minicom` or `tio` as usual, e.g.:

```
sudo tio /dev/ttyACM0
```

From there, you should see the various log messages from ZMK and Zephyr, depending on which systems you have set to what log levels.
