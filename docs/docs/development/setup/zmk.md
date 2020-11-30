---
title: ZMK Sources
sidebar_label: ZMK Sources
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

export const OsTabs = (props) => (<Tabs
groupId="operating-systems"
defaultValue="docker"
values={[
{label: 'VS Code & Docker', value: 'docker'},
{label: 'Debian/Ubuntu', value: 'debian'},
{label: 'macOS', value: 'mac'},
{label: 'Windows', value: 'win'},
{label: 'Raspberry OS', value: 'raspberryos'},
{label: 'Fedora', value: 'fedora'},
]
}>{props.children}</Tabs>);

:::caution Windows Users
If you're using the Docker environment on Windows, you _must_ checkout the sources to a folder within `C:\Users\[your_user_here]` to avoid a potential permissions issue.

If you're using the WSL2 native filesystem the sources should go under `~/` to avoid potential permissions issues.
:::

### Source Code

Next, you'll need to clone the ZMK source repository if you haven't already. Open a terminal and navigate to the folder you would like to place your `zmk` directory in, then run the following command:

```
git clone https://github.com/zmkfirmware/zmk.git
```

### Initialize & Update Zephyr Workspace

Since ZMK is built as a Zephyr™ application, the next step is
to use `west` to initialize and update your workspace. The ZMK
Zephyr™ application is in the `app/` source directory:

#### Step into the repository

<OsTabs>
<TabItem value="docker">

Open the `zmk` checkout folder in VS Code. The repository includes a configuration for containerized development, so an alert will pop up:

![VS Code Dev Container Configuration Alert](../../assets/dev-setup/vscode_devcontainer.png)

Click `Reopen in Container` in order to reopen the VS Code with the running container.

The first time you do this on your machine, it will pull the docker image down from the registry and build the container. Subsequent launches are much faster!

:::caution
All subsequent steps must be performed from the VS Code terminal _inside_ the container.
:::

</TabItem>

<TabItem value="debian">

```sh
cd zmk
```

</TabItem>
<TabItem value="raspberryos">

```sh
cd zmk
```

</TabItem>
<TabItem value="fedora">

```sh
cd zmk
```

</TabItem>
<TabItem value="mac">

```sh
cd zmk
```

</TabItem>
<TabItem value="win">

```sh
cd zmk
```

</TabItem>
</OsTabs>

#### Initialize West

```sh
west init -l app/
```

:::caution Command Not Found? (Native OS)
If you encounter errors like `command not found: west` then your `PATH` environment variable is likely
missing the Python 3 user packages directory. See the [West Build Command](#west-build-command)
section again for links to how to do this
:::

#### Update To Fetch Modules

```sh
west update
```

:::tip
This step pulls down quite a bit of tooling. Go grab a cup of coffee, it can take 10-15 minutes even on a good internet connection!
:::

#### Export Zephyr™ Core

```sh
west zephyr-export
```

#### Install Zephyr Python Dependencies

```sh
pip3 install --user -r zephyr/scripts/requirements-base.txt
```
