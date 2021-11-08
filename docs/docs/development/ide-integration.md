---
title: IDE Integration
sidebar_label: IDE Integration
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

export const OsTabs = (props) => (<Tabs
groupId="operating-systems"
defaultValue="debian"
values={[
{label: 'Debian/Ubuntu', value: 'debian'},
{label: 'Windows', value: 'win'},
{label: 'macOS', value: 'mac'},
{label: 'Raspberry OS', value: 'raspberryos'},
{label: 'Fedora', value: 'fedora'},
{label: 'VS Code & Docker', value: 'docker'},
]
}>{props.children}</Tabs>);

## Visual Studio Code

Visual Studio Code needs to know some things about the project such as include
paths and compiler paths before features such as code completion, go to definition,
and graying out disabled code blocks will work. Fortunately, CMake can generate
that configuration for us automatically.

### Create a Compilation Database

To configure `west` to tell CMake to generate a compilation database, open a
terminal to the ZMK repository and run the following command:

```sh
west config build.cmake-args -- -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Every [build](build-flash.md#building) will now update the database. You will
need to build once to create the database before code completion will work.
We'll tell Visual Studio Code where to find the database in the next step.

:::note
If you have set any other CMake arguments such as the path to your zmk-config, the
above command will overwrite them. You should instead provide the flag to export
compile commands and all other arguments surrounded by quotes. For example:

```sh
west config build.cmake-args -- "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DZMK_CONFIG=/path/to/zmk-config/config"
```

:::

### Create a C/C++ Configuration

Install the [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools),
then run **F1 > C/C++: Edit Configurations (UI)**. It should automatically create
a new configuration for you, but if the text box under **Configuration name** is empty,
click **Add Configuration**, enter a name, and click **OK**.

Change these options:

| Option                                | Value                                                |
| ------------------------------------- | ---------------------------------------------------- |
| Compiler path                         | Path to your toolchain's GCC binary (see below)      |
| IntelliSense mode                     | gcc-arm                                              |
| Advanced Settings > Compiler commands | `${workspaceFolder}/app/build/compile_commands.json` |

#### Compiler Path

<OsTabs>
<TabItem value="debian">

Open VS Code's integrated terminal and run the following commands. It will print
your compiler path.

```sh
source zephyr/zephyr-env.sh
echo ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
```

:::note
You will need to update this path any time you switch to a new version of the Zephyr SDK.
:::

</TabItem>
<TabItem value="win">

Your compiler path is

```
${env:GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc.exe
```

This assumes `GNUARMEMB_TOOLCHAIN_PATH` is set in your system or user environment variables.
If not, you will need to list the full path instead of using the `${env}` placeholder.

</TabItem>
<TabItem value="mac">

Open VS Code's integrated terminal and run the following command. It will print
your compiler path.

```sh
echo ${GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc
```

</TabItem>
<TabItem value="raspberryos">

Your compiler path is

```
/usr/bin/arm-none-eabi-gcc
```

</TabItem>
<TabItem value="fedora">

Open VS Code's integrated terminal and run the following commands. It will print
your compiler path.

```sh
source zephyr/zephyr-env.sh
echo ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
```

:::note
You will need to update this path any time you switch to a new version of the Zephyr SDK.
:::

</TabItem>
<TabItem value="docker">

Your compiler path is

```
${env:ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
```

</TabItem>
</OsTabs>

#### Compiler Commands Path

When building with all default options, the path to the compilation database file
is `${workspaceFolder}/app/build/compile_commands.json` as shown in the table above,
however some arguments to `west build` can change this path.

The `-d` or `--build-dir` option lets you change the build directory to something
other than `build`. Replace `build` in the above path with what you set this to.
For example, if you build with `-d build/my_shield`, the path is
`${workspaceFolder}/app/build/my_shield/compile_commands.json`. If you use this
to keep builds for multiple keyboards separate, you may want to create a separate
C/C++ configuration for each one in VS Code.

You can also build from the root folder of the project instead of the `app`
folder by adding `-S app` to your CMake arguments. In this case, simply remove
`app` from the path to `compile_commands.json`, for example,
`${workspaceFolder}/build/compile_commands.json`.
