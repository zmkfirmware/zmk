---
title: Docker+VS Code Setup
sidebar_label: Docker
---

:::danger
The Docker environment will **NOT** run on arm CPUs like the Raspberry Pi or Apple Silicon. You must use the native environment if using an arm CPU.
:::

This setup leverages the same [Docker image used when building ZMK on GitHub](https://github.com/zmkfirmware/zmk-docker/). Beyond the benefits of [dev/prod parity](https://12factor.net/dev-prod-parity), this approach is also the easiest to set up. No toolchain or dependencies are necessary when using Docker; the container image you'll be using already has the toolchain installed and set up to use.

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for your operating system.
2. Install [VS Code](https://code.visualstudio.com/)
3. Install the [Remote - Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

:::note Windows Users
Please note the ZMK builds run slower (up to 3-5 minutes on slower hardware) with Docker on Windows if you don't use the WSL2 filesystem to store files. If you run into performance problems you can checkout the ZMK sources inside a WSL2 environment and run `code .` to open the sources. This can make builds run at near-native speed.

This approach will also need the [Remote - WSL](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl) extension installed in VS Code.

Files stored within WSL2 can be accessed via Windows Explorer by navigating to `\\wsl$`.
:::

## ZMK Sources Setup

Once the above is setup you're ready to [set up the ZMK sources](/docs/development/setup/zmk).
