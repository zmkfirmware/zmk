# Nucleo WB55CG dongle support for [Zephirum](http://zephirum.tuxfamily.org)

Zephirum is an ergo monoblock angled keyboards powered by a Nucleo
WB55CG USB dongle.

## Building ZMK firmware

```
west build -b nucleo_wb55cg_dongle -- -DSHIELD=zephirum
```
