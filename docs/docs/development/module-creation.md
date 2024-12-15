---
title: Module Creation
sidebar_label: ZMK Module Creation
---

[ZMK modules](../features/modules.mdx) are the recommended method of adding content to ZMK, whenever it is possible. This page will guide you through creating a module for ZMK. Distinction is made between modules used for different purposes:

- Modules containing one or more keyboard-related definitions (boards, shields, interconnects, etc.)
- Modules containing behaviors & features
- Modules containing drivers
- Modules containing visual effects

ZMK comes with a Git repository called `zmk-modules` which tracks various different modules. This allows modules to be discovered and used by various tools and scripts. The below described creation process will guide you through the creation and addition of a module to `zmk-modules`.

:::tip
For open source hardware designs, it can be convenient to use [Git submodules](https://github.blog/open-source/git/working-with-submodules/) to have the ZMK module also be a Git submodule of the repository hosting the hardware design.
:::

## Module Setup

ZMK has a template to make creating a module easier. Navigate to [the ZMK module template repository](https://github.com/Nick-Munnich/zmk-module-template) and select "Use this template" followed by "Create a new repository" in the top right.

The rest of this page will go through the contents of the template step by step, guiding you through the configuration of the module.

### Zephyr Module File

This is the file that defines your module. Only when this file is configured correctly will your module function as expected.

#### Name

First, you will need to name your module. ZMK has a naming convention that you should follow:

```
zmk-<type>-<description>
```

Valid options for type are:

| Type         | Description                                                                                                           |
| ------------ | --------------------------------------------------------------------------------------------------------------------- |
| `keyboards`  | The module contains definitions for one or multiple keyboards, be they boards or shields.                             |
| `components` | The module contains definitions for one or multiple components, such as boards for shields to use.                    |
| `behavior`   | The module contains a single behavior.                                                                                |
| `driver`     | The module contains a single driver.                                                                                  |
| `feature`    | The module contains a single feature which doesn't fall under the other categories.                                   |
| `vfx`        | The module contains definitions for one or multiple visual effects, such as display customisations or RGB animations. |

`<description>` is your choice of name to describe the module. See the names of other modules of your chosen type in the `zmk-modules` repository for examples.

Note that your _module name_ does not have to be the same as your _repository name_. For example, if you have named your module `zmk-keyboards-corne`, you may for your own organisational purposes want to name the repository e.g. `zmk-keyboards-corne-module`.

Your module name goes in the `zephyr/module.yml` file:

```yaml title="zephyr/module.yaml"
name: <your module name>
```

#### Build Options

Next, you need to define the options to build your module. These also go into `zephyr/module.yml`, as children of the `build` property. Commonly used options are:

- The `depends` child property is used to specify a list of modules which the module depends on. The dependencies are referred to by their [module name](#module-name).
- The `cmake` child property is used to identify a folder containing a `CMakeLists.txt` used to build the module.
- The `kconfig` child property is used to identify the `Kconfig` used to build the module.
- `settings` is a child property containing additional child properties, two of which are particularly relevant:
  - `board_root` points to the parent directory of a `boards` directory, which contains additional board/shield definitions.
  - `dts_root` points to the parent directory of a `dts` directory, which contains additional devicetree bindings.
  - `snippet_root` points to the parent directory of a `snippets` directory, which contains [snippets](https://docs.zephyrproject.org/latest/build/snippets/index.html).

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

Make sure to include a `README.md` file and a `LICENSE` file in your repository. Only modules with MIT-compatible licenses can be added to `zmk-modules`.

### Predefined Files and Folders

The repository comes with a number of files and folders already positioned for you to edit. The below table describes which files are most likely kept and which are most likely deleted, based on your module's type. Note that these aren't hard rules, merely the most common use case.

|                  | `keyboards` | `components` | `behavior` | `driver` | `feature` | `vfx` |
| ---------------- | ----------- | ------------ | ---------- | -------- | --------- | ----- |
| `board/`         | ✅          | ✅           | ❌         | ❌       | ❌        | ✅    |
| `dts/`           | ❌          | ❌           | ✅         | ✅       | ✅        | ❌    |
| `CMakeLists.txt` | ❌          | ❌           | ✅         | ✅       | ✅        | ✅    |
| `Kconfig`        | ❌          | ❌           | ✅         | ✅       | ✅        | ✅    |
| `include/`       | ❌          | ❌           | ✅         | ✅       | ✅        | ❌    |
| `src/`           | ❌          | ❌           | ✅         | ✅       | ✅        | ❌    |
| `snippets/`      | ❌          | ❌           | ❌         | ❌       | ✅        | ❌    |

The below table reminds of the purpose of each of these files and folders, if you are not already familiar with them:

| File or Folder   | Description                                                                            |
| ---------------- | -------------------------------------------------------------------------------------- |
| `board/`         | Folder containing definitions for boards, shields and interconnects                               |
| `dts/`           | Folder containing devicetree bindings and includes with devicetree nodes (.dtsi)                                 |
| `CMakeLists.txt` | CMake configuration to specify source files to build                                                              |
| `Kconfig`        | Kconfig file for the module                                                            |
| `include/`       | Folder for C header files                                                       |
| `src/`           | Folder for C source files                                                              |
| `snippets/`      | Folder for [snippets](https://docs.zephyrproject.org/3.5.0/build/snippets/index.html) |

Note that the `include` and `src` folders are not mandated by the module system, and all of these can be positioned anywhere in your module's filetree if you adjust the `zephyr/module.yml` accordingly.

## Inclusion in ZMK Modules Repository

:::warning
The ZMK modules repository is currently experimental, and the below content is subject to change. ZMK modules is also missing tooling.
:::

Once you have written and tested your module, you should prepare it for inclusion into `zmk-modules`. The primary reasons for you wanting to do so are:

- It makes discovering and using your module significantly easier for other users
- Automated tools can help you maintain your module notifying you if there are any potentially breaking changes that you should investigate or new features that you may want to account for.

### Requirements

The following requirements are necessary for a module to be included in `zmk-modules`:

- [ ] The module is named according to convention
- [ ] The module has been tested and confirmed working
- [ ] The module features a `README.md` file describing the module
- [ ] The module is licensed under an MIT-compatible license, preferably through a `LICENSE` file
- [ ] The module follows the general style of other modules of the same type found in `zmk-modules`
- [ ] There are no unused files or folders present in the module
- [ ] The module is up-to-date with the current version of ZMK
- [ ] If the module defines keyboards or hardware components, said keyboards/components are available for public use (sold by a vendor, open source, etc.)
- [ ] If the module defines boards or shields, they follow the board/shield definition practices as recommended in the ZMK docs

### Module File

If the requirements are satisfied, then you will need to make a pull request to the `zmk-modules` repository adding a file allowing us to track your module.

The file should be named `<module-name>.yml`, where module-name is the _Zephyr_ module name, not the _repository_ name. It has the following format:

```yaml title="<module-name>.yml"
remotes:
  - name: remote-name
    url: https://github.com/remote-name/repository-name
versions:
  - remote: remote-name
    zmk: "0.1.0"
    commit: d1d9b06ebb524d5c8edc6d6dba58b98d98a5a6aa
```

The `versions` property is used to assign particular commits in the repository to ZMK versions. This simultaneously protects `zmk-modules` from malicious commits and allows for improved compatibility -- if multiple modules are being used, the maximum version that all modules are compatible with is used. Ideally, for each ZMK update a new entry would appear under `versions`. The addition of automated tools to help with this process is planned.

The `remotes` name exists primarily as a fallback in case a module is not maintained for a prolonged period of time. If such an event was to happen, then another user could update the module for a more recent version and "take over" the duties of maintaining the module.

The module file should be added to the appropriate `type` folder under `zmk-modules`. Once you have done so, you can make a pull request.
