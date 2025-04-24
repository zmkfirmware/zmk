---
title: Module Creation
sidebar_label: ZMK Module Creation
---

[ZMK modules](../features/modules.mdx) are the recommended method of adding content to ZMK, whenever it is possible. This page will guide you through creating a module for ZMK. Distinction is made between modules used for different purposes:

- Modules containing one or more keyboard-related definitions (boards, shields, interconnects, etc.)
- Modules containing behaviors & features
- Modules containing drivers
- Modules containing other features, such as visual effects

See also Zephyr's [page on modules](https://docs.zephyrproject.org/3.5.0/develop/modules.html).

:::tip
For open source hardware designs, it can be convenient to use [Git submodules](https://github.blog/open-source/git/working-with-submodules/) to have the ZMK module also be a Git submodule of the repository hosting the hardware design.
:::

## Module Setup

ZMK has a template to make creating a module easier. Navigate to [the ZMK module template repository](https://github.com/zmkfirmware/zmk-module-template) and select "Use this template" followed by "Create a new repository" in the top right.

The rest of this page will go through the contents of the template step by step, guiding you through the configuration of the module.

:::tip
The [Unified ZMK Config Template](https://github.com/zmkfirmware/unified-zmk-config-template) is also a module, albeit one focused on keyboards. If you are making a module for a keyboard, you may find it useful to base the module off of the template and use GHA to help troubleshoot any errors. Later, you can extract the keyboard to a dedicated module or remove superfluous parts from your module with the help of the information on this page.
:::

### Zephyr Module File

This is the file that defines your module. Only when this file is configured correctly will your module function as expected.

#### Name

First, you will need to name your module. ZMK has a naming convention that you should follow:

```
zmk-<type>-<description>
```

Valid options for type are:

| Type        | Description                                                                                                           |
| ----------- | --------------------------------------------------------------------------------------------------------------------- |
| `keyboard`  | The module contains definitions for one or multiple keyboards, be they boards or shields.                             |
| `component` | The module contains definitions for one or multiple components, such as boards for shields to use.                    |
| `behavior`  | The module contains a single behavior.                                                                                |
| `driver`    | The module contains a single driver.                                                                                  |
| `feature`   | The module contains a single feature which doesn't fall under the other categories.                                   |
| `vfx`       | The module contains definitions for one or multiple visual effects, such as display customisations or RGB animations. |

For each of these, you can use the singular or plural, i.e. `keyboard` and `keyboards` are both acceptable. Use your judgement to decide between the two.

`<description>` is your choice of name to describe the module.

Note that your _module name_ does not have to be the same as your _repository name_. For example, if you have named your module `zmk-keyboards-corne`, you may for your own organisational purposes want to name the repository e.g. `zmk-corne-module`.

Your module name goes in the `zephyr/module.yml` file:

```yaml title="zephyr/module.yaml"
name: <your module name>
```

#### Build options

Next, you need to define the options to build your module. These also go into `zephyr/module.yml`, as children of the `build` property. Commonly used options are:

- The `depends` child property is used to specify a list of modules which the module depends on. The dependencies are referred to by their [module name](#name).
- The `cmake` child property is used to identify a folder containing a `CMakeLists.txt` file that lists the CMake commands used to build source files.
- The `kconfig` child property is used to identify the `Kconfig` file that defines configuration settings used by the module.
- `settings` is a child property containing additional child properties, two of which are particularly relevant:
  - `board_root` points to the parent directory of a `boards` directory, which contains additional board/shield definitions.
  - `dts_root` points to the parent directory of a `dts` directory, which contains additional devicetree bindings.
  - `snippet_root` points to the parent directory of a `snippets` directory, which contains [snippets](https://docs.zephyrproject.org/3.5.0/build/snippets/index.html).

See the `zephyr/module.yml` found in the template for a usage example.

### Dependencies

If `zephyr/module.yml` has anything listed under `depends`, then you should also define a [west manifest](https://docs.zephyrproject.org/3.5.0/develop/west/manifest.html) file. While the `zephyr/module.yml` file defines _which_ modules your module depends on, the west manifest file defines _where_ said modules are found. This then allows automatic tooling to fetch the modules when building firmware. If `depends` is not present in `zephyr/module.yml`, then this file (named `west.yml` in the template) should be removed.

It is recommended that you place the manifest file at the root of your module, though you can place it elsewhere. Be sure to note in your `README.md` that your module uses dependencies, so that users import these correctly.
Below is an example `west.yml` file for a user that would be using your module, with the necessary `import` field if the module has dependencies:

```yaml title="west.yml"
manifest:
  remotes:
    - name: remote-name
      url-base: https://github.com/remote-name
  projects:
    # Together with the above defined remote, this refers to a module located at
    # https://github.com/remote-name/repository-name
    - name: repository-name
      remote: remote-name
      # This points to a west manifest file in the remote module for further imports
      import: west.yml
```

See the [modules feature page](../features/modules.mdx) for additional information on using west manifest files.

### Informational Files

Make sure to include a `README.md` file and a `LICENSE` file in your repository.

### Predefined Files and Folders

The repository comes with a number of files and folders already positioned for you to edit. The below table describes which files are most likely kept and which are most likely deleted, based on your module's type. Note that these aren't hard rules, merely the most common use case.

|                  | `keyboard` | `component` | `behavior` | `driver` | `feature` | `vfx` |
| ---------------- | ---------- | ----------- | ---------- | -------- | --------- | ----- |
| `boards/`        | ✅         | ✅          | ❌         | ❌       | ❌        | ✅    |
| `dts/`           | ❌         | ❌          | ✅         | ✅       | ✅        | ❌    |
| `CMakeLists.txt` | ❌         | ❌          | ✅         | ✅       | ✅        | ✅    |
| `Kconfig`        | ❌         | ❌          | ✅         | ✅       | ✅        | ✅    |
| `include/`       | ❌         | ❌          | ✅         | ✅       | ✅        | ❌    |
| `src/`           | ❌         | ❌          | ✅         | ✅       | ✅        | ❌    |
| `snippets/`      | ❌         | ❌          | ❌         | ❌       | ✅        | ❌    |

The below table reminds of the purpose of each of these files and folders, if you are not already familiar with them:

| File or Folder   | Description                                                                           |
| ---------------- | ------------------------------------------------------------------------------------- |
| `boards/`        | Folder containing definitions for boards, shields and interconnects                   |
| `dts/`           | Folder containing devicetree bindings and includes with devicetree nodes (.dtsi)      |
| `CMakeLists.txt` | CMake configuration to specify source files to build                                  |
| `Kconfig`        | Kconfig file for the module                                                           |
| `include/`       | Folder for C header files                                                             |
| `src/`           | Folder for C source files                                                             |
| `snippets/`      | Folder for [snippets](https://docs.zephyrproject.org/3.5.0/build/snippets/index.html) |

Note that the `include` and `src` folders are not mandated by the module system, and all of these can be positioned anywhere in your module's filetree if you adjust the `zephyr/module.yml` accordingly. The `west.yml` file is not commonly present in any of the types.

Modules should expose all provided header files with an include path name beginning with the module-name, for example at `include/zmk_<type>_<description>/<header>.h`.

:::info
If your module requires adding drivers to existing subsystems in modules, you will need to use the `zephyr_library_amend()` CMake command, which requires you to have a specific directory structure. See [here](https://github.com/zephyrproject-rtos/zephyr/blob/main/cmake/modules/extensions.cmake#L454) for the definition and some documentation in the comments, with an example [here](https://github.com/petejohanson/ec-support-zmk-module/tree/main/drivers/kscan).
:::

## Examples

Below are some examples of modules for different types. Unless under the `zmkfirmware` project, these are not endorsed officially and may not follow our conventions perfectly. For such reason, the modules chosen to be presented here may change with time.

- Keyboard: https://github.com/petejohanson/zmk-keyboards-katori
- Behavior: https://github.com/urob/zmk-leader-key
- Driver: https://github.com/petejohanson/cirque-input-module
- Feature: https://github.com/joelspadin/zmk-locales
- VFX: https://github.com/caksoylar/zmk-rgbled-widget
