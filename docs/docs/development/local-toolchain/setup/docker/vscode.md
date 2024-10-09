---
title: VS Code
sidebar_label: VS Code
---

:::note
This approach is documented for [VS Code](https://github.com/microsoft/vscode)
not [Code OSS](https://github.com/microsoft/vscode/wiki/Differences-between-the-repository-and-Visual-Studio-Code).
:::

### Installing Development Tools

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for
   your operating system.
2. Install [VS Code](https://code.visualstudio.com/).
3. Install the [Remote - Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).

### Initialize & Update Zephyr Workspace

Open the `zmk` checkout directory in VS Code. The repository includes a
configuration for containerized development. Therefore, an alert will pop up:

![VS Code Dev Container Configuration Alert](../../../../assets/dev-setup/vscode_devcontainer.png)

Click `Reopen in Container` to reopen the VS Code with the running container.
If the alert fails to pop up or you accidentally close it, you can perform the
same action by pressing the following keys based on your operating system and
selecting `Remote: Show Remote Menu`:

- **Windows/Linux**: `Ctrl + Shift + P`
- **MacOs**: `Cmd + Shift + P`

The first time you do this on your machine, it will pull down the Docker image
from the registry and build the container. **Subsequent launches are much
faster!**

:::caution
The following step and any future [build commands](../../build-flash.mdx) must
be executed from the VS Code terminal _inside_ the container.
:::

Initialize the application and update modules, including Zephyr:

```sh
west init -l app/
west update
```

:::tip
This step pulls down quite a bit of tooling, be patient!
:::

:::info
You must restart the container at this point. The easiest way to do this is to
close the VS Code window, verify that the container has stopped in the Docker
Dashboard, and reopen the container with VS Code.

Your setup is complete once your container has restarted.
:::
