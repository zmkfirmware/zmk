---
title: Getting Started
sidebar_label: Getting Started
---

:::info
Throughout this documentation you will see commands like

```sh
sudo apt update
```

These commands should be run in a terminal such as Bash (Linux/MacOS/Docker), PowerShell, or Command Prompt (Windows).
:::

:::tip
We recommend reading through the setup process before following it step by step, to ensure that you are happy with installing the required dependencies.
:::

### Source Code

First, you'll need to clone the ZMK source repository if you haven't already. Open a terminal and navigate to the folder you would like to place your `zmk` directory in, then run the following command:

```sh
git clone https://github.com/zmkfirmware/zmk.git
```

:::warning

#### Windows Users

If you're using the Docker environment on Windows, you _must_ checkout the sources to a folder within `C:\Users\[your_user_here]` to avoid a potential permissions issue.

If you're using the WSL2 filesystem the sources should go under `~/` to avoid potential permissions issues.
:::

## Environment Setup

There are two ways to setup the ZMK development environment:

- [Docker](/docs/development/setup/docker) ;
- [Native](/docs/development/setup/native), i.e. using the operating system directly.

The Docker approach is a self-contained development environment while the native approach will setup your local operating system for development. It uses the same [Docker image which is used by the GitHub action](https://github.com/zmkfirmware/zmk-docker) for local development. Beyond the benefits of [dev/prod parity](https://12factor.net/dev-prod-parity), this approach may be easier to set up for some operating systems. No toolchain or dependencies are necessary when using Docker; the container image has the toolchain installed and set up to use.

The native approach installs the toolchain and dependencies on your system directly. This typically runs slightly faster than the Docker approach, and can be preferable for users who already have the dependencies on their system.

Please see the [Docker](/docs/development/setup/docker) instructions or [native](/docs/development/setup/native) instructions to continue setup.
