---
id: troubleshooting
title: Troubleshooting
sidebar_title: Troubleshooting
---
### Summary

The following page provides suggestions for common errors that may occur during firmware compilation. If the information provided is insufficient to resolve the issue, feel free to seek out help from the [ZMK Discord](https://zmkfirmware.dev/community/discord/invite).

### CMake Error

An error along the lines of `CMake Error at (zmk directory)/zephyr/cmake/generic_toolchain.cmake:64 (include): include could not find load file:` during firmware compilation indicates that the Zephyr Environment Variables are not properly defined.
For more information, click [here](../docs/dev-setup#environment-variables).

### dtlib.DTError

An error along the lines of `dtlib.DTError: <board>.dts.pre.tmp:<line number>` during firmware compilation indicates an issue within the `<shield>.keymap` file.
This can be verified by checking the file in question, found in `mkdir/app/build`.

|       ![Example Error Screen](../docs/assets/troubleshooting/keymaps/errorscreen.png)                                             |
| :-------------------------------------------------------------------------------:                                                 |
|            An example of the dtlib.DTError when compiling an iris with the nice!nano while the keymap is not properly defined     |

After opening the `<board>.dts.pre.tmp:<line number>` and scrolling down to the referenced line, one can locate errors within their shield's keymap by checking if the referenced keycodes were properly converted into the correct [USB HID Usage ID](https://www.usb.org/document-library/hid-usage-tables-12).

|       ![Unhealthy Keymap Temp](../docs/assets/troubleshooting/keymaps/unhealthyEDIT.png)  |
| :-------------------------------------------------------------------------------:         |
|   An incorrectly defined keymap unable to compile. As shown in red, `&kp SPAC` is not a valid reference to the [USB HID Usage ID](https://www.usb.org/document-library/hid-usage-tables-12) used for "Keyboard Spacebar"         |

|  ![Healthy Keymap Temp](../docs/assets/troubleshooting/keymaps/healthyEDIT.png)  |
| :-------------------------------------------------------------------------------: |
|  A properly defined keymap with successful compilation. As shown in red, the corrected keycode (`&kp SPC`) references the proper Usage ID defined in the [USB HID Usage Tables](https://www.usb.org/document-library/hid-usage-tables-12)|
