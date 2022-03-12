---
title: Hardware Metadata Files
---

## Overview

ZMK makes use of an additional metadata YAML file for all boards and shields to provide high level information about the hardware to be incorporated into setup scripts/utilities, website hardware list, etc.

The naming convention for metadata files is `{item_id}.zmk.yml`, where the `item_id` is the board/shield identifier, including version information but excluding any optional split `_left`/`_right` suffix, e.g. `corne.zmk.yml` or `nrfmicro_11.zmk.yml`.

## Example File

Here is a sample `corne.zmk.yml` file from the repository:

```yaml
file_format: "1"
id: corne
name: Corne
type: shield
url: https://github.com/foostan/crkbd/
requires:
  - pro_micro
exposes:
  - i2c_oled
features:
  - keys
  - display
siblings:
  - corne_left
  - corne_right
```

## Schema

ZMK uses a [JSON Schema](https://github.com/zmkfirmware/zmk/blob/main/schema/hardware-metadata.schema.json) file to validate metadata files and sure all required properties are present, and all other optional properties provided conform to the expected format. You can validate all metadata files in the repository by running `west metadata check` from your configured ZMK repository.

## File Format

The first line of every metadata file should contain the file format. As of today, the only acceptable file format is `1`, which should be written like:

```yaml
file_format: "1"
```

:::note

ZMK plans to expand on the initial based set of metadata properties, while maintaining backwards compatibility. In particular, new _optional_ properties may be added to file format `1` as time progress, and when new set of properties is settled upon as being required moving forward, a new major version of the file format will be created to encompass those changes.

:::

### Item ID and Name

All metadata files should then contain two key pieces of identifying information, the `id` for the item, and the `name` which used a the human readable display name in various UI locations:

```yaml
id: corne
name: Corne
```

### Item Types

Each metadata file includes a `type` property uniquely identifying the type of item, be it `board`, `shield`, or the less frequently needed `interconnect` (which is used to document generic hardware interconnects like the Pro Micro format):

```yaml
type: shield
```

### URL

The `url` property should contain the canonical URL used to learn more about the board/shield, e.g. the main vendor website or GitHub repository for a given keyboard/controller:

```yaml
url: https://github.com/foostan/crkbd/
```

### Interconnect Requires/Exposes

For boards and shields, one of the key pieces of high level information is compatibility between the two items. In particular, a board usually exposes one ore more "interconnects", the physical location/type of connections available, and their assigned possible uses (e.g. GPIO, power, ground, i2c, etc). Similarly, a shield is usually designed around one (or sometimes more) "interconnects" that allow it to connect to one of those boards.

In ZMK, we encode both of those scenarios with the `exposes` and `requires` properties, respectively. For example, for a Corne shield that requires a Pro Micro compatible controller to function, and simultaneously exposes a four pin header to be used by standard i2c OLED modules, the metadata file contains:

```yaml
requires:
  - pro_micro
exposes:
  - i2c_oled
```

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
