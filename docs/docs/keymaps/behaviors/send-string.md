---
title: Send String Behavior
sidebar_label: Send String
---

## Summary

The send string behavior types a string of text when pressed.

## Behavior Definition

Each string you want to send must be defined as a new behavior in your keymap, then bound to a key. Each behavior must have the following properties:

```dts
compatible = "zmk,behavior-send-string";
#binding-cells = <0>;
text = "...";
```

For example, the following defines a `&hello_world` behavior that types `Hello, world!` when triggered:

```dts
/ {
    chosen {
        zmk,charmap = &charmap_us;
    };

    behaviors {
        hello_world: hello_world {
            compatible = "zmk,behavior-send-string";
            #binding-cells = <0>;
            text = "Hello, world!";
        };
    };

    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <&hello_world>;
        };
    };
};
```

:::caution[Character Maps]
You must also select a [character map](#character-maps) so ZMK knows which key to press for each character in the string.
:::

### Configuration

#### `text`

This sets the text to type. It can contain almost any text, however some characters require special escape sequences:

- Double quotes must be written as `\"`.
  - e.g. `text = "This is \"quoted\" text"` types `This is "quoted" text`.
- Backslashes must be written as `\\`.
  - e.g. `text = "C:\\Windows\\System32"` types `C:\Windows\System32`.
- `\t` will press <kbd>tab</kbd>.
- `\n` will press <kbd>enter</kbd>.
- `\x08` will press <kbd>backspace</kbd>.
- `\x7F` will press <kbd>delete</kbd>.

#### `charmap`

Selects the [character map](#character-maps) to use for mapping string characters to keys.

If this is not set, it defaults to the `zmk,charmap` chosen node.

#### `tap-ms` and `wait-ms`

The `tap-ms` property controls how long each key is held. The `wait-ms` property controls how long of a delay there is between key presses. These default to 5 and 0 ms respectively, but they can be increased if strings aren't being typed correctly (at the cost of typing them more slowly).

```dts
hello_world: hello_world {
    compatible = "zmk,behavior-send-string";
    #binding-cells = <0>;
    text = "Hello, world!";
    tap-ms = <15>;
    wait-ms = <5>;
};
```

You can also change the default values in [your `.conf` file](../../config/index.md) with the `CONFIG_ZMK_SEND_STRING_DEFAULT_TAP_MS` and `CONFIG_ZMK_SEND_STRING_DEFAULT_WAIT_MS` settings, e.g.:

```kconfig
CONFIG_ZMK_SEND_STRING_DEFAULT_TAP_MS=15
CONFIG_ZMK_SEND_STRING_DEFAULT_WAIT_MS=5
```

### Convenience C Macro

You can use the `ZMK_SEND_STRING(name, text)` macro to reduce the boilerplate when defining a new string.

For example...

```dts
/ {
    behaviors {
        ZMK_SEND_STRING(hello_world, "Hello, world!")
    };
};
```

...creates this behavior:

```dts
/ {
    behaviors {
        hello_world: hello_world {
            compatible = "zmk,behavior-send-string";
            #binding-cells = <0>;
            text = "Hello, world!";
        };
    };
};
```

You can also add a third parameter with extra properties such as [timing configuration](#tap-ms-and-wait-ms) and [character map selection](#character-maps):

```dts
/ {
    behaviors {
        ZMK_SEND_STRING(hello_world, "Hello, world!",
            tap-ms = <15>;
            wait-ms = <5>;
        )
    };
};
```

### Behavior Queue Limit

Send string behaviors use an internal queue to handle each key press and release. Adding a send string behavior to your keymap will increase the default size of the queue to 256. Each character queues one key press and one release, so this allows 128 characters to be queued.

If you need to send longer strings, you can change the size of this queue via the `CONFIG_ZMK_BEHAVIORS_QUEUE_SIZE` setting in your [`.conf` file](../../config/index.md). For example, `CONFIG_ZMK_BEHAVIORS_QUEUE_SIZE=512` would allow a string of 256 characters.

## Character Maps

You must select a character map for ZMK to know which key to press to type each character. ZMK provides one character map, `&charmap_us`, which is designed to work if your operating system is set to a US keyboard layout. If your OS is set to a different layout, you can [create a new character map](#creating-character-maps).

To set the character map to use by default, set the `zmk,charmap` chosen node:

```dts
/ {
    chosen {
        zmk,charmap = &charmap_us;
    };
};
```

You can also override this for individual send string behaviors with the `charmap` property:

```dts
/ {
    behaviors {
        ZMK_SEND_STRING(hello_world, "Hello, world!",
            charmap = <&charmap_us>;
        )
    };
};
```

:::note
Properties with a `-map` suffix have a special meaning in Zephyr, so the property is named `charmap` instead of `character-map`.
:::

### Creating Character Maps

If your OS is set to a non-US keyboard layout, you will need to create a matching character map.

Add a node to your keymap with the following properties:

```dts
/ {
    charmap_name: charmap_name {
        compatible = "zmk,character-map";
        behavior = <&kp>;
        map = <codepoint keycode>
            , <codepoint keycode>
            ...
            ;
    };
};
```

The `behavior` property selects the behavior that the key codes will be sent to. This will typically be `&kp`.

The `map` property is a list of pairs of values. The first value in each pair is the Unicode [code point](https://en.wikipedia.org/wiki/List_of_Unicode_characters) of a character, and the second value is the key code to send to `&kp` to type that character. Add a pair for every character that you want to use.

A character map for German might look like this (many characters are omitted from this example for brevity):

```dts
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>

#define DE_A A
...
#define DE_Y Z
#define DE_Z Y
#define DE_A_UMLAUT SQT
#define DE_O_UMLAUT SEMI
#define DE_U_UMLAUT LBKT
...

/ {
    charmap_de: charmap_de {
        compatible = "zmk,character-map";
        behavior = <&kp>;
        map = <0x08 BACKSPACE>
            ...
            , <0x20 SPACE>
            , <0x21 DE_EXCL>   // !
            , <0x22 DE_DQT>    // "
            ...
            , <0x41 LS(DE_A)>  // A
            , <0x42 LS(DE_B)>  // B
            ...
            , <0x59 LS(DE_Y)>  // Y
            , <0x5A LS(DE_Z)>  // Z
            ...
            , <0x61 DE_A>      // a
            , <0x62 DE_B>      // b
            ...
            , <0x79 DE_Y>      // y
            , <0x7A DE_Z>      // z
            , <0x7B DE_LBRC>   // [
            , <0x7C DE_PIPE>   // |
            , <0x7D DE_RBRC>   // ]
            , <0x7E DE_TILDE>  // ~
            , <0x7F DELETE>
            , <0xC4 LS(DE_A_UMLAUT)>  // Ä
            , <0xD6 LS(DE_O_UMLAUT)>  // Ö
            , <0xDC LS(DE_U_UMLAUT)>  // Ü
            , <0xE4 DE_A_UMLAUT>      // ä
            , <0xF6 DE_O_UMLAUT>      // ö
            , <0xFC DE_U_UMLAUT>      // ü
            ...
            ;
    };
};
```

Every character map should typically have the following mappings:

| Code point | Key code    |
| ---------- | ----------- |
| `0x08`     | `BACKSPACE` |
| `0x0A`     | `RETURN`    |
| `0x0B`     | `TAB`       |
| `0x20`     | `SPACE`     |
| `0x7F`     | `DELETE`    |

You can then select this character map by setting the chosen node to its node label:

```dts
/ {
    chosen {
        zmk,charmap = &charmap_de;
    };
};
```

If you want different strings to use different character maps (for example if you have different layers for different OS keyboard layouts), you can set a different `charmap` property on each send string behavior:

```dts
/ {
    behaviors {
        ZMK_SEND_STRING(qwerty_us, "qwerty",
            charmap = <&charmap_us>;
        )
        ZMK_SEND_STRING(qwerty_de, "qwerty",
            charmap = <&charmap_de>;
        )
    };
};
```

### Unmapped Characters

By default, if a string contains a character whose code point is not in the character map, that character will be ignored. If you add a `fallback-behavior` property which refers to a one-parameter behavior, then that behavior will be invoked with the unmapped code point as its parameter instead.

ZMK does not yet have a behavior for sending arbitrary Unicode characters, but once this is added, it could be used as follows:

```dts
&uni {
    // Unicode behavior configuration...
};

/ {
    charmap_de: charmap_de {
        compatible = "zmk,character-map";
        behavior = <&kp>;
        fallback-behavior = <&uni>;
        ...
    };
};
```
