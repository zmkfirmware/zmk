---
title: Docker
sidebar_label: Docker
---

:::note
Currently the Docker approach is only documented for VSCode, Microsoft's proprietary version of [Code OSS](https://github.com/microsoft/vscode) - See [here](https://github.com/microsoft/vscode/wiki/Differences-between-the-repository-and-Visual-Studio-Code) for more info on different versions. There is an alternative approach using [devcontainers](https://containers.dev/), but this is not documented yet - contributions are welcome!
:::

## Installing Development Tools

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for your operating system.
2. Install [VS Code](https://code.visualstudio.com/)
3. Install the [Remote - Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

### Initialize & Update Zephyr Workspace

Since ZMK is built as a Zephyr™ application, the next step is
to use `west` to initialize and update your workspace. The ZMK
Zephyr™ application is in the `app/` source directory:

#### Step into the repository

Open the `zmk` checkout folder in VS Code. The repository includes a configuration for containerized development, so an alert will pop up:

![VS Code Dev Container Configuration Alert](../../assets/dev-setup/vscode_devcontainer.png)

Click `Reopen in Container` in order to reopen the VS Code with the running container.

The first time you do this on your machine, it will pull the docker image down from the registry and build the container. Subsequent launches are much faster!

:::note
If the alert fails to pop up or you accidentally close it, you can perform the same action from the remote menu: `ctrl+shift+p` and invoke `Remote: Show Remote Menu`.
:::

:::caution
All subsequent steps must be performed from the VS Code terminal _inside_ the container.
:::

#### Initialize West

```sh
west init -l app/
```

#### Update To Fetch Modules

```sh
west update
```

:::tip
This step pulls down quite a bit of tooling. Go grab a cup of coffee, it can take 10-15 minutes even on a good internet connection!
:::

:::info
You must restart the container at this point. The easiest way to do so is to close the VS Code window, verify that the container has stopped in Docker Dashboard, and reopen the container with VS Code.

Your setup is complete once your container has restarted.
:::
