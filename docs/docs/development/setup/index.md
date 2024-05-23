---
title: Getting Started
sidebar_label: Getting Started
---

:::tip
We recommend reading through the setup process before following it step by step, to ensure that you are happy with installing the required dependencies.
:::

## Environment Setup

There are two ways to set up the ZMK development environment:

- [Docker](/docs/development/setup/docker): \
  A self-contained development environment. It uses the same [Docker image which is used by the GitHub action](https://github.com/zmkfirmware/zmk-docker) for local development. Beyond the benefits of [dev/prod parity](https://12factor.net/dev-prod-parity), this approach may be easier to set up for some operating systems. No toolchain or dependencies are necessary when using Docker; the container image has the toolchain installed and set up to use.

- [Native](/docs/development/setup/native):\
  This uses your operating system directly. Usually runs slightly faster than the Docker approach, and can be preferable for users who already have the dependencies on their system.

Please see the [Docker](/docs/development/setup/docker) instructions or [native](/docs/development/setup/native) instructions to continue setup.
