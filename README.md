# Zephyr™ Mechanical Keyboard (ZMK) Firmware

[![Discord](https://img.shields.io/discord/719497620560543766)](https://zmk.dev/community/discord/invite)
[![Build](https://github.com/afayge/zmk/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/afayge/zmk/actions/workflows/build.yml)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](CODE_OF_CONDUCT.md)

[ZMK Firmware](https://zmk.dev/) is an open source ([MIT](LICENSE)) keyboard firmware built on the [Zephyr™ Project](https://www.zephyrproject.org/) Real Time Operating System (RTOS). ZMK's goal is to provide a modern, wireless, and powerful firmware free of licensing issues.

## 本分支相比官方 ZMK 增加了什么

本分支在 ZMK 上扩展了逐键灯光、Studio 固件接口和 BMK 板级功能。下表以本分支合入的[官方基线 bb2d711d1](https://github.com/zmkfirmware/zmk/tree/bb2d711d192f3cca83dff69fe1fc5119418a0102)为参照。

| 功能 | 使用场景与入口 |
| --- | --- |
| LED Matrix | 将按键位置映射到 LED，使用 16 种主灯效渲染器，可按层改色、给独立灯区选预设，并显示电量、USB、蓝牙和 HID 锁定状态。[从 Corne54 开始配置](docs/docs/development/hardware-integration/lighting/led-matrix.mdx) |
| 多种灯光硬件 | 可直接驱动 RGB 灯带或 Zephyr LED 设备，也可用 `led_composite` 把多块驱动芯片、单色 LED 和灯带编成同一组逻辑灯号。[后端、通道映射与供电参考](docs/docs/led-matrix-led-composite-reference.mdx) |
| Studio 在线编辑 | 兼容本分支协议的客户端可读取灯效目录、调整预设和灯区，也可为带旋钮的键盘逐层设置顺、逆时针绑定。灯效持久化编辑需启用 LED Matrix 编辑器和 settings，旋钮编辑需启用 Studio 并在键盘上定义传感器。[Studio 配置](docs/docs/config/studio.md) · [配套客户端](https://github.com/afayge/zmk-studio) |
| 轨迹球与指针速度 | BlackBerry 轨迹球支持按滚动速度加速、切换速度档位、配置按压动作和四颗 PWM 灯。[接线及调校示例](docs/docs/hardware-integration/blackberry-trackball.mdx) |
| BMK 键盘与显示 | 包含 Corne54 系列和 Numpad 等板型。支持的状态屏可显示 Num、Caps 和 Scroll Lock；BMK Numpad 还可在板上计算并将结果输入当前 USB 或蓝牙主机。[BMK 板型](app/boards/bmk) · [Numpad 计算器键位](app/boards/bmk/numpad/calculator/README.md) |

## Upstream ZMK resources

Check out the website to learn more: https://zmk.dev/.

You can also come join our [ZMK Discord Server](https://zmk.dev/community/discord/invite).

To review features, check out the [feature overview](https://zmk.dev/docs/). ZMK is under active development, and new features are listed with the [enhancement label](https://github.com/zmkfirmware/zmk/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement) in GitHub. Please feel free to add 👍 to the issue description of any requests to upvote the feature.
