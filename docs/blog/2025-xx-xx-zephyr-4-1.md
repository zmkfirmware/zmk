---
title: "Zephyr 4.1 Update"
authors: nmunnich
tags: [firmware, zephyr, core]
---

We're happy to announce that after a long wait, ZMK's `main` branch is now running [Zephyr 4.1](https://docs.zephyrproject.org/latest/releases/release-notes-4.1.html)!

<!-- truncate -->

Zephyr 4.1 is a large leap forward from our previous version of 3.5, featuring:

- Support for lots of new SoCs, boards, and shields, such as the WCH CH32V003, the Raspberry Pi Pico 2, and [many many more](https://docs.zephyrproject.org/4.1.0/boards/index.html#boards=).
- Hardware Model V2 (HWMv2), providing better support for SoCs which have multiple cores on the same chip, such as the nRF5340.
- Lots of new drivers for chips such as the nPM1300.

This was a very large undertaking, and a big congratulations and thanks to [petejohanson] is due for all of his hard work in making this possible.

After we have verified functionality, ironed out any major bugs, and given any third party module maintainers time to update their code, we will be releasing ZMK `v0.4` from this Zephyr version update.

**All** out-of-tree keyboards will need to be updated to use HWMv2. If you maintain such a keyboard, you can find instructions on doing so [below](#moving-to-hwmv2).

## Board Revisions

As part of this change, ZMK is now using board/shield revisions, rather than duplicate board/shield definitions. This means that instead of having e.g. `nice_nano`, and `nice_nano_v2`, we only have `nice_nano`, which by default points to the `2.0.0` revision. To point to the original Nice!Nano V1, you would need to use `nice_nano@1.0.0` where you would have previously used `nice_nano`. Of course, you could also put `nice_nano@2.0.0` if you wished to make that explicit, instead of merely replacing `nice_nano_v2` with `nice_nano`. Some boards, such as the `nrfmicro`, also have additional _board qualifiers_ such as the choice between multiple SoCs. Board qualifiers must always be specified, and do not have defaults. See [Zephyr's overview](https://docs.zephyrproject.org/4.1.0/hardware/porting/board_porting.html#board-terminology) for more information on board qualifiers. The below table provides an overview of some of the differences in in-tree boards we have in ZMK, and how they are selected in the new build system. The shorthand shows the minimum needed to build with a specific board, taking into account defaults.

<table>
  <thead>
    <tr>
      <th> Board/Shield</th> <th>New Board/Shield ID</th> <th>Correlation between old and new IDs </th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Nice!Nano</td> <td>`nice_nano`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`nice_nano`</td> <td>`nice_nano@1.0.0`</td> <td>`nice_nano@1`</td> </tr>
            <tr> <td>`nice_nano_v2`</td> <td>`nice_nano@2.0.0`</td> <td>`nice_nano`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>nRFMicro</td> <td>`nrfmicro/nrf52840`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`nrfmicro_11`</td> <td>`nrfmicro@1.1.0/nrf52840`</td> <td>`nrfmicro@1.1/nrf52840`</td> </tr>
            <tr> <td>`nrfmicro_11_flipped`</td> <td>`nrfmicro@1.1.0/nrf52840/flipped`</td> <td>`nrfmicro@1.1/nrf52840/flipped`</td> </tr>
            <tr> <td>`nrfmicro_13`</td> <td>`nrfmicro@1.3.0/nrf52840`</td> <td>`nrfmicro/nrf52840`</td></tr>
            <tr> <td>`nrfmicro_13_52833`</td> <td>`nrfmicro@1.3.0/nrf52833`</td><td>`nrfmicro/nrf52833`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>Mikoto</td> <td>`mikoto`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`mikoto`</td> <td>`mikoto@5.20.0`</td> <td>`mikoto`</td> </tr>
            <tr> <td>`mikoto@6.1`</td> <td>`mikoto@6.1.0`</td> <td>`mikoto@6`</td> </tr>
            <tr> <td>`mikoto@7.2`</td> <td>`mikoto@7.2.0`</td> <td>`mikoto@7`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>XIAO rp2040</td> <td>`xiao_rp2040`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`seeeduino_xiao_rp2040`</td> <td>`xiao_rp2040`</td> <td>`xiao_rp2040`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>XIAO BLE</td> <td>`xiao_ble`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`seeeduino_xiao_ble`</td> <td>`xiao_ble`</td> <td>`xiao_ble`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>bt60</td> <td>`bt60`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`bt60_v1`</td> <td>`bt60@1.0.0`</td> <td>`bt60@1.0.0`</td> </tr>
            <tr> <td>`bt60_v2`</td> <td>`bt60@2.0.0`</td> <td>`bt60@2.0.0`</td> </tr>
            <tr> <td>`bt60_hs`</td> <td>`bt60_hs`</td> <td>`bt60_hs`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>planck</td> <td>`planck`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`planck_rev6`</td> <td>`planck`</td> <td>`planck`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>bdn9</td> <td>`bdn9`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`bdn9_rev2`</td> <td>`bdn9`</td> <td>`bdn9`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
    <tr>
      <td>ferris</td> <td>`ferris`</td>
      <td>
        <table>
          <thead>
            <tr> <th>Old</th> <th>New</th> <th>Short</th> </tr>
          </thead>
          <tbody>
            <tr> <td>`ferris_rev02`</td> <td>`ferris`</td> <td>`ferris`</td> </tr>
          </tbody>
        </table>
      </td>
    </tr>
  </tbody>
</table>

The boards above are those which have changed in ZMK's tree, with the addition of the very popular XIAO series. For other boards in Zephyr's tree, please refer to the Zephyr documentation or source files directly.

## Getting The Changes

### User Config Repositories Using GitHub Actions

Existing user repositories which are currently running ZMK's `main` branch will receive the changes automatically when rebuilding.

Any user repositories created on or after `2025-07-03` are currently pinned to the most recent ZMK release. You will need to change over to `main` to get these changes immediately, or wait for `v0.4` and upgrade your version then.
See the recent [blog post on pinning ZMK versions](./2025-06-20-pinned-zmk.md) for more information.

Likewise, if you are currently on `main` but do not wish to upgrade yet, please pin your ZMK version to `v0.3` by following the instructions in said blog post.

### VS Code & Docker (Dev Container)

If you build locally using VS Code & Docker then:

- Pull the latest ZMK `main` with `git pull` for your ZMK checkout
- Reload the project
- If you are prompted to rebuild the remote container, click `Rebuild`
- Otherwise, press `F1` and run `Remote Containers: Rebuild Container`
- Once the container has rebuilt and reloaded, run `west update` to pull the updated Zephyr version and its dependencies.

Once the container has rebuilt, VS Code will be running the 4.1 Docker image.

### Local Host Development

The following steps will get you building ZMK locally against Zephyr 4.1:

- Run the updated [toolchain installation](/docs/development/local-toolchain/setup) steps, and once completed, remove the previously installed SDK version (optional, existing SDK should still work)
- Install the latest version of `west` by running `pip3 install --user --update west`.
- Pull the latest ZMK `main` with `git pull` for your ZMK checkout
- Run `west update` to pull the updated Zephyr version and its dependencies

From there, you should be ready to build as normal!

## Moving To HWMv2

The move to HWMv2 has already been completed for all boards/shields in ZMK's `main` branch. For out-of-tree boards/shields, the following changes are necessary:

### Migrating an Out-Of-Tree Board

#### Write a `board.yml`

In your board's folder, next to other files such as `<your_board>.dts`, add a file called `board.yml`. The contents of this file should have the following shape:

```yml title="board.yml"
board:
  name: <board-name>
  vendor: <board-vendor>
  revision:
    format: <major.minor.patch|letter|number|custom>
    default: <default-revision-value>
    exact: <true|false>
    revisions:
    - name: <revA>
    - name: <revB>
      ...
  socs:
  - name: <soc-1>
    variants:
    - name: <variant-1>
    - name: <variant-2>
      variants:
      - name: <sub-variant-2-1>
        ...
  - name: <soc-2>
    ...
```

In the above:

- `<board-name>` is the name of the board as specified when selecting a build target, such as `nice_nano`.
- `<vendor-name` is the name of the board's vendor, such as `nicekeyboards`. If you are an individual, rather than acting as an organisation, please use your name/online id/similar (e.g. `zhiayang` in the case of the `mikoto`).
- `revision` defines any board revisions. See [Zephyr's overview](https://docs.zephyrproject.org/4.1.0/hardware/porting/board_porting.html#multiple-board-revisions) for more information on board revisions. If your board does not have any revisions, you can omit this section.
- `socs` lists all SoCs that your board could have, e.g. `nrf52840` or `stm32f072xb`. If your board only has one SoC available and no variants, then the SoC can be omitted when selecting a build target, but must still be specified in this file. For an understanding of SoC variants, refer to the Zephyr documentation.

If you need to define multiple boards in the same `board.yml`, such as for a split keyboard, you can do so like this:

```yml
boards:
 - name: <board_name_1>
   ...
 - name: <board_name_2>
   ...
```

#### Move Your Board Folder

Previously, your board folder should have had a filepath similar to `boards/arm/<board>`. Move your board to `boards/<vendor>/<board>`, where `<vendor>` matches the vendor specified in `board.yml`.

#### Revision Adjustments

If, as a side effect of adding revisions, you renamed the board (e.g. `ferris_rev02` -> `ferris`), you should adjust the other places where the board name was previously - `<board>.zmk.yml` and `<board>.yaml`. You may also need to rearrange/consolidate other Kconfig flags and devicetree nodes. See [the Zephyr documentation](https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html#multiple-board-revisions) for more details.

#### Adjust Kconfig Files

##### `Kconfig.<board>`

Previously, your board folder will have had a file named `Kconfig.board`. This should be renamed to `Kconfig.<board>`, where `<board>` is the board name given in `board.yml`. The contents of this file will previously look something like this:

```title="Kconfig.board"
config BOARD_FERRIS
    bool "Ferris rev 0.2"
    depends on SOC_STM32F072XB
```

Remove the `bool` and change the `depends on` to a `select`:

```title="Kconfig.ferris"
config BOARD_FERRIS
    select SOC_STM32F072XB
```

If you had multiple boards specified for different SoCs, you should consolidate them to one:

```title="Kconfig.nrfmicro"
config BOARD_NRFMICRO
    select SOC_NRF52840_QIAA if BOARD_NRFMICRO_NRF52840
    select SOC_NRF52840_QIAA if BOARD_NRFMICRO_NRF52840_FLIPPED
    select SOC_NRF52833_QIAA if BOARD_NRFMICRO_NRF52833
```

##### `<board>_defconfig`

Previously, this file was used to select the board and SOC with Kconfig flags. All such selections should be removed from this file. For example, all of the below flags should be removed:

```
CONFIG_SOC_SERIES_NRF52X=y
CONFIG_SOC_NRF52833_QIAA=y
CONFIG_SOC_NRF52840_QIAA=y
CONFIG_BOARD_<BOARDNAME>=y
CONFIG_SOC_SERIES_STM32F0X=y
CONFIG_SOC_STM32F072XB=y
```

In addition, if your board uses RGB underglow, the following Kconfig flag should be removed as well:

```
CONFIG_WS2812_STRIP=y
```

If your board makes use of no other SPI device, also remove the flag enabling SPI. It will be automatically re-enabled.

#### DeviceTree Changes

For most boards, aside from rearranging due to moving to revisions, there should be no changes necessary to the devicetree nodes. However, if your board makes use of upstream Zephyr drivers, these may have been renamed (e.g. Ferris' `microchip,mcp230xx` has been changed to `microchip,mcp23017`).

#### Bootloader Setup

With the version bump, the previous method to enable `&bootloader` has been disabled. Instead, ZMK is introducing _boot retention_, which as a side effect also enables `&bootloader` for SoCs which previously didn't work with said behavior, such as the STM32F072. To set up boot retention for your board, please read through [the dedicated page](/docs/development/hardware-integration/boot-retention).

### Changes To Out-Of-Tree Shields

#### Kconfig Changes

If your shield uses RGB underglow, the following Kconfig flag which was previously enabled should now be removed:

```
CONFIG_WS2812_STRIP=y
```

If this is the only SPI device your shield uses, also remove the Kconfig flag enabling SPI (assuming it is present). It will be automatically re-enabled.

### Other Changes

LVGL was updated to 9.3.0, which comes with breaking API changes. If you are using custom widgets or displays from a module, these will likely need fixing. See the [LVGL changelog](https://docs.lvgl.io/master/CHANGELOG.html#v9-3-0-3-june-2025) for details.

## Thanks!

Thanks to all the testers who have helped verify ZMK functionality on the newer Zephyr version.

[nmunnich]: https://github.com/nmunnich
[petejohanson]: https://github.com/petejohanson
