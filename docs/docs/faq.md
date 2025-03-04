---
title: FAQs
sidebar_label: FAQs
---

### Why Zephyr™?

As a best-in-class RTOS, Zephyr™ brings many [benefits](https://www.zephyrproject.org/benefits) to ZMK, such as:

- A _single_ platform [supporting](https://docs.zephyrproject.org/3.5.0/boards/index.html) many architectures, processors and boards.
- Optimization for low-powered, small memory footprint devices.
- Powerful hardware abstraction and configuration using [DeviceTree](https://docs.zephyrproject.org/3.5.0/build/dts/index.html) and [Kconfig](https://docs.zephyrproject.org/3.5.0/build/kconfig/index.html).
- A BLE stack that periodically obtains [qualification](https://docs.zephyrproject.org/3.5.0/connectivity/bluetooth/bluetooth-qual.html) listings, making it easier for final products to obtain qualification from the Bluetooth® SIG.
- Multi-processor support, which is critical for power efficiency in upcoming MCUs.
- Permissive licensing with its Apache 2.0 open source [license](https://www.apache.org/licenses/LICENSE-2.0).
- A buzzing developer [community](https://github.com/zephyrproject-rtos/zephyr) including many leading [embedded technology](https://www.zephyrproject.org/project-members) companies.
- Long term support (LTS) with security updates.

### Why yet another keyboard firmware?

That’s an excellent question! There are already great keyboard firmwares available, but ZMK has some advantages:

- Zephyr™
  - See [Why Zephyr™?](#why-zephyr)
- Licensing
  - Just like other open source firmware, ZMK is all about the free and the sharing. However, some other projects use the GPL license which prevents integration of libraries and drivers whose licenses are not GPL-compatible (such as some embedded BLE drivers). ZMK uses the permissive [MIT](https://github.com/zmkfirmware/zmk/blob/main/LICENSE) license which doesn’t have this limitation.
- Wireless First
  - ZMK is designed for the future, and we believe the future is wireless. So power efficiency plays a critical role in every design decision, just like in Zephyr™.

The ZMK contributors firmly believe that a keyboard firmware built on Zephyr™ will provide more long term benefits.

### What license does ZMK use?

ZMK uses the MIT [license](https://github.com/zmkfirmware/zmk/blob/main/LICENSE).

### What hardware/platforms does ZMK support?

ZMK has the potential to run on any platform supported by Zephyr™. However, it’s impractical for the ZMK contributors to test all possible hardware.

The Zephyr™ [documentation](https://docs.zephyrproject.org/3.5.0/boards/index.html) describes which hardware is currently natively supported by the Zephyr™ platform. _Similar documentation covering which keyboards have been integrated into ZMK is currently being planned._

### Does ZMK compile for AVR?

Sorry, Zephyr™ only supports 32-bit and 64-bit platforms.

### How do I get started?

ZMK is still in its infancy, so there’s a learning curve involved. But if you’d like to try it out, please check out the development [documentation](/docs) and the other FAQs. Please keep in mind that the project team is still small, so our support capability is limited whilst we focus on development. But we’ll try our best! Interested developers are also very welcome to contribute!

### What are _boards_ and _shields_? Why not just "keyboard"?

ZMK uses the Zephyr concepts of "boards" and "shields" to refer to different parts of a keyboard build, that in turn get combined during a firmware build.
This provides the modularity to be able to use composite keyboards with different compatible controllers.
Please see the [explainer on boards & shields](development/hardware-integration/index.mdx#boards--shields) for more details.

### Does ZMK support wired split?

Currently, ZMK only supports wireless split, but wired split is possible and we welcome contributions!

### How is the latency?

The latency of ZMK is comparable to other firmware offerings. ZMK is equipped with a variety of scanning methods and [debounce algorithms](features/debouncing.md) that can affect the final measured latency. [This video](https://www.youtube.com/watch?v=jWL4nU-vtWs) shows a latency comparison of ZMK and other keyboard firmwares.

### Any chance for 2.4GHz dongle implementation?

At this time, there are no current plans to implement 2.4GHz dongle mode. This is because utilizing Nordic's proprietary 2.4GHz low level protocols requires use of the Nordic Connect SDK, which is licensed with a more restrictive license than ZMK's MIT license. However, it is possible to [create a dongle](development/hardware-integration/dongle.mdx) for your keyboard that runs ZMK and communicates between parts using BLE (with encryption). This results in a 3.75ms average theoretical latency from the protocol itself.

### What bootloader does ZMK use?

ZMK isn’t designed for any particular bootloader, and supports flashing different boards with different flash utilities (e.g. OpenOCD, nrfjprog, etc.). So if you have any difficulties, please let us know on [Discord](https://zmk.dev/community/discord/invite)!

### Can I contribute?

Of course! Please use the developer [documentation](/docs) to get started!

### I have an idea! What should I do?

Please join us on [Discord](https://zmk.dev/community/discord/invite) and discuss it with us!

### I want to add a new keyboard! What should I do?

The exact process for the management of all the possible hardware is still being finalized, but any developer looking to contribute new keyboard definitions should chat with us on [Discord](https://zmk.dev/community/discord/invite) to get started.

### Does ZMK have a Code of Conduct?

Yes, it does have a [Code of Conduct](https://github.com/zmkfirmware/zmk/blob/main/CODE_OF_CONDUCT.md)! Please give it a read!

### What does “ZMK” mean?

ZMK was originally coined as a quasi-acronym of “Zephyr Mechanical Keyboard” and also taking inspiration from the amazing keyboard firmware projects, TMK and QMK.

### Is ZMK related to TMK or QMK?

No. But inspired by, of course!

### Who created ZMK?

ZMK was created by Pete Johanson. It is developed and maintained by the open source community.
