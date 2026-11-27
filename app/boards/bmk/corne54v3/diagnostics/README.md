# Corne54v3 左手死机对照与取证

11 月 26 日原版、同日源码仅关闭灯光通知的对照版、修复版分别配套自己的 UF2 和 ELF。右手在三轮中使用同一份原版。对照版在修改公共发送路径前构建。当前源码设置 `CONFIG_ZMK_LED_MATRIX_STUDIO_NOTIFICATIONS=n` 会同时包含公共发送修复，不能替代已保存的对照版。

## 实机对照

每轮记录左手使用的 UF2、右手版本、实际 HID 输出（USB 或蓝牙）、供电方式、Studio 是否连接、开始和故障时间。先按平常节奏打字、切层和使用灯光，在原版记录能触发故障的动作与用时。用相同负载测试对照版和修复版，覆盖两个 HID 端点和 Studio 打开、关闭两种状态。区分左手本地按键、右手按键和屏幕刷新。修复版至少覆盖原版复现用时的三倍，并验证断开重连后的首条 Studio 请求。原版没有复现时只报告无故障观察时长。

## 独立 SWD 记录

将 SWD 调试器接到左手。保留设备当前运行状态，启动本地 J-Link GDB Server（SWD、nRF52840、端口 2331、连接时不复位），使用 Zephyr SDK 的 arm-zephyr-eabi-gdb 打开与该轮 UF2 匹配的 ELF。下面的 GDB 命令从仓库根目录运行。每轮使用单独日志名。

```gdb
target remote localhost:2331
set logging file corne-left-original.log
set logging overwrite off
set logging enabled on
source app/boards/bmk/corne54v3/diagnostics/freeze.gdb
corne-trace-original
continue
```

对照版也使用 `corne-trace-original`。修复版使用 `corne-trace-fixed`。故障发生时先按 Ctrl-C，再运行 `corne-snapshot`，保存结果后再重启左手。这些命令使用硬件断点，诊断日志通过调试器输出。命中断点会短暂停核，可能改变蓝牙时序。若影响复现，可在日常运行出现故障后直接附加，执行 snapshot 取得当前调用栈；此时没有历史时间戳。

时间为内核 tick，三份左手固件每秒 32768 tick。-1 表示该断点尚未命中。按键和显示时间记录对应工作项的进入时刻。原版发送进展取自 RPC 缓冲区消费位置，修复版记录传输完成回调报告正字节数的时刻。寄存器 RESETREAS 是该次启动以来保留的复位标志，须结合当前 PC、调用栈、供电和人工重启记录判断。若 PC 在空闲线程或中断，保留系统工作队列线程的保存寄存器，使用同一 ELF 进一步还原其调用栈。

## 自动回归

从仓库根目录使用已配置的 Zephyr 工具链，先构建左手固件，再运行独立测试。STUDIO_BUILD 指向该左手构建目录，测试会复用对应的 protobuf 生成文件。

```sh
west build -s app -b corne54v3_left -S studio-rpc-usb-uart -d build/corne-left -- -DCONFIG_ZMK_STUDIO=y
west build -s app/tests/studio/transport -b qemu_cortex_m3 -d build/studio-transport-test -- -DSTUDIO_BUILD="$PWD/build/corne-left"
west build -d build/studio-transport-test -t run
```

测试运行实际的 Zephyr 队列、事件分发、Studio 编解码和传输回调，UART FIFO 与蓝牙对端采用测试替身。
