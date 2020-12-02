---
title: Docker+VS Code Setup
sidebar_label: Docker
---

:::danger
The Docker environment will **NOT** run on arm CPUs like the Raspberry Pi. You must use the native environment if using an arm CPU.
:::

:::note Windows Users
Please note the zmk builds can run slower with Docker on Windows if you don't use the WSL2 filesystem to store files. Build times can take 3-5 minutes on slower hardware without using the WSL2 filesystem. If you run into performance problems you can checkout the zmk sources inside a WSL2 environment and use `code .` inside the WSL2 environment to open the sources. This can make builds run as fast as 20s.

This approach will also need the [Remote - WSL](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl) extension installed in VS Code.

Files stored within WSL2 can be accessed via Windows Explorer by navigating to `\\wsl$`.
:::

This setup leverages the same [image which is used by the GitHub action](https://github.com/zmkfirmware/zephyr-west-action) for local development. Beyond the benefits of [dev/prod parity](https://12factor.net/dev-prod-parity), this approach is also the easiest to set up. No toolchain or dependencies are necessary when using Docker; the container image you'll be using already has the toolchain installed and set up to use.

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for your operating system.
2. Install [VS Code](https://code.visualstudio.com/)
3. Install the [Remote - Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

## ZMK Sources Setup

Once the above is setup you're ready to [set up the ZMK sources](/docs/development/setup/zmk).
