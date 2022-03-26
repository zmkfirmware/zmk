# ZMK Fork Optimized for Sofle Keyboards

This is a zmk fork that improves encoder, display and underglow support of zmk.

While it's been optimized for and tested with a sofle choc keyboard, it can be benificial for and should work with any split keyboard that is using encoders and displays.

## It adds the following features and fixes:

* Adds underglow (used for backlight) support to Sofle shield ([PR #1188](https://github.com/zmkfirmware/zmk/pull/1188))
* Fixes split side encoder not working ([PR #728](https://github.com/zmkfirmware/zmk/pull/728))
* Fixes display not working if you toggle external power off and then on again ([Issue #674](https://github.com/zmkfirmware/zmk/issues/674))
* Adds automatic disabling and enabling of external power when USB is disconnected or connected ([PR #1184](https://github.com/zmkfirmware/zmk/pull/1184))
* Adds automatic disabling of backlight if the keyboard is idle ([PR #1179](https://github.com/zmkfirmware/zmk/pull/1179))

Most of these fixes and features have not made it into the official zmk yet, because they don't meet the (very resaonable and completely understandable) code standards of the zmk maintainers.

However, while these fixes and features may not meet the quality standards of the official project, they work well enough to be used until these features get properly implemented.

## How to use

### Adjust your zmk-config's west.yml

If you already have an existing sofle config, you can adjust your `config/west.yml` file to use this fork instead of the official version.

Use [this commit as an example](https://github.com/infused-kim/zmk-config-sofle/commit/6b770ebdb6ad505f102f6b157c5f354ae9c884d0) of how to do it.

Refer to this [.conf file](https://github.com/infused-kim/zmk-config-sofle/blob/main/config/sofle.conf) and this [.keymap file](https://github.com/infused-kim/zmk-config-sofle/blob/main/config/sofle.keymap) for config options.

### Use zmk-config-sofle

Alternatively, you can also use my [zmk-config-sofle repo](https://github.com/infused-kim/zmk-config-sofle) as a starting point for your config.
