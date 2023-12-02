---
title: "Community Spotlight Series #2: Node-free Config"
author: Cem Aksoylar
author_title: Documentation maintainer
author_url: https://github.com/caksoylar
author_image_url: https://avatars.githubusercontent.com/u/7876996
tags: [keyboards, firmware, community]
---

This blog continues our series of posts where we highlight projects within the ZMK ecosystem
that we think are interesting and that the users might benefit from knowing about them. You might
be aware that ZMK configurations in the [Devicetree format](/docs/config#devicetree-files)
use the [C preprocessor](https://en.wikipedia.org/wiki/C_preprocessor) so that directives like
`#define RAISE 2` or `#include <behaviors.dtsi>` can be used in them. In this installment we are
highlighting the [`zmk-nodefree-config` project](https://github.com/urob/zmk-nodefree-config)
by [urob](https://github.com/urob) that contains helper methods that utilizes this fact
for users who prefer editing and maintaining their ZMK config directly using the Devicetree
syntax format.

In the rest of the post we leave it to urob to introduce and explain the motivations of the
project, and various ways it can be used to help maintain ZMK keymaps. Stay tuned for future
installments in the series!

## Overview

Loosely speaking the _nodefree_ repo -- more on the name later -- is a
collection of helper functions that simplify configuring keymap files. Unlike
the graphical keymap editor covered in the [previous spotlight
post](https://zmk.dev/blog/2023/11/09/keymap-editor), it is aimed at users who
edit and maintain directly the source code of their keymap files.

The provided helpers fall into roughly one of three categories:

1. Helpers that eliminate boilerplate, reduce the complexity of keymaps, and improve readability.
2. Helpers that improve portability of "position-based" properties such as combos.
3. Helpers that define international and other unicode characters.

The reminder of this post details each of these three categories.

## Eliminating Boilerplate

In ZMK, keymaps are configured using so-called _Devicetree_ files. Devicetree files
define a collection of nested _nodes_, whereas each node in turn specifies a variety of
_properties_ through which one can customize the keymap.

For example, the following snippet sets up a
[mod-morph](https://zmk.dev/docs/behaviors/mod-morph) behavior that sends <kbd>.</kbd>
("dot") when pressed by itself and sends <kbd>:</kbd> ("colon") when shifted:

```dts {6-7} showLineNumbers
/ {
    behaviors {
        dot_colon: dot_colon_behavior {
            compatible = "zmk,behavior-mod-morph";
            #binding-cells = <0>;
            bindings = <&kp DOT>, <&kp COLON>;
            mods = <(MOD_LSFT|MOD_RSFT)>;
        };
    };
};
```

Adding this snippet to the keymap will create a new node `dot_colon_behavior`
(nested underneath the `behaviors` and root `/` nodes), and assigns it four
properties (`compatible`, `#binding-cells`, etc). Here, the crucial properties are `bindings`
and `mods`, which spell out the actual functionality of the new behavior. The rest
of the snippet (including the nested node-structure) is boilerplate.

The idea of the _nodefree_ repo is to use C preprocessor macros to improve
readability by eliminating as much boilerplate as possible. Besides hiding
redundant behavior properties from the user, it also automatically creates and
nests all required behavior nodes, making for a "node-free" and less
error-prone user experience (hence the name of the repo).

For example, using `ZMK_BEHAVIOR`, one of the repo's helper functions, the
above snippet simplifies to:

```dts showLineNumbers
ZMK_BEHAVIOR(dot_colon, mod_morph,
    bindings = <&kp DOT>, <&kp COLON>;
    mods = <(MOD_LSFT|MOD_RSFT)>;
)
```

For complex keymap files, the gains from eliminating boilerplate can be
enormous. To provide a benchmark, consider my [personal
config](https://github.com/urob/zmk-config), which uses the _nodefree_ repo to
create various behaviors, set up combos, and add layers to the keymap. Without
the _nodefree_ helpers, the total size of my keymap would have been 41 kB. Using
the helper macros, the actual size is instead reduced to a more sane 12 kB.[^1]

[^1]:
    To compute the impact on file size, I ran `pcpp
--passthru-unfound-includes` on the `base.keymap` file, comparing two
    variants. First, I ran the pre-processor on the actual file. Second, I ran
    it on a version where I commented out all the _nodefree_ headers,
    preventing any of the helper functions from getting expanded. The
    difference isolates precisely the size gains from eliminating boilerplate,
    which in my ZMK config are especially large due to a vast number of
    behaviors used to add various Unicode characters to my keymap.

## Simplifying "Position-based" Behaviors

In ZMK, there are several features that are position-based. As of today, these
are [combos](/docs/features/combos) and [positional
hold-taps](/docs/behaviors/hold-tap#positional-hold-tap-and-hold-trigger-key-positions),
with behaviors like the ["Swapper"](https://github.com/zmkfirmware/zmk/pull/1366) and [Leader
key](https://github.com/zmkfirmware/zmk/pull/1380) currently
developed by [Nick Conway](https://github.com/nickconway) in pull requests also utilizing them.

Configuring these behaviors involves lots of key counting, which can be
cumbersome and error-prone, especially on larger keyboards. It also reduces the
portability of configuration files across keyboards with different layouts.

To facilitate configuring position-based behaviors, the _nodefree_ repo comes
with a community-maintained library of "key-position labels" for a variety of
popular layouts. The idea is to provide a standardized naming convention that
is consistent across different keyboards. For instance, the labels for a 36-key
layout are as follows:

```
    ╭─────────────────────┬─────────────────────╮
    │ LT4 LT3 LT2 LT1 LT0 │ RT0 RT1 RT2 RT3 RT4 │
    │ LM4 LM3 LM2 LM1 LM0 │ RM0 RM1 RM2 RM3 RM4 │
    │ LB4 LB3 LB2 LB1 LB0 │ RB0 RB1 RB2 RB3 RB4 │
    ╰───────╮ LH2 LH1 LH0 │ RH0 RH1 RH2 ╭───────╯
            ╰─────────────┴─────────────╯
```

The labels are all of the following form:

- `L/R` for **L**eft/**R**ight side
- `T/M/B/H` for **T**op/**M**iddle/**B**ottom and t**H**umb row.
- `0/1/2/3/4` for the finger position, counting from the inside to the outside

The library currently contains definitions for 17 physical
layouts, ranging from the tiny [Osprette](https://github.com/smores56/osprette) to the large-ish
[Glove80](https://www.moergo.com/collections/glove80-keyboards).
While some of these layouts contain more keys than others, the idea behind the
library is that keys that for all practical purposes are in the "same" location
share the same label. That is, the 3 rows containing the alpha keys are
always labeled `T/M/B` with `LM1` and `RM1` defining the home position of
the index fingers. For larger boards, the numbers row is always labeled
`N`. For even larger boards, the function key row and the row below `B` are
labeled `C` and `F` (mnemonics for **C**eiling and **F**loor), etc.

Besides sparing the user from counting keys, the library also makes it easy to
port an entire, say, combo configuration from one keyboard to the next by simply
switching layout headers.

## Unicode and International Keycodes

The final category of helpers is targeted at people who wish to type international characters
without switching the input language of their operation system. To do so, the repo comes with
helper functions that can be used to define Unicode behaviors.

In addition, the repo also ships with a community-maintained library of
language-files that define Unicode behaviors for all relevant characters in a
given language. For instance, after loading the German language file, one can
add `&de_ae` to the keymap, which will send <kbd>ä</kbd>/<kbd>Ä</kbd> when pressed or shifted.

## About Me

My path to ZMK and programmable keyboards started in the early pandemic, when I
built a [Katana60](https://geekhack.org/index.php?topic=88719.0) and learned
how to touch-type Colemak. Soon after I purchased a Planck, which turned out
to be the real gateway drug for me.

Committed to making the best out of the Planck's 48 keys, I have since
discovered my love for tinkering with tiny layouts and finding new ways of
[squeezing out](https://xkcd.com/2583/) a bit more ergonomics. Along the way, I
also made the switch from QMK to ZMK, whose "object-oriented" approach to
behaviors I found more appealing for complex keymaps.[^2]

[^2]:
    I am using the term object-oriented somewhat loosely here. What I mean by
    that is the differentiation between abstract behavior classes (such as
    hold-taps) and specific behavior instances that are added to the keymap.
    Allowing to set up multiple, reusable instances of each behavior has been a
    _huge_ time-saver compared to QMK's more limited behavior settings that are
    either global or key-specific.

These days I mostly type on a Corne-ish Zen and are waiting for the day when I
will finally put together the
[Hypergolic](https://github.com/davidphilipbarr/hypergolic) that's been sitting
on my desk for months. My current keymap is designed for 34 keys, making
liberal use of combos and [timerless homerow
mods](https://github.com/urob/zmk-config#timeless-homerow-mods) to make up for
a lack of keys.
