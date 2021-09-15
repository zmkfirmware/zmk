# Building ZMK for the Choconum

Some general notes/commands for building standard Choconum layouts from the assembly documentation.

## Standard Build

```
west build -p -d build/choconum --board choconum
```

## Flashing

```
west flash -d build/choconum
```

