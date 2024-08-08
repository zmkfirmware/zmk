### Interconnect Requires/Exposes

For boards and shields, one of the key pieces of high level information is compatibility between the two items. In particular, a board usually exposes one ore more "interconnects", the physical location/type of connections available, and their assigned possible uses (e.g. GPIO, power, ground, i2c, etc). Similarly, a shield is usually designed around one (or sometimes more) "interconnects" that allow it to connect to one of those boards.

In ZMK, we encode both of those scenarios with the `exposes` and `requires` properties, respectively. For example, for a Corne shield that requires a Pro Micro compatible controller to function, and simultaneously exposes a four pin header to be used by standard i2c OLED modules, the metadata file contains:

```yaml
requires:
  - pro_micro
exposes:
  - i2c_oled
```

### Manufacturer

The optional `manufacturer` property can be used to name the manufacturer/vendor/designer of a hardware device.
