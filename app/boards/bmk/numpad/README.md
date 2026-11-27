# BMK Numpad

nRF52840 数字键盘，配有 IS31FL3737 LED 矩阵和 ST75256 液晶屏。

## Building numpad ZMK firmware

此板的 LCD 依赖 ST75256 驱动。若本地 Zephyr 已包含 `drivers/display/display_st75256.c`，可直接构建。首次使用本分支的 west 工作区时，从仓库根目录将随板提供的驱动补丁应用到 Zephyr，再构建固件：

```sh
git -C zephyr apply --check "$PWD/app/boards/bmk/numpad/st75256-zephyr.patch"
git -C zephyr apply "$PWD/app/boards/bmk/numpad/st75256-zephyr.patch"
```

```sh
west build -s app -b numpad//zmk -d build/numpad-normal
```

## Calculator

[Board-local calculator usage, porting and tests](calculator/README.md).

## LCD backlight diagnostics

[Read-only probe, firmware variants, measurement procedure and acceptance checks](diagnostics/README.md).

## ZMK Studio (normal firmware)

正常版默认启用 ZMK Studio，板级 DTS 和 Kconfig 已包含 USB CDC ACM 传输，
不需要额外添加 `studio-rpc-usb-uart` snippet。USB HID 键盘功能继续保留。

使用 USB 数据线连接电脑；如当前输出为蓝牙，先按 **FN + 小数点** 切换到 USB。
在 ZMK Studio 中选择该键盘的串口连接，然后按 **FN + 0** 解锁。Base、Media 和计算器模式均可使用；断开连接会重新锁定，空闲 600 秒也会锁定。
FN + NLCK、FN + DEL、FN + Enter 保持原有功能。请保留 Fn 键和解锁键，
以及计算器层、计算器 Fn 层的编号和专用绑定，避免在线改键后失去这些入口。

背光诊断配置显式设置 `CONFIG_ZMK_STUDIO=n`，保持之前的诊断基线；
隔离版仍关闭 settings、自动消隐与设备电源管理，不提供 Studio USB 串口。
关闭 Studio 的固件中 FN + 0 不执行操作。刷入新固件不会清除已保存的设置；
如以前使用 Studio 保存过键位，保存的映射可能覆盖新默认解锁键。
