---
title: Caps Word Behavior
sidebar_label: Caps Word
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

## Summary

The caps word behavior behaves similar to a caps lock, but will automatically deactivate when any key not in a continue list is pressed, or if the caps word key is pressed again. For smaller keyboards using [mod-taps](/docs/behaviors/mod-tap), this can help avoid repeated alternating holds when typing words in all caps.

The modifiers are applied only to to the alphabetic (`A` to `Z`) keycodes, to avoid automatically appliying them to numeric values, etc.

### Behavior Binding

- Reference: `&caps_word`

Example:

```
&caps_word
```

### Configuration

#### Continue List

By default, the caps word will remain active when any alphanumeric character or underscore (`UNDERSCORE`), backspace (`BACKSPACE`), or delete (`DELETE`) characters are pressed. Any other non-modifier keycode sent will turn off caps word. If you would like to override this, you can set a new array of keys in the `continue-list` property in your keymap:

```
&caps_word {
    continue-list = <UNDERSCORE MINUS>;
};

/ {
    keymap {
        ...
    };
};
```

#### Applied Modifier(s)

In addition, if you would like _multiple_ modifiers, instead of just `MOD_LSFT`, you can override the `mods` property:

```
&caps_word {
    mods = <MOD_LSFT | MOD_LALT>;
};

/ {
    keymap {
        ...
    };
};
```

#### OS-Mapped Layouts or Non-English Characters

By default, caps word will only capitalize alpha characters a-z. If you use an alternative layout (Colemak, Dvorak etc) _and_ are performing the mapping in the OS rather than in the firmware, then there are some additional configuration options needed.

As an example, take Colemak mapped in the OS, when you type "o", the keyboard thinks you are pressing `SEMICOLON`. For caps word to produce a capital "O" we need to configure caps word to both remain active _and apply the shift modifier_ when it sees `SEMICOLON`.

Likewise when you type ";" the keyboard thinks you are pressing `P`. If caps word is active and you type ";" we need to tell caps word to deactivate even though it sees the alpha keycode `P`.

To do this, there are two optional properties: `also-mod-list` is a list of (non-alpha) keycodes that caps word should also apply the shift modifier to. `break-list` handles the second case, and is a list of (alpha) keycodes that should deactivate caps word.

##### Example Configurations

<Tabs
defaultValue="os_colemak"
values={[
{label: 'Colemak', value: 'os_colemak'},
{label: 'Dvorak', value: 'os_dvorak'},
{label: 'Workman', value: 'os_workman'},
]}>

<TabItem value="os_colemak">

This will allow caps word to work as expected when using Colemak mapped in the OS. If you are mapping the Colemak layout in your keyboard firmware, there is no need to do this.

```dtsi title="For OS-mapped Colemak"
&caps_word {
    continue-list = <UNDERSCORE BACKSPACE DELETE SEMICOLON>; // <--- prevent SEMICOLON ("o") from deactivating
    also-mod-list = <SEMICOLON>;                             // <--- capitalize SEMICOLON ("o")
    break-list = <P>;                                        // <--- deactivate on P (";")
};

/ {
    keymap {
        ...
    };
};
```

</TabItem>

<TabItem value="os_dvorak">

This will allow caps word to work as expected when using Dvorak mapped in the OS. If you are mapping the Dvorak layout in your keyboard firmware, there is no need to do this.

```dtsi title="For OS-mapped Dvorak"
&caps_word {
    // Prevent from deactivating:          "_"      "s"   "w"  "v"  "z"
    continue-list = <BACKSPACE DELETE DOUBLE_QUOTES SEMI COMMA DOT SLASH>;
    also-mod-list = <SEMI COMMA DOT SLASH>; // <--- capitalize "s" "w" "v" "z"
    break-list = <Q W E Z>;                 // <--- deactivate on "'" "," "." ";"
};

/ {
    keymap {
        ...
    };
};
```

</TabItem>

<TabItem value="os_workman">

This will allow caps word to work as expected when using Workman mapped in the OS. If you are mapping the Workman layout in your keyboard firmware, there is no need to do this.

```dtsi title="For OS-mapped Workman"
&caps_word {
    continue-list = <UNDERSCORE BACKSPACE DELETE SEMICOLON>; // <--- prevent SEMICOLON ("i") from deactivating
    also-mod-list = <SEMICOLON>;                             // <--- capitalize SEMICOLON ("i")
    break-list = <P>;                                        // <--- deactivate on P (";")
};

/ {
    keymap {
        ...
    };
};
```

</TabItem>

</Tabs>

### Multiple Caps Breaks

If you want to use multiple caps breaks with different codes to break the caps, you can add additional caps words instances to use in your keymap:

```
/ {
    prog_caps: behavior_prog_caps_word {
        compatible = "zmk,behavior-caps-word";
        label = "PROG_CAPS";
        #binding-cells = <0>;
        continue-list = <UNDERSCORE>;
    };

    keymap {
        ...
    };
};
```
