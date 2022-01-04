<!-- If you're adding a board/shield please fill out this check-list, otherwise you can delete it -->
## Board/Shield Check-list
 - [ ] This board/shield is tested working on real hardware
 - [ ] Definitions follow the general style of other shields/boards upstream ([Reference](https://zmk.dev/docs/development/new-shield))
 - [ ] `.zmk.yml` metadata file added
 - [ ] Proper Copyright + License headers added to applicable files (Generally, we stick to "The ZMK Contributors" for copyrights to help avoid churn when files get edited)
 - [ ] General consistent formatting of DeviceTree files
 - [ ] Keymaps do not use deprecated key defines (Check using the [upgrader tool](https://zmk.dev/docs/codes/keymap-upgrader))
 - [ ] `&pro_micro` used in favor of `&pro_micro_d/a` if applicable
 - [ ] If split, no name added for the right/peripheral half
 - [ ] Kconfig.defconfig file correctly wraps *all* configuration in conditional on the shield symbol
 - [ ] `.conf` file has optional extra features commented out
 - [ ] Keyboard/PCB is part of a shipped group buy or is generally available in stock to purchase (OSH/personal projects without general availability should create a zmk-config repo instead)
