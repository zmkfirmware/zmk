# LCD 背光诊断

适用：当前 numpad / nRF52840、ST75256 LCD。症状为 USB、电池供电均偶尔闪亮后变为微光，
但 LCD 内容仍正常更新。**诊断固件用于定位，不代表已经修复硬件故障。**

## 原理与边界

2021-09-17 的 numpad 原理图第 2、6、7 页给出的连接为：

```text
VSYS -> U4 (RT9193-33) -> 3V3P -> U19 -> 3V3 -> U20 -> LIGHT_POWER -> R15 (10 ohm) -> LCD A
                                 EN              EN                                  LCD K -> GND
                           P0.12 / R35      P0.08 / R36
```

U19、U20 是 SGM2554A，高电平使能。两者 EN（4 脚）开启时应达到 1.5V 以上，
关闭时应低于 0.4V。这是芯片引脚处的实测电压，不能用 GPIO 输出寄存器值替代。
参考：[SGMICRO 数据手册](https://www.sg-micro.com/rect/assets/0ffae148-84c9-4dc0-abb0-0a35017cbb1b/SGM2554.pdf)。

当前固件引脚、极性、FN + DEL 绑定均与原理图匹配。`gpio-leds` 在亮度大于零时持续输出高电平，
默认 40 并非 40% PWM；诊断配置保留这个默认值，不通过增大亮度试探。
本诊断不改通用背光驱动，不翻转极性，不增加 PWM，不擦除设置或修改供电电路。

## 固件版本

| 版本 | 额外配置（按顺序合并） | settings | 空闲熄背光／显示消隐 | 日志 |
| --- | --- | --- | --- | --- |
| normal | 无 | 保持正常默认 | 保持正常默认 | 原有日志，无探针 |
| isolated | observe.conf;isolated.conf | 禁用 | 禁用 | 开启 |
| observe | observe.conf | 保持正常默认 | 保持正常默认 | 开启 |
| no-settings | observe.conf;no-settings.conf | 禁用 | 保持正常默认 | 开启 |
| no-idle | observe.conf;no-idle.conf | 保持正常默认 | 禁用 | 开启 |

`CONFIG_NUMPAD_LCD_BACKLIGHT_DIAGNOSTICS` 默认关闭。开启后读取 P0.08/P0.12，
编译检查会拒绝将该探针误用于改过这两个引脚的 overlay。

禁用 settings 的版本跳过加载／保存，**不删除**已有配对或用户配置；不能依赖已保存的蓝牙配对。
优先使用 USB 测试按键输出，电池测试只验证本地显示和背光。刷回 normal 后重新加载原设置。
observe/no-idle 版本保留正常保存行为，手动按 FN + DEL 会照常保存背光开关状态，测试结束时请恢复原状态。

所有诊断配置显式关闭 ZMK Studio，保持加入 Studio 前的诊断基线。
正常版默认启用 Studio USB；隔离版仍不加载 settings，也不提供 Studio USB 串口。

## 构建

从仓库根目录执行。使用当前环境的 west（本仓库可用 `.venv/bin/west`）：

```sh
west build -s app -b numpad//zmk -d /tmp/numpad-lcd-normal
west build -s app -b numpad//zmk -d /tmp/numpad-lcd-isolated -- \
  '-DEXTRA_CONF_FILE=boards/bmk/numpad/diagnostics/observe.conf;boards/bmk/numpad/diagnostics/isolated.conf'
west build -s app -b numpad//zmk -d /tmp/numpad-lcd-observe -- \
  -DEXTRA_CONF_FILE=boards/bmk/numpad/diagnostics/observe.conf
```

`EXTRA_CONF_FILE` 中的相对路径相对于 `app/`。其余两个版本以相同方式合并表中配置，
各使用独立构建目录。SDK 不能自动发现时，显式传入
`-DZEPHYR_TOOLCHAIN_VARIANT=zephyr -DZEPHYR_SDK_INSTALL_DIR=<本机 SDK 目录>`。

每个构建目录的 `zephyr/zmk.uf2` 为固件，`zephyr/.config` 为最终配置，
`zephyr/zephyr.dts` 为最终引脚配置。保留这些文件及日志来对应实际刷入版本。
通过现有 UF2 bootloader 更新，不使用整片擦除或 settings reset 固件。

## 读取日志

使用 SWD 调试器连接 T1 的 VTref、GND、SWDIO、SWCLK，在 J-Link RTT Viewer 等 RTT 工具中
选择 nRF52840、SWD 和通道 0。板级配置使用 RTT，普通 USB 键盘连接不会自动提供这份日志。
没有 SWD 调试器也可以刷 isolated 进行电压测量，但无法用 RTT 日志直接判定软件状态。
观察配置使用非阻塞 RTT 丢弃模式及 4096 字节缓冲；没有读取器或缓冲已满时会丢日志，
不会等待调试器。需要捕获启动日志时，先连接读取器再复位。

探针在 main() 加载 settings **之前**输出 `before_settings`，随后每秒采样，按键／活动事件后
再安排一次采样。日志包含：

- `settings/auto_idle/auto_usb/blank_idle`：本次固件的实际开关。
- `t`：上电后的毫秒数；`activity=0/1/2`：活动／空闲／休眠。
- `bl_ready`：背光驱动是否就绪；`bl_on/brt`：软件开关与有效亮度。
- `power`：EXT_POWER 驱动报告的状态，1 开、0 关、负值为错误。
- `P0.08_OUT/P0.12_OUT`：nRF 输出锁存位；`CNF08/CNF12`：原始 PIN_CNF 寄存器。
- `key pos/down`：物理按键事件；默认布局 FN 的位置为 16，DEL 为 1。

PIN_CNF 的 DIR 位（bit 0）为 1 表示输出，INPUT 位（bit 1）为 1 表示输入缓冲断开；
PULL 在 bits 3:2，DRIVE 在 bits 10:8。探针不修改这些字段。

正常打开背光时预计 `bl_on=1 brt=40 power=1 P0.08_OUT=1 P0.12_OUT=1`。
关闭背光时预计 P0.08_OUT=0，而 P0.12_OUT 保持 1。日志显示 OUT=1 **不证明**芯片 EN 达到高电平，
仍需测量 R35/R36 两端。每秒及事件后采样也不能保证捕获所有瞬态，瞬态掉压应使用示波器。

## 排查顺序

1. 刷 isolated，以 USB 供电开机观察至少 60 秒；按 FN + DEL 做 20 次完整开关，
   普通模式和计算器模式都测试。电池供电重复同样步骤，确认文字仍更新。
2. 记录闪亮瞬间和微亮后的 U20 EN、VIN、VOUT，以及 U19 EN、3V3P、3V3、LIGHT_POWER。
   检查实物芯片型号、R15 实装阻值及 SB1 是否闭合，与原理图对照。
3. isolated 正常、observe 异常时，再分别测试 no-settings 和 no-idle：
   no-settings 正常提示保存状态相关；no-idle 正常提示空闲管理相关。两者都异常而 isolated 正常，
   需检查两项交互和启动过程，不单凭这个结果修改极性。
4. isolated 仍异常时，优先按下面的实测表分段定位硬件；除非日志证明控制状态异常，否则不改通用驱动。

| 实测现象 | 下一步 |
| --- | --- |
| U20 EN 随变暗降为低电平 | 与 OUT 对照。OUT 也低：追踪 settings、按键或活动事件；OUT 高但 EN 低：检查 R36、焊接、走线、U20 EN 输入及 MCU 实际 IO 电压。 |
| U20 EN 稳定为高，但 VIN 掉压 | 沿 3V3、U19、3V3P、U4、VSYS 查找首先掉压的位置，同时测 U19 EN 和核对 SB1。 |
| U20 EN、VIN 稳定，但 VOUT 降低／脉动 | 检查 U20、C62、背光支路异常电流／短路及保护动作；按证据维修，不反复强制使能。 |
| LIGHT_POWER 稳定而背光微亮 | 检查 R15 是否为 10Ω、A/K 电压、连接器和背光规格；若实装为 10Ω，支路电流约为 R15 压降除以 10Ω。不要盲目减小限流电阻。 |

若确认保存状态问题，只恢复相关背光／LCD 电源状态，不清空整个 NVS。
若确认空闲恢复问题，先复现并加入相关回归，再修改触发故障的状态路径。
硬件维修范围由上述测量结果决定，当前未进行硬件维修，也未声称某个器件已经损坏。

## 测量记录与验收

可复制下表，分别填写 USB／电池、开启瞬间／变暗后／关闭三种状态：

| 固件、供电、状态、时间 | bl_on/brt/power | OUT08/OUT12 | U19 EN | 3V3P | 3V3（U20 VIN） | U20 EN | LIGHT_POWER | R15 压降 | 背光 A/K 电压 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 待测 | | | | | | | | | |

最终验收：持续开启至少 60 秒不闪灭；USB／电池各开关 20 次；正常版 30 秒空闲熄灭、按键唤醒、
手动关闭后空闲／唤醒、断电重启行为符合预期；两种模式 FN + DEL 一致，计算器和按键灯正常。
构建通过和静态检查不能替代这些实机验收。
