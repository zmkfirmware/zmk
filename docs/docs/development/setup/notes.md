### Notes about setup

As I cannot seem to figure out a reliable way to get comments working with Docusaurus, I'm writing notes in this file instead.

I don't know how accurate this is, and [Zephyr does not recommend WSL](https://docs.zephyrproject.org/3.5.0/develop/getting_started/index.html), so I'm not sure this should be included.

:::note Windows Users
Please note the ZMK builds run slower (up to 3-5 minutes on slower hardware) with Docker on Windows if you don't use the WSL2 filesystem to store files. If you run into performance problems you can checkout the ZMK sources inside a WSL2 environment and run `code .` to open the sources. This can make builds run at near-native speed.

This approach will also need the [Remote - WSL](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-wsl) extension installed in VS Code.

Files stored within WSL2 can be accessed via Windows Explorer by navigating to `\\wsl$`.
:::

Googling this seems to imply that there are workarounds, but I do not have access to a device to test it out. It is not included under the current docs, so I don't see a reason to include it until it is brought up by someone.

:::danger The Docker environment will NOT run on arm CPUs like the Raspberry Pi or Apple Silicon. You must use the native environment if using an arm CPU. :::

I think this is true, but I have not tested.

:::caution Windows Users If you're using the Docker environment on Windows, you must checkout the sources to a folder within C:\Users\[your_user_here] to avoid a potential permissions issue.

If you're using the WSL2 filesystem the sources should go under ~/ to avoid potential permissions issues. :::
