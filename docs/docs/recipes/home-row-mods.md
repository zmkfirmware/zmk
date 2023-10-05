---
title: Home Row Mods
sidebar_label: Home Row Mods
---

## Summary

Home Row Mods assigns a [modifier key](../../docs/codes/modifiers) to the 'hold' binding of a [hold-tap](../../docs/behaviors/hold-tap) for each key under your fingers when your hands rest naturally on the home row. The 'tap' binding of each hold-tap is the same as the original key. On a QWERTY layout, these keys would be: `A`, `S`, `D`, `F` (left hand) and `J`, `K`, `L`, `;` (right hand).

### Benefits

- Reduced finger movement
- Reduced load on pinky
- Smaller key count

### Difficulties

- Adoption period
- Rolls
- Inadvertent triggers
- Potentially reduced typing speed

## Implementation

### Ingredients

- [Hold-Tap](../../docs/behaviors/mod-tap)
- Configuration

### Steps

Add a new home row mods `hold-tap` to your keymap:

```
hrm: home-row-mods {
    compatible = "zmk,behavior-hold-tap";
    label = "HOME_ROW_MODS";
    #binding-cells = <2>;
    flavor = "tap-preferred";
    tapping-term-ms = <200>;
    quick-tap-ms = <125>;
    global-quick-tap;
    bindings = <&kp>, <&kp>;
};
```

Decide what order you want to for the modifiers. `GUI`, `ALT`, `CTRL`, `SHIFT` is a commonly used one. There are reasons for different orderings, and ultimate this is a matter of preference.

Use the new hold-tap to assign modifiers as the 'hold' binding for home row keys in your default layer, and the original key as the 'tap' binding. Mirror the hold-tap 'hold' bindings for the other hand. Example of the [Corne keymap](https://github.com/zmkfirmware/zmk/blob/main/app/boards/shields/corne/corne.keymap#L22-L25) with this applied:
```
   &kp TAB   &kp Q       &kp W       &kp E       &kp R        &kp T       &kp Y   &kp U        &kp I       &kp O       &kp P          &kp BSPC
   &kp LCTRL &hrm LGUI A &hrm LALT S &hrm LCTL D &hrm LSHFT F &kp G       &kp H   &hrm RSHFT J &hrm RCTL K &hrm RALT L &hrm RGUI SEMI &kp SQT
   &kp LSHFT &kp Z       &kp X       &kp C       &kp V        &kp B       &kp N   &kp M        &kp COMMA   &kp DOT     &kp FSLH       &kp ESC
                                     &kp LGUI    &mo 1        &kp SPACE   &kp RET &mo 2        &kp RALT
```

The bindings should be symmetrical for each hand. (TODO: explain why?)

### Troubleshooting
Try typing using this modified keymap. You may notice misfires caused by inadvertently rolling home row mod hold-tap keys, or other timing issues.

- Adjust the 'tapping-term-ms' value (under what circumstances?)
- Adjust the 'quick-tap-ms' value (under what circumstances?)
- Remove the 'global-quick-tap' setting (under what circumstances?)

## Variations

### Bottom Row Mods

Move the hold-tap bindings one row lower. This is a good alternative if no timing or other configuration adjustments succeed at preventing misfires.

### Different order for mods

Change the order of modifiers so that your stronger fingers are associated with the modifiers that _you_ use most frequently.

### Exclude same-hand key combinations

Configure the hold-tap so that a hold is not considered a hold if the next key is one assigned to the same hand. This can help reduce problems misfires from rolls.

### Avoid `RALT` due to unusual Windows behavior.

On Windows, the right Alt key operates differently from the left, and so if you are a Windows user you may want to use `LALT` for both hands.
