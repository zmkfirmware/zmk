# Building ZMK for the Tidbit

Some general notes/commands for building standard tidbit layouts from the assembly documentation.

## Standard "Non Dense" Build

```
west build -p -d build/tidbit/default --board nice_nano -- -DSHIELD=tidbit
```

## Dense "19 keys" Build

```
west build -p -d build/tidbit/19_key --board nice_nano -- -DSHIELD=tidbit_19key
```

## LED Notes

If you built your tidbit without the LEDs _and_ are using a nice!nano board, you'll need to change the following in your local tidbit config or add them to the end of the file.

```
CONFIG_ZMK_RGB_UNDERGLOW=n
CONFIG_WS2812_STRIP=n
```

## Encoder Notes

If you built your tidbit without encoders, you'll need to change the following in your local tidbit config or add them to the end of the file.

```
CONFIG_EC11=n
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=n
```

## OLED Builds

If using an OLED screen, you'll need to change the following in your local tidbit config or add them to the end of the file.

```
CONFIG_ZMK_DISPLAY=y
```
