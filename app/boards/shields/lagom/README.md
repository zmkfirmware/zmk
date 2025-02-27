# Building ZMK for the Lagom

Some general notes/commands for building standard lagom layout.

## Encoder Notes

If you built your lagom without an encoder, you'll need to change the following in your local lagom config or add them to the end of the file.

```
CONFIG_EC11=n
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=n
```

## OLED Notes

If are not using an OLED screen, you'll need to change the following in your local lagom config or add them to the end of the file.

```
CONFIG_ZMK_DISPLAY=n
```
