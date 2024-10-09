---
title: Docker
sidebar_label: Docker
---

### Source Code

First, you'll need to clone the ZMK source repository if you haven't already.
Open a terminal and navigate to the folder you would like to place your `zmk`
directory in, then run the following command:

```sh
git clone https://github.com/zmkfirmware/zmk.git
```

### Docker Setup

Currently, there are two ways of building your local development environment:

- [VS Code](vscode.md): \
  This approach takes advantage of [VS Code](https://code.visualstudio.com/)’s
  [Remote - Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
  extension, allowing you to open the ZMK repository in a Docker container
  directly within the editor. It is recommended for everyone who is familiar
  with or already uses [VS Code](https://code.visualstudio.com/), since it
  easily integrates with Docker and provides a seamless development experience.

- [Dev Container CLI](cli.md): \
  For users who prefer not to use [VS Code](https://code.visualstudio.com/), the
  [Dev Container CLI](https://github.com/devcontainers/cli) provides a
  command-line alternative for managing and running
  [dev containers](https://containers.dev/), with similar functionality.

### Additional Setup

In case you have a `zmk-config` or want to build with additional modules, it is
necessary to first make those available to the Docker container. This can be
done by mounting them as volumes.

#### `zmk-config`

```sh
docker volume create --driver local -o o=bind -o type=none \
  -o device="/path/to/zmk-config/" zmk-config
```

#### Modules

```sh
docker volume create --driver local -o o=bind -o type=none -o \
  device="/path/to/zmk-module/" zmk-module
```

:::tip
Once this is done, you can refer to the `zmk-config` with
`/workspace/zmk-config` and `/workspace/zmk-module` to the modules instead of
using their full path like it is shown in the
[build commands](../../build-flash.mdx), since these are the locations they were
mounted to in the container.
:::
