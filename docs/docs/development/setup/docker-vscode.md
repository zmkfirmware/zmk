---
title: Docker+VSCode Setup
sidebar_label: Docker+VSCode
---

:::danger The Docker+VSCode environment will **NOT** run on arm CPUs. You must use the Native environment if using an arm CPU.
:::

This setup leverages the same [image which is used by the GitHub action](https://github.com/zmkfirmware/zephyr-west-action) for local development. Beyond the benefits of [dev/prod parity](https://12factor.net/dev-prod-parity), this approach is also the easiest to set up. No toolchain or dependencies are necessary when using Docker; the container image you'll be using already has the toolchain installed and set up to use.

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for your operating system.
2. Install [VS Code](https://code.visualstudio.com/)
3. Install the [Remote - Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

:::info
Once the above is setup you're ready to setup the ZMK sources as defined [here](/docs/development/setup/zmk)
:::
