=================
Dactyl Manuform 
=================

This shield is basing on this project:
https://github.com/abstracthat/dactyl-manuform

-------------------
Board
-------------------

I ported the Dactyl Manuform, where instead of using Pro Pico board I picked Particle Xenon.
This board is quite cheap and easy to find in many stores. 
Other supported boards was too pricy or unavailable in my country.

^^^^^^^^^^^^^^^^^^^
Bootloader
^^^^^^^^^^^^^^^^^^^

To make it works you need to flash your Particle with:
https://github.com/adafruit/Adafruit_nRF52_Bootloader

Why Adafruit bootloader you may ask?

* MCUBOOT:
  This bootloader is hard to use and require installation of quite big amount of tools.
* Particle:
  Default bootloader in other hands, require special version of nrftool and you need to use more then one button to trigger boot mode.
  It should support binary generated here.
* Adafruit:
  It has all MCUBOOT functionality and also support UF2 binary format.

Here you can find the step by step tutorial how to flash it.
https://docs.particle.io/tutorials/learn-more/xenon-circuit-python/
https://learn.adafruit.com/circuitpython-on-the-nrf52/nrf52840-bootloader

-------------------
Building
-------------------

west build -p auto -b particle_xenon  -- -DSHIELD='dactyl_manuform' 

-----------------------
Flashing with Adafruit
-----------------------

To flash device you need to trigger dfu mode on this bootloader. To do, so double click reset button.

adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application build/zephyr/zmk.hex dfu-package.zip

adafruit-nrfutil dfu serial --package dfu-package.zip -p /dev/ttyACM0 -b 115200
