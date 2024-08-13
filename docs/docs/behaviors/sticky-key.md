---
title: Sticky Key Behavior
sidebar_label: Sticky Key
---

## Summary

A sticky key stays pressed until another key is pressed. It is often used for 'sticky shift'. By using a sticky shift, you don't have to hold the shift key to write a capital.

### Behavior Binding

- Reference: `&sk`
- Parameter #1: The keycode , e.g. `LSHIFT`

Example:

```dts
&sk LSHIFT
```

You can use any keycode that works for `&kp` as parameter to `&sk`:

```dts
&sk LG(LS(LA(LCTRL)))
```

### Configuration

#### `release-after-ms`

By default, sticky keys stay pressed for a second if you don't press any other key. You can configure this with the `release-after-ms` setting.

#### `quick-release`

Some typists may find that using a sticky shift key interspersed with rapid typing results in two or more capitalized letters instead of one. This happens as the sticky key is active until the next key is released, under which other keys may be pressed and will receive the modifier. You can enable the `quick-release` setting to instead deactivate the sticky key on the next key being pressed, as opposed to released.

#### `lazy`

By default, sticky keys are activated on press until another key is pressed. You can enable the `lazy` setting to instead activate the sticky key right _before_ the other key is pressed. This is useful for mouse interaction or situations where you don't want the host to see anything during a sticky-key timeout, for example `&sk LGUI`, which can trigger a menu if pressed alone.

Note that tapping a lazy sticky key will not trigger other behaviors such as the release of other sticky keys or layers. If you want to use a lazy sticky key to activate the release of a sticky layer, potential solutions include wrappping the sticky key in a simple macro which presses the sticky behavior around the sticky key press, doing the same with `&mo LAYER`, or triggering a tap of some key like `K_CANCEL` on sticky key press.

#### `ignore-modifiers`

This setting is enabled by default. It ensures that if a sticky key modifier is pressed before a previously pressed sticky key is released, the modifiers will get combined so you can add more sticky keys or press a regular key to apply the modifiers. This is to accommodate _callum-style mods_ where you are prone to rolling sticky keys. If you want sticky key modifiers to only chain after release, you can disable this setting. Please note that activating multiple modifiers via [modifier functions](https://zmk.dev/docs/codes/modifiers#modifier-functions) such as `&sk LS(LALT)`, require `ignore-modifiers` enabled in order to function properly.

#### Example

```dts
&sk {
    release-after-ms = <2000>;
    quick-release;
    /delete-property/ ignore-modifiers;
};

/ {
    keymap {
        ...
    };
};
```

This configuration would apply to all sticky keys. This may not be appropriate if using `quick-release` as you'll lose the ability to chain sticky key modifiers. A better approach for this use case would be to create a new behavior:

```dts
/ {
    behaviors {
      skq: sticky_key_quick_release {
        compatible = "zmk,behavior-sticky-key";
        #binding-cells = <1>;
        bindings = <&kp>;
        release-after-ms = <1000>;
        quick-release;
        ignore-modifiers;
      };
    };

    keymap {
        ...
    };
};
```

### Advanced Usage

Sticky keys can be combined; if you tap `&sk LCTRL` and then `&sk LSHIFT` and then `&kp A`, the output will be ctrl+shift+a.

### Comparison to QMK

In QMK, sticky keys are known as 'one shot mods'.
