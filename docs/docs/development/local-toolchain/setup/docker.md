---
title: Docker
sidebar_label: Docker
---

:::note
Currently the Docker approach is only documented for [VS Code](https://github.com/microsoft/vscode) (not [Code OSS](https://github.com/microsoft/vscode/wiki/Differences-between-the-repository-and-Visual-Studio-Code)). While it can be replicated using [devcontainers](https://containers.dev/) this is not documented yet - contributions are welcome!
:::

### Source Code

First, you'll need to clone the ZMK source repository if you haven't already. Open a terminal and navigate to the folder you would like to place your `zmk` directory in, then run the following command:

```sh
git clone https://github.com/zmkfirmware/zmk.git
```

### Installing Development Tools

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for your operating system.
2. Install [VS Code](https://code.visualstudio.com/).
3. Install the [Remote - Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).

### Initialize & Update Zephyr Workspace

Open the `zmk` checkout folder in VS Code. The repository includes a configuration for containerized development, so an alert will pop up:

![VS Code Dev Container Configuration Alert](../../assets/dev-setup/vscode_devcontainer.png)

Click `Reopen in Container` in order to reopen the VS Code with the running container. If the alert fails to pop up or you accidentally close it, you can perform the same action by pressing `ctrl+shift+p` and selecting `Remote: Show Remote Menu`.

The first time you do this on your machine, it will pull the docker image down from the registry and build the container. Subsequent launches are much faster!

:::caution
The following step and any future [build commands](../build-flash.mdx) must be executed from the VS Code terminal _inside_ the container.
:::

Initialize the application and update to fetch modules, including Zephyr:

```sh
west init -l app/
west update
```

:::tip
This step pulls down quite a bit of tooling, be patient!
:::

:::info
You must restart the container at this point. The easiest way to do so is to close the VS Code window, verify that the container has stopped in Docker Dashboard, and reopen the container with VS Code.

Your setup is complete once your container has restarted.
:::
