---
title: Dev Container CLI
sidebar_label: Dev Container CLI
---

### Installing Development Tools

1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop) for
   your operating system.
2. Install the [Dev Container CLI](https://github.com/devcontainers/cli).

### Initialize & Update Zephyr Workspace

Open up your Terminal an make sure that the
[Dev Container CLI](https://github.com/devcontainers/cli) is installed by
running:

```sh
devcontainer --version
```

To be able to execute commands, the
[Dev Container CLI](https://github.com/devcontainers/cli) has to know where the
`devcontainer.json` is located. This can be done using the `--workspace-folder`
option, which is available for all commands and will automatically look up the
file relative to the path. Therefore, the container can be started by executing:

```sh
devcontainer up --workspace-folder "/absolute/path/to/zmk"
```

The first time you do this on your machine, it will pull down the Docker image
from the registry and build the container. **Subsequent launches are much
faster!**

Once the command is finished, the development environment is available.
Furthermore, the path is mounted under `/workspaces/zmk/`. It can then be used
to initialize the application and update modules, including Zephyr.

```sh
devcontainer exec --workspace-folder "/absolute/path/to/zmk" \
  west init -l "/workspaces/zmk/app"
devcontainer exec --workspace-folder "/absolute/path/to/zmk" \
  west update
```

:::tip
This step pulls down quite a bit of tooling, be patient!
:::

:::info
You must restart the container at this point. This can be done by using:

- `docker container ps` to get the container id
- `docker container stop <id>` to stop the container

Your setup is complete once your container has been restarted.
:::
