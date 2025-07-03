# Building ZMK for the Ferris 0.2

## Standard Build

```
west build -p -d build/ferris --board ferris
```

## Flashing

`west` can be used to flash the board directly. Press the reset button once, and run:

```
west flash -d build/ferris
```
