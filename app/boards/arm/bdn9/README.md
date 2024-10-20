# Building ZMK for the BDN9

Some general notes/commands for building standard BDN9 layouts from the assembly documentation.

## Standard Build

```
west build -p -d build/bdn9 --board bdn9_rev2
```

## Encoder Notes

If you built your BDN9 with encoders, you'll need to change the following in your local BDN9 config or add them to the end of the file.

```
CONFIG_EC11=y
CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y
```

Then, you'll want to uncomment the necessary encoder lines in your `bdn9_rev2.keymap`:

```
&sensors {
     status = "okay";
     sensors = <&left_encoder &mid_encoder &right_encoder>;
};

&left_encoder { status = "okay"; };
&mid_encoder { status = "okay"; };
&right_encoder { status = "okay"; };
```

And then add the correct `sensor-bindings` array to each keymap layer, e.g.:

```
sensor-bindings = <&inc_dec_kp PG_UP PG_DN &inc_dec_kp M_VOLU M_VOLD &inc_dec_kp C_PREV C_NEXT>;
```
