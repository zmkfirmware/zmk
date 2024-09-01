# NRF52 DK

This is a sample self-contained ZMK board that is built for the ubiquitous NRF52 DK from Nordic. This can help jumpstart new board projects for people by showcasing various features on a known standard

You should be able to use this by plugging in an NRF52 DK, turning on the power switch, then running the following from the zmk/app folder:

````
west build --pristine -b nrf52dk_nrf52832  && west flash
````

## Features

- BLE functionality
- Simple keymap with modifiers
- Direct GPIO kscan
- Additonal custom code alongside ZMK runtime
- Connection LED

##  Other details

Many of the files are from zmk/zepjhyr/boards/arm/nrf52dk_nrf52832, with additional modifications for ZMK.