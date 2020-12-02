---
title: Getting Started
sidebar_label: Getting Started
---

## Environment Setup

There are two ways to setup the zmk development environment: Docker+VS Code (Docker in the rest of the documentation) and using the operating system directly (native in the rest of the documentation). The Docker approach is a self-contained development environment while the native approach will setup your local operating system for development. The Docker approach is great for getting going quickly while the native approach is a bit faster but more difficult to setup initially.

Please see the [Docker](/docs/development/setup/docker-vscode) instructions or [native](/docs/development/setup/native) instructions to continue setup.

:::danger
The Docker environment will **NOT** run on arm CPUs like the Raspberry Pi. You must use the native environment if using an arm CPU.
:::

## Standard Conventions

Throughout this documentation you will see commands like

```
sudo apt update
```

These commands should be run in a terminal such as Bash (Linux/macOS/Docker), PowerShell, or Command Prompt (Windows).
