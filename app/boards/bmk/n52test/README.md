# BMK n52test

`n52test` 是一块用于验证 BlackBerry Trackballer、四路 PWM LED 和 4×5 矩阵键盘的
nRF52840 测试板。默认启用 USB、BLE、外部 32.768 kHz 晶振、普通 DC/DC 和 VDDH 高压
DC/DC，并为已预刷 BMK/Adafruit UF2 bootloader 生成 `zmk.uf2`。

## GPIO

| 功能               | GPIO                              | 配置                   |
| ------------------ | --------------------------------- | ---------------------- |
| Trackball BLU      | P1.05                             | PWM0 channel 3，高有效 |
| Trackball RED      | P0.02                             | PWM0 channel 2，高有效 |
| Trackball GRN      | P0.31                             | PWM0 channel 1，高有效 |
| Trackball WHT      | P0.30                             | PWM0 channel 0，高有效 |
| Trackball UP       | P0.28                             | 高有效脉冲             |
| Trackball DWN      | P0.27                             | 高有效脉冲             |
| Trackball LFT      | P0.26                             | 高有效脉冲             |
| Trackball RHT      | P0.25                             | 高有效脉冲             |
| Trackball BTN      | P1.06                             | 低有效、内部上拉       |
| LED profile 测试键 | P1.09                             | 低有效、内部上拉       |
| Matrix columns     | P0.13、P0.14、P0.15、P0.16        | 高有效、下拉           |
| Matrix rows        | P0.17、P0.21、P0.19、P0.20、P0.22 | 高有效                 |

矩阵二极管方向为 `row2col`。Trackball breakout 使用 3.3 V 并与 MCU 共地。

## 默认 keymap

默认 keymap 参照 `numpadv2`，包含 Base、Media、Fn 三层。4×5 Base 层为：

```text
NumLock   Delete    Backspace   Divide
7         8         9           Multiply
4         5         6           Minus
1         2         3           Plus
Fn        0         Dot         Enter
```

Fn 层左上两个键分别降低和提高 Trackball 速度档位；其余按键提供 BLE profile、清除配对和
LED Matrix 控制。Fn 层右下角切换 Media 层。
P1.09 独立按键在所有层中执行 `&lm LM_CFF 0`，依次循环：

1. 四灯 25%
2. WHT 25%
3. GRN 25%
4. RED 25%
5. BLU 25%
6. 全关

Trackball BTN 默认直接作为鼠标左键。如需右键，可把 DTS 中的 `button-code` 改为
`INPUT_BTN_1`，也可以按照 BlackBerry Trackball 指南使用 input processor 绑定任意 ZMK
behavior。

## Trackball 灵敏度

本板根据实测约 5–23 Hz 的方向脉冲范围，将加速区间设置为 5–30 Hz，并保留 1–8 步长：

```dts
acceleration-start-hz = <5>;
acceleration-max-hz = <30>;
acceleration-exponent = <2>;
minimum-step = <1>;
maximum-step = <8>;
report-period-ms = <4>;
interpolation-frames = <4>;
```

驱动使用通用默认的二次加速曲线。约 10、20、23 Hz 时，步长分别约为
1.28、3.52、4.63；达到 30 Hz 时使用最大步长 8。与原线性曲线相比，慢滚和中速定位更细，
越接近 30 Hz，步长增长越快。需要恢复等比例增长时可改为
`acceleration-exponent = <1>`。
未设置 `minimum-pulse-interval-us`，因此使用默认值 `0`，方向脉冲不进行去毛刺过滤。

驱动完成上述加速后，`trackball_speed` processor 再提供五档 DPI 式倍率：

| 档位 | 倍率 | 最大步长约值 | 适用场景           |
| ---- | ---- | ------------ | ------------------ |
| 1    | 150% | 12           | 小屏幕、精细定位   |
| 2    | 300% | 24           | 默认档位           |
| 3    | 450% | 36           | 中大屏幕           |
| 4    | 600% | 48           | 大屏幕             |
| 5    | 900% | 72           | 超宽屏、快速跨屏幕 |

默认使用第 2 档。Fn 层第 1、2 个键分别执行
`&tbspd INPUT_SPEED_DOWN` 和 `&tbspd INPUT_SPEED_UP`；到第 1/5 档后停止，不循环。当前档位
通过 ZMK settings 保存，断电或重启后仍会恢复。`track-remainders` 分别保留 X/Y 的百分比
缩放余数。

驱动会把每个脉冲的整数位移分摊到最多四个 4 ms 报告中。默认 300% 档的高速位移由原本
一次约 24 改为四次约 6；900% 档由一次约 72 改为四次约 18。总移动距离和平均速度不变，
只改善高倍率下的视觉平滑度。反向脉冲会先抵消尚未发送的同轴积压；没有新脉冲时，最长插值
尾窗为 16 ms。单个 1-step 慢速脉冲仍在第一帧立即发送。

若需要改变五档比例，编辑 `n52test_nrf52840_zmk.dts` 中的
`speed-percentages = <150 300 450 600 900>;`。若硬件测试确认需要过滤原始杂波，可再设置
`minimum-pulse-interval-us = <4000>`；基础速度曲线仍由 5–30 Hz 和 1–8 step 参数决定。

## 电源要求

本板启用了 nRF52840 的两级 DC/DC：

- `reg0`：VDDH 高压 DC/DC。
- `reg1`：主电源 DC/DC。

PCB 必须按照 Nordic nRF52840 参考电路配置 VDDH、DECUSB、REG0/REG1 电感及去耦电容。
缺少相应外围时不要使用此电源配置。

## UF2 分区

| 分区           | 地址       | 大小       |
| -------------- | ---------- | ---------- |
| MBR            | `0x000000` | `0x001000` |
| ZMK            | `0x001000` | `0x0D3000` |
| Settings/NVS   | `0x0D4000` | `0x020000` |
| UF2 bootloader | `0x0F4000` | `0x00C000` |

此布局只适用于已经刷入兼容 BMK/Adafruit nRF52840 UF2 bootloader 的硬件。

## 构建和刷写

在仓库根目录运行：

```sh
west build -s app -p -b n52test/nrf52840/zmk
```

构建完成后，将 `build/zephyr/zmk.uf2` 复制到双击复位后出现的 UF2 磁盘。不要擦除或覆盖
`0xF4000–0xFFFFF` bootloader 区域。

## 硬件检查

1. 逐个测试 20 个矩阵按键，确认每个位置唯一且没有鬼键。
2. 保持 Trackball 静止，确认指针不会持续抖动。
3. 滚动 Trackball，确认左、右、上、下方向、慢速精度和速度加速；重点检查 45° 对角滚动
   是否平滑。
4. 依次按 Fn+减档/加档键，确认五档边界不循环，150% 和 900% 的速度差异明显；重启后确认
   最后档位已恢复。
5. 按下 Trackball BTN，确认鼠标左键按下和释放均正常。
6. 按 P1.09，确认六个 LED profile 按顺序循环且 PWM 亮度稳定。
7. 分别验证 USB、BLE、休眠唤醒和 UF2 双击复位。
8. 在高速档突然停止并快速反向，确认没有超过约 16 ms 的旧方向尾随。
