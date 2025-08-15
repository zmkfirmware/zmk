---
title: Devicetree Overview
sidebar_label: Devicetree Overview
---

ZMK makes heavy usage of a type of [tree data structure](<https://en.wikipedia.org/wiki/Tree_(abstract_data_type)>) known as _devicetree_.
Devicetree is a _declarative_ way of describing almost everything about a Zephyr device, from the definition of keymaps and configuration of behaviors all the way to the internal storage partitions and architecture of the board's MCU.

This page is an introduction to devicetree for ZMK users and designers.
For further reading, refer to the [devicetree spec](https://github.com/devicetree-org/devicetree-specification/releases) and [Zephyr's documentation](https://docs.zephyrproject.org/latest/build/dts/index.html#devicetree-guide).

## Running Example

The following segment taken from a keymap will be used as a running example:

```dts
#include <dt-bindings/zmk/keys.h>
#include <behaviors.dtsi>

/ {
    behaviors {
        spc_ul: space_underscore {
            compatible = "zmk,behavior-mod-morph";
            #binding-cells = <0>;
            bindings = <&kp SPACE>, <&kp UNDERSCORE>;
            mods = <(MOD_LSFT|MOD_RSFT)>;
        };
    };
    keymap {
        compatible = "zmk,keymap";
        default_layer {
            bindings = <&spc_ul &kp Z &kp M &kp K>;
        };
    };
};
```

It may be helpful to open this page twice and leave one copy open at this example.
Note also that Devicetree uses C-style comments, i.e. `// ...` for line comments and `/* ... */` for block comments.

## Structure

A devicetree node has the general structure (parts within `[]` being optional)

```dts
[label:] name {
    [properties]
    [child nodes]
};
```

The root node of the devicetree always has the name `/`, i.e. is written as

```dts
 / {
    [child nodes]
 };
```

It is also the _only_ node which has the `/` character as a name. See the devicetree spec for permitted characters for node names.

After various preprocessing steps, all contents of the devicetree will be found within/under the root node.
If one node is found within another node, we say that the first node is a _child node_ of the second one. Similarly, one can also refer to a _grandchild node_, etc.

In the running example, `behaviors` and `keymap` are child nodes of the root node. `space_underscore` and `default_layer` are child nodes of `behaviors` and `keymap` respectively, making them both grandchild nodes of the root node.

### Properties

What properties a node may have varies drastically. Of the standard properties, there are two which are of particularly relevant to users and designers: `compatible` and `status`. Additional standard properties may be found in the [devicetree spec](https://github.com/devicetree-org/devicetree-specification/releases).

#### Property types

These are some of the property types you will see most often when working with ZMK. [Zephyr's Devicetree bindings documentation](https://docs.zephyrproject.org/3.5.0/build/dts/bindings.html) provides more detailed information and a full list of types.

##### bool

True or false. To set the property to true, list it with no value. To set it to false, do not list it.

Example: `property;`

If a property has already been set to true and you need to override it to false, use the following command to delete the existing property:

```dts
/delete-property/ the-property-name;
```

##### int

A single integer surrounded by angle brackets. Also supports mathematical expressions.

Example: `property = <42>;`

##### string

Text surrounded by double quotes.

Example: `property = "foo";`

##### array

A list of integers surrounded by angle brackets and separated with spaces. Mathematical expressions can be used but must be surrounded by parenthesis.

Example: `property = <1 2 3 4>;`

Values can also be split into multiple blocks, e.g. `property = <1 2>, <3 4>;`

##### phandle

A single node reference surrounded by angle brackets. Phandles will be explained in more detail in a [later section](#labels-and-phandles).

Example: `property = <&label>`

##### phandles

A list of node references surrounded by angle brackets. Phandles will be explained in more detail in a [later section](#labels-and-phandles).

Example: `property = <&label1 &label2 &label3>`

##### phandle array

A list of node references and possibly numbers to associate with the node. Mathematical expressions can be used but must be surrounded by parenthesis. Phandles will be explained in more detail in a [later section](#labels-and-phandles).

Example: `property = <&none &mo 1>;`

Values can also be split into multiple blocks, e.g. `property = <&none>, <&mo 1>;`

See the documentation for "phandle-array" in [Zephyr's Devicetree bindings documentation](https://docs.zephyrproject.org/3.5.0/build/dts/bindings.html)
for more details on how parameters are associated with nodes.

##### GPIO array

This is just a phandle array. The documentation lists this as a different type to make it clear which properties expect an array of GPIOs.

Each item in the array should be a label for a GPIO node (the names of which differ between hardware platforms) followed by an index and configuration flags. See [Zephyr's GPIO documentation](https://docs.zephyrproject.org/3.5.0/hardware/peripherals/gpio.html) for a full list of flags. Phandles and labels will be explained in more detail in a [later section](#labels-and-phandles).

Example:

```dts
some-gpios =
    <&gpio0 0 GPIO_ACTIVE_HIGH>,
    <&gpio0 1 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>
    ;
```

##### path

A path to a node, either as a node reference or as a string. This will be explained in more detail in a [later section](#labels-and-phandles).

Examples:

```dts
property = &label;
property = "/path/to/some/node";
```

#### Compatible

The most important property that a node has is generally the `compatible` property. This property is used to map code to nodes. There are some special cases, such as the node named `chosen`, where the node name is used rather than a `compatible` property.

In the running example, `space_underscore` has the property `compatible = "zmk,behavior-mod-morph";`. The [ZMK's mod-morph behavior code](https://github.com/zmkfirmware/zmk/blob/main/app/src/behaviors/behavior_mod_morph.c#L7) acts on all nodes with `compatible` set to this value. The [ZMK keymap code](https://github.com/zmkfirmware/zmk/blob/main/app/src/keymap.c#L29) acts similarly for `compatible = "zmk,keymap";`.

The `compatible` property is also used to identify what additional properties a node may have. Any properties which are not one of the standard properties must be listed in a "devicetree bindings" file. These files will sometimes also include some additional information on the usage of the node.

ZMK keeps all of its devicetree bindings under the [`app/dts/bindings` directory](https://github.com/zmkfirmware/zmk/tree/main/app/dts/bindings).

The bindings file for `compatible = "zmk,behavior-mod-morph";` is [`app/dts/bindings/behaviors/zmk,behavior-mod-morph.yaml`](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/behaviors/zmk%2Cbehavior-mod-morph.yaml).

```dts title="zmk,behavior-mod-morph.yaml"
# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

description: Mod Morph Behavior

compatible: "zmk,behavior-mod-morph"

include: zero_param.yaml

properties:
  bindings:
    type: phandle-array
    required: true
  mods:
    type: int
    required: true
  keep-mods:
    type: int
    required: false
```

The properties the node can have are listed under `properties`. Some additional properties are imported from [zero_param.yaml](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/behaviors/zero_param.yaml). Bindings files are **the authority** on node properties, with our [documentation of said properties](https://zmk.dev/docs/config/behaviors#devicetree-7) sometimes omitting things like the `#binding-cells` property (imported from the previously mentioned file, describing the number of parameters that the behavior accepts). A full description of the bindings file syntax can be found in [Zephyr's documentation](https://docs.zephyrproject.org/3.5.0/build/dts/bindings-syntax.html).

Note that binding files can also specify properties for children, like the [`zmk,keymap.yaml` bindings file](https://github.com/zmkfirmware/zmk/blob/main/app/dts/bindings/zmk%2Ckeymap.yaml) specifying properties for layers in the keymap.

#### Status

The `status` property simply describes the status of a node. For ZMK users and designers, there are only two relevant values that this could be set to:

- `status = "disabled";` The node is disabled. Code should not take effect or make use of the node, but it can still be referenced by other parts of the devicetree.
- `status = "okay";` The default setting when not explicitly stated. The node is treated as "active". This property is generally only explicitly stated when overwriting a `status = "disabled";`.

How this property is used in practice will become more clear after the [devicetree preprocessing](#devicetree-preprocessing) section later on.

### Labels and Phandles

In addition to _names_, nodes can also have _labels_. For the ZMK user/designer, labels are arguably more important than node names. Whereas node names are used within code to access individual nodes, labels are used to reference other nodes from within devicetree itself. Such a reference is called a _phandle_, and can be thought of as similar to a pointer in C.

In the running example, `spc_ul` is the label given to the node `space_underscore`. The `bindings` property of the `default_layer` node is a "phandle-array" - an array of references to other nodes[^1]. Its first element is `&spc_ul` - a phandle to the node with label `spc_ul`, i.e. `space_underscore`. `&kp` is another example of a phandle. It points to a node [defined as below](https://github.com/zmkfirmware/zmk/blob/main/app/dts/behaviors/key_press.dtsi):

```dts
/ {
    behaviors {
        kp: key_press {
            compatible = "zmk,behavior-key-press";
            #binding-cells = <1>;
            display-name = "Key Press";
        };
    };
};
```

This node is imported from a different file -- imports will be discussed later on. The `&kp` phandles found in the running example also show the concept of _parameters_ being passed to phandles. In this case, `Z`, `M`, and `K` are passed as parameters.

When ZMK needs to trigger a behavior found at a location in the keymap's `binding` property, it uses the phandle to identify the behavior node which needs to be called. It then executes the code determined by the `compatible` property of said node, passing in parameters while doing so[^2]. Depending on the behavior, another behavior phandle may need to be triggered, in which case the same process is used to identify the node and thus the parts of code which need to be executed.

Essentially, each layer in a keymap consists of an array of phandles pointing to various behaviors (alongside parameters) that were defined elsewhere. If you do not need to define the behavior node yourself, that just means ZMK has already defined it for you.

[^1]: A phandle array by definition also includes metadata, i.e. parameters. Strictly speaking, a list of phandles without metadata has type `phandles` rather than `phandle-array`. A property with a single phandle has type `phandle`.

[^2]: The number of parameters passed to the behavior code (and skipped over to find the next behavior phandle) is determined by the `#binding-cells` property mentioned above.

## Devicetree Preprocessing

Much of the complexity in `dts` files comes from preprocessing. The resulting devicetree after all preprocessing has finished [can be inspected](../troubleshooting/building-issues.md#devicetree-related-issues) for both GitHub Actions and local builds. For reasons that will make more sense later, your keymap and most of your customisations will be found near the bottom of the file.

Preprocessing comes from two sources:

1. The [C preprocessor](https://gcc.gnu.org/onlinedocs/cpp/) can be used within Devicetree Source (`dts`) files.
2. Devicetree has its own system for merging together, overwriting, and even deleting nodes and properties.

### C Preprocessor

An introduction to the C preprocessor is beyond the scope of this page. There are plenty of resources online for the unfamiliar reader to refer to.

However, some specific methods of how the C preprocessor is used in ZMK's devicetree files can be useful, to better understand how everything fits together.

The C preprocessor is used to import some nodes and other preprocessor definitions from other files. The lines

```dts
#include <dt-bindings/zmk/keys.h>
#include <behaviors.dtsi>
```

which are found at the top of the running example import the default behavior node definitions for ZMK, along with a list of preprocessor definitions. The parameters `Z`, `M`, and `K` (passed to the `&kp` phandle in the running example) are actually C preprocessor defines. For example, during preprocessing references to `Z` get turned into the number `0x07001D`, which is the number that gets passed to the ZMK host device (e.g. your computer) for it to then re-interpret as the letter "z".

The C preprocessor often gets leveraged by ZMK power users to reduce repetition in their keymap files. An example of this is the [macro-behavior convenience macro](../keymaps/behaviors/macros.md#convenience-c-macro). ZMK designers will also come across the `RC` macro used for matrix transformations, and make use of convenience defines such as `GPIO_ACTIVE_HIGH`.

### Devicetree Processing

A devicetree is almost always constructed from multiple files. These files are generally speaking:

- `.dtsi` files, which exist exclusively to be included via the C preprocessor (their contents get "pasted" at the location of the `#include` command) and are not used by the build sytem otherwise.
- A `.dts` file, which forms the "base" of the devicetree. A single one of these is always present when a devicetree is constructed. For ZMK, the `.dts` file contains the sections of the devicetree describing the [_board_](hardware-integration/index.mdx#what-is-a-board). This includes importing a number of `.dtsi` files describing the specific SoC that the board uses.
- Any number of `.overlay` files. These files can come from various sources, such as [shields](hardware-integration/index.mdx#what-is-a-shield) or [snippets](https://docs.zephyrproject.org/3.5.0/build/snippets/index.html). An overlay is applied to a `.dts` file by appending its contents to the end of the `.dts` file, i.e. it is placed at the bottom of the file. Multiple overlays are applied by doing so repeatedly in a particular order. Without going into the details of the exact order in which overlays are applied, it is enough to know that if you specify e.g. `shield: corne_left nice_view_adapter nice_view` in your `build.yaml`, then the overlays are applied left to right.
- A single `.keymap` file. This file being included is ZMK-specific, and is treated as the "final" `.overlay` file, appended after all other overlays.

#### Merging and overwriting nodes

When a node appears multiple times in the devicetree (after the files are imported and merged together), it gets merged into a single node as a preprocessing step. For example:

```dts
/ {
    mn_ex: my_example_node {
        property1 = <0>;
        property2 = <2>;
    };
};

/ {
    my_example_node {
        property2 = <1>;
        property3 = <4>;
    };

    example2 {
        property;
    };
};
```

The second appearance of `my_example_node` has priority, thus its `property2` value will overwrite the first appearance. The two root nodes also get merged in the process. The resulting tree after processing would be

```dts
/ {
    mn_ex: my_example_node {
        property1 = <0>;
        property2 = <1>;
        property3 = <4>;
    };

    example2 {
        property;
    };
};
```

Labels do not get overwritten; a node can have multiple labels. Phandles can also be used to overwrite or add properties:

```dts
&mn_ex {
    property4 = <2>;
};
```

The phandle approach is the recommended one, as one does not need to know the exact names of all the parent nodes with this approach. Crucially, when using phandles to overwrite or add properties, **the phandle must not be located within the root node**. It is instead placed outside of the tree entirely.

#### Special devicetree directives

Devicetree has some special directives that affect the tree. Relevant ones are:

- Nodes can be deleted with the /delete-node/ directive: `/delete-node/ &node_label;` outside of the root node.
- Properties can be deleted with the /delete-property/ directive: `/delete-property/ node-property;` inside the relevant node.
- `/omit-if-no-ref/` causes a node to be omitted from the resulting devicetree if there are no references/phandles to the node: `/omit-if-no-ref/ &node_label;`
