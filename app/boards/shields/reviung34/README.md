# REVIUNG34

REVIUNG34 is a 33-34 key unibody split keyboard by [gtips](https://github.com/gtips). An in-stock version can be found at [Little Keyboards](https://www.littlekeyboards.com/products/reviung34-analyst-keyboard-kit).

By default, the 2x1u layout is used. To use to the 1x2u layout, add the following to your keymap:

```
/ {
    chosen {
        zmk,matrix-transform = &single_2u_transform;
    };
};
```
