# Building ZMK for the Nibble

Some general notes/commands for building standard nibble layouts from the assembly documentation.

## LED Notes

If you built your nibble without the LEDs _and_ are using a nice!nano board, you'll need to change the following in your local nibble config or add them to the end of the file.

```
CONFIG_ZMK_RGB_UNDERGLOW=n
CONFIG_WS2812_STRIP=n
```

## Encoder Notes

If you built your nibble without an encoder, you'll need to change the following in your local nibble config or add them to the end of the file.

```
CONFIG_EC11=n
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=n
```

## OLED Builds

If using an OLED screen, you'll need to change the following in your local nibble config or add them to the end of the file.

```
CONFIG_ZMK_DISPLAY=y
```
