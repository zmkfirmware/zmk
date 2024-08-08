### Features

Boards and shields should document the sets of hardware features found on them using the `features` array. There is a fixed enum of possible values to use here, which will be expanded over time. The current set of possible `features` values is:

- `keys` - Any board or shield that contains keyboard keys should include this feature. It is a central feature used to determine if we have a "complete combination" for ZMK to produce a keyboard firmware when performing setup.
- `display` - Indicates the hardware includes a display for use with the ZMK display functionality.
- `encoder` - Indicates the hardware contains one or more rotary encoders.
- `underglow` - Indicates the hardware includes underglow LEDs.
- `backlight` - Indicates the hardware includes backlight LEDs.
- `pointer` (future) - Used to indicate the hardware includes one or more pointer inputs, e.g. joystick, touchpad, or trackpoint.

### Siblings

The `siblings` array is used to identify multiple hardware items designed to be used together as one logical device. Right now, that primarily is used to identify the two halves of a split keyboard, but future enhancements will include more complicated and flexible combinations.

The array should contain the complete hardware IDs of the siblings that combine in the logical device, e.g. with the `corne.zmk.yml` file:

```yaml
id: corne
siblings:
  - corne_left
  - corne_right
```

Future versions of the metadata file format will be expanded to allow documenting any specifics of each sibling that are unique, e.g. if only the left side contains the `encoder` feature.

### Variants

Some keyboards may have multiple different configurations available to them. The `variants` array is used to document these options. For example, the `corne` keyboard comes in a 42-key version and a 36-key version which has the outer pinky columns snapped off.

```yaml
id: corne
variants:
  - "42-key"
  - "36-key"
```
