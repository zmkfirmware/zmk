# BlackBerry Trackballer 代码说明与快速入门

本文介绍 ZMK 树内的 SparkFun BlackBerry Trackballer 支持，包括硬件信号、驱动实现、
Devicetree 配置、按键自定义、四颗 PWM LED，以及 `n52test` 测试板实例。

如果只需要快速接线和使用，可以先阅读“最小配置”和“`n52test` 实例”；如果需要修改驱动，
再阅读“驱动代码说明”。更完整的通用集成说明以
[`blackberry-trackball.mdx`](docs/docs/hardware-integration/blackberry-trackball.mdx) 为准。

## 1. 功能简介

SparkFun BlackBerry Trackballer Breakout 将轨迹球的运动转换为四路数字脉冲：

- `LFT`：向左滚动。
- `RHT`：向右滚动。
- `UP`：向上滚动。
- `DWN`：向下滚动。
- `BTN`：按下轨迹球时产生的独立按键输入。
- `WHT`、`GRN`、`RED`、`BLU`：四颗可独立控制的 LED。

ZMK 驱动使用 GPIO 中断捕获方向脉冲，根据滚动频率计算移动步长，再通过 Zephyr Input
子系统发送相对 X/Y 事件。滚得越快，指针步长越大。BTN 默认可作为鼠标左键，也可以映射为
鼠标右键、键盘键、layer、macro 或其他 ZMK behavior。

四颗 LED 不属于输入驱动本身。它们通过 Zephyr `pwm-leds` 和 ZMK LED Matrix 控制，因此
可以单独点亮、组合点亮并调整 PWM 亮度。

### 数据流

```text
LFT/RHT/UP/DWN GPIO 有效沿
        │
        ▼
每方向最小脉冲间隔过滤
        │
        ▼
方向与相邻已接受脉冲时间计算
        │
        ▼
Q16/Q8 定点指数加速、有符号累加与反向抵消
        │
        ▼
report-period-ms 延迟 work 合并 X/Y
并按 interpolation-frames 分帧取出
        │
        ▼
Zephyr Input INPUT_REL_X / INPUT_REL_Y
        │
        ▼
ZMK Input Listener 与 Input Processors
（可选的运行时速度档位按百分比缩放）
        │
        ▼
USB/BLE HID 鼠标报告
```

BTN 使用另一条路径：

```text
BTN 双沿中断 → debounce-interval-ms 延迟采样 → INPUT_BTN_* → 鼠标键或 ZMK behavior
```

## 2. 相关文件目录

```text
zmk/
├── BLACKBERRY_TRACKBALLER.md
├── app/
│   ├── module/
│   │   ├── drivers/input/
│   │   │   ├── input_blackberry_trackballer.c
│   │   │   ├── Kconfig
│   │   │   └── CMakeLists.txt
│   │   └── dts/bindings/input/
│   │       └── sparkfun,blackberry-trackballer.yaml
│   ├── dts/bindings/
│   │   ├── behaviors/zmk,behavior-input-speed.yaml
│   │   └── input_processors/zmk,input-processor-speed-profile.yaml
│   ├── include/
│   │   ├── drivers/input_processor_speed_profile.h
│   │   └── dt-bindings/zmk/input_speed.h
│   ├── src/
│   │   ├── behaviors/behavior_input_speed.c
│   │   └── pointing/input_processor_speed_profile.c
│   ├── boards/bmk/n52test/
│   │   ├── Kconfig
│   │   ├── Kconfig.defconfig
│   │   ├── Kconfig.n52test
│   │   ├── README.md
│   │   ├── board.cmake
│   │   ├── board.yml
│   │   ├── n52test-pinctrl.dtsi
│   │   ├── n52test.keymap
│   │   ├── n52test.yaml
│   │   ├── n52test.zmk.yml
│   │   ├── n52test_layouts.dtsi
│   │   ├── n52test_nrf52840_zmk.dts
│   │   ├── n52test_nrf52840_zmk_defconfig
│   │   └── pre_dt_board.cmake
│   └── tests/pointing/blackberry-trackballer/
│       ├── acceleration/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── acceleration-curve/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── button-key/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── button-left/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── button-right/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── directions/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── deglitch/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── interpolation/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── pm/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── speed-profile/
│       │   ├── events.patterns
│       │   ├── keycode_events.snapshot
│       │   ├── native_sim.conf
│       │   └── native_sim.keymap
│       ├── speed-profile-saturation/
│       │   └── ...
│       └── speed-profile-settings/
│           └── ...
└── docs/docs/hardware-integration/
    ├── blackberry-trackball.mdx
    └── pointing.mdx
```

### 文件职责

| 文件或目录                                                                                                           | 作用                                                                          |
| -------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------- |
| [`input_blackberry_trackballer.c`](app/module/drivers/input/input_blackberry_trackballer.c)                          | GPIO 中断、速度加速、X/Y 报告、BTN 消抖和电源管理                             |
| [`Kconfig`](app/module/drivers/input/Kconfig)                                                                        | 定义 `CONFIG_INPUT_SPARKFUN_BLACKBERRY_TRACKBALLER`，有启用节点时自动选择驱动 |
| [`CMakeLists.txt`](app/module/drivers/input/CMakeLists.txt)                                                          | 在对应 Kconfig 启用时编译驱动源码                                             |
| [`sparkfun,blackberry-trackballer.yaml`](app/module/dts/bindings/input/sparkfun,blackberry-trackballer.yaml)         | 定义 Devicetree compatible、GPIO、轴、按键和加速参数                          |
| [`input_processor_speed_profile.c`](app/src/pointing/input_processor_speed_profile.c)                                | 运行时档位、百分比缩放、XY 余数、饱和与 settings 持久化                       |
| [`behavior_input_speed.c`](app/src/behaviors/behavior_input_speed.c)                                                 | 处理减档、加档和直选档位；split 时在 central 转换为绝对档位                   |
| [`zmk,input-processor-speed-profile.yaml`](app/dts/bindings/input_processors/zmk,input-processor-speed-profile.yaml) | 定义档位百分比、默认档位和持久化属性                                          |
| [`zmk,behavior-input-speed.yaml`](app/dts/bindings/behaviors/zmk,behavior-input-speed.yaml)                          | 定义速度切换 behavior 的 processor 引用                                       |
| [`input_speed.h`](app/include/dt-bindings/zmk/input_speed.h)                                                         | 定义 `INPUT_SPEED_DOWN/UP/SET` keymap 命令                                    |
| [`n52test_nrf52840_zmk.dts`](app/boards/bmk/n52test/n52test_nrf52840_zmk.dts)                                        | `n52test` 的 Trackball、矩阵键盘、PWM LED、电源和 Flash 分区                  |
| [`n52test-pinctrl.dtsi`](app/boards/bmk/n52test/n52test-pinctrl.dtsi)                                                | PWM0 四个输出通道的 nRF pinctrl 配置                                          |
| [`n52test.keymap`](app/boards/bmk/n52test/n52test.keymap)                                                            | 参照 `numpadv2` 的默认三层 keymap，以及 P1.09 LED profile 测试键              |
| [`n52test_nrf52840_zmk_defconfig`](app/boards/bmk/n52test/n52test_nrf52840_zmk_defconfig)                            | 启用 USB、BLE、PWM、Pointing、LED Matrix、UF2 和 Settings                     |
| [`n52test_layouts.dtsi`](app/boards/bmk/n52test/n52test_layouts.dtsi)                                                | 20 个矩阵键与一个独立测试键的物理布局                                         |
| `app/boards/bmk/n52test/Kconfig*`                                                                                    | 板级 SoC、名称、USB 控制器和 ZMK 兼容配置                                     |
| `board.yml`、`n52test.yaml`、`n52test.zmk.yml`                                                                       | Zephyr/ZMK 板级元数据                                                         |
| `board.cmake`、`pre_dt_board.cmake`                                                                                  | UF2/烧录 runner 和 Devicetree 编译参数                                        |
| [`n52test/README.md`](app/boards/bmk/n52test/README.md)                                                              | `n52test` 接线、构建、刷写和硬件检查说明                                      |
| `directions/`                                                                                                        | 验证左右上下符号、抵消、对角移动和同步报告                                    |
| `acceleration/`                                                                                                      | 验证显式指数 1 的线性兼容、速度上下限和静止后的最小步长                       |
| `acceleration-curve/`                                                                                                | 验证未声明指数时采用默认二次曲线，以及低、中、高速输出                        |
| `deglitch/`                                                                                                          | 验证同方向高频杂波过滤且不污染后续速度计算                                    |
| `interpolation/`                                                                                                     | 验证位移分帧、总量守恒、对角同步、叠加、抵消和默认兼容行为                    |
| `button-left/`、`button-right/`                                                                                      | 验证默认左键和右键映射                                                        |
| `button-key/`                                                                                                        | 验证 BTN 消抖以及映射为普通 ZMK 按键                                          |
| `pm/`                                                                                                                | 验证 suspend/resume 后没有旧运动、虚假点击或卡键                              |
| `speed-profile/`                                                                                                     | 验证五档倍率、XY 余数、边界、直选和非移动事件透传                             |
| `speed-profile-saturation/`                                                                                          | 验证正负相对移动饱和到 `INT16` 范围                                           |
| `speed-profile-settings/`                                                                                            | 验证默认值、settings 恢复、越界回退和延迟保存                                 |
| [`blackberry-trackball.mdx`](docs/docs/hardware-integration/blackberry-trackball.mdx)                                | 通用硬件集成与配置的中文版权威指南                                            |
| [`pointing.mdx`](docs/docs/hardware-integration/pointing.mdx)                                                        | ZMK Pointing/Input Listener 的通用说明                                        |

每个 simulator 测试目录中的文件用途相同：

- `native_sim.keymap`：测试用 Devicetree、模拟 GPIO 和事件序列。
- `native_sim.conf`：测试构建所需的 Kconfig。
- `events.patterns`：从模拟器日志中提取需要比较的事件。
- `keycode_events.snapshot`：预期结果快照。

## 3. 驱动启用方式

驱动使用以下 compatible：

```dts
compatible = "sparkfun,blackberry-trackballer";
```

当 Devicetree 中存在启用状态的节点时，以下 Kconfig 会自动启用：

```text
CONFIG_INPUT_SPARKFUN_BLACKBERRY_TRACKBALLER=y
```

其启用条件为：

- Zephyr Input 子系统已经启用。
- Devicetree 存在 `sparkfun,blackberry-trackballer` 节点。
- GPIO 驱动可用。

`CMakeLists.txt` 只在该配置为 `y` 时编译
`input_blackberry_trackballer.c`，不需要用户手工把源文件加入板级构建。

## 4. 最小配置

Trackball 使用 3.3 V 供电，并且必须与 MCU 共地。方向输出按照 SparkFun breakout 的
有效高脉冲配置；BTN 通常使用低有效和内部上拉。

```dts
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    trackball: trackball {
        compatible = "sparkfun,blackberry-trackballer";

        left-gpios = <&gpio0 26 GPIO_ACTIVE_HIGH>;
        right-gpios = <&gpio0 25 GPIO_ACTIVE_HIGH>;
        up-gpios = <&gpio0 28 GPIO_ACTIVE_HIGH>;
        down-gpios = <&gpio0 27 GPIO_ACTIVE_HIGH>;
        button-gpios = <&gpio1 6 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;

        zephyr,axis-x = <INPUT_REL_X>;
        zephyr,axis-y = <INPUT_REL_Y>;
    };

    trackball_listener: trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball>;
    };
};
```

方向遵循 HID 语义：

| 方向 | 轴  | 报告值 |
| ---- | --- | ------ |
| LFT  | X   | 负值   |
| RHT  | X   | 正值   |
| UP   | Y   | 负值   |
| DWN  | Y   | 正值   |

如果轨迹球安装方向不同，不建议交换驱动内部的轴定义。应在 listener 上使用现有 ZMK input
transformer 完成 XY 交换或反向。

## 5. Devicetree 配置接口

### 必填属性

| 属性            | 说明                                 |
| --------------- | ------------------------------------ |
| `left-gpios`    | LFT 有效高脉冲 GPIO                  |
| `right-gpios`   | RHT 有效高脉冲 GPIO                  |
| `up-gpios`      | UP 有效高脉冲 GPIO                   |
| `down-gpios`    | DWN 有效高脉冲 GPIO                  |
| `button-gpios`  | BTN GPIO，通常为低有效并启用内部上拉 |
| `zephyr,axis-x` | 水平相对事件，通常为 `INPUT_REL_X`   |
| `zephyr,axis-y` | 垂直相对事件，通常为 `INPUT_REL_Y`   |

### 可调属性

| 属性                        |        默认值 | 作用和约束                                         |
| --------------------------- | ------------: | -------------------------------------------------- |
| `button-code`               | `INPUT_BTN_0` | BTN 输出的 input code                              |
| `report-period-ms`          |           `8` | 合并 X/Y 及插值帧的周期，必须大于 0                |
| `interpolation-frames`      |           `1` | 位移最多分摊的报告帧数，范围 1–255；`1` 表示关闭   |
| `debounce-interval-ms`      |           `5` | BTN 延迟采样消抖时间，必须大于 0                   |
| `minimum-pulse-interval-us` |           `0` | 同方向运动脉冲的最小间隔，非负；`0` 表示禁用过滤   |
| `acceleration-start-hz`     |          `20` | 小于或等于该频率时使用最小步长，必须大于 0         |
| `acceleration-max-hz`       |         `120` | 大于或等于该频率时使用最大步长，必须大于起始频率   |
| `acceleration-exponent`     |           `2` | 归一化频率的曲线指数，范围 1–4；`1` 表示线性       |
| `minimum-step`              |           `1` | 慢速脉冲的相对移动量，必须大于 0                   |
| `maximum-step`              |           `8` | 快速脉冲的相对移动量，不小于最小步长且不超过 32767 |

完整调校示例：

```dts
&trackball {
    button-code = <INPUT_BTN_0>;
    report-period-ms = <4>;
    interpolation-frames = <4>;
    debounce-interval-ms = <5>;
    minimum-pulse-interval-us = <4000>;
    acceleration-start-hz = <20>;
    acceleration-max-hz = <120>;
    acceleration-exponent = <2>;
    minimum-step = <1>;
    maximum-step = <8>;
};
```

这些参数由驱动中的 `BUILD_ASSERT` 在编译期校验，错误配置会直接使构建失败。

## 6. 驱动代码说明

### 6.1 配置结构体

`struct trackballer_config` 保存来自 Devicetree 的只读配置：

- 四个方向的 `gpio_dt_spec`。
- BTN 的 `gpio_dt_spec`。
- X/Y event code 和 BTN event code。
- 报告周期、插值帧数、BTN 消抖时间、方向脉冲过滤间隔、加速阈值和曲线指数。

每个启用的 Devicetree 实例都有一个独立的配置对象，因此驱动没有写死 `n52test` 或任何
具体 GPIO。

### 6.2 运行时数据

`struct trackballer_data` 保存运行期间会变化的状态：

- 四个方向中断的 callback 和 BTN callback。
- 运动报告与 BTN 消抖使用的两个 delayable work。
- 保护 X/Y 累加数据的 spinlock。
- `movement_q8[2]`：X/Y 的 Q8 定点累计移动量。
- `interpolation_frames_remaining[2]`：X/Y 各自还可使用的插值帧数。
- `last_direction_pulse_ticks[4]`：四个方向上一次被接受的脉冲时间。
- `have_last_direction_pulse[4]`：四个方向是否已有去毛刺时间基线。
- `last_pulse_ticks[2]`：每个轴上一次脉冲的时间。
- `have_last_pulse[2]`：每个轴是否已有可用于测速的前一脉冲。
- 当前 BTN 状态和 suspend 状态。

X/Y 分别计时，所以水平方向与垂直方向的滚动速度互不干扰；同一轴反向时继续使用该轴最近
一次脉冲间隔计算速度。

### 6.3 方向与符号

驱动内部固定使用以下索引：

```text
LEFT=0  RIGHT=1  UP=2  DOWN=3
X=0     Y=1
```

`trackballer_direction_axis()` 将 LEFT/RIGHT 映射到 X，将 UP/DOWN 映射到 Y。
`trackballer_direction_sign()` 为 RIGHT/DOWN 返回 `+1`，为 LEFT/UP 返回 `-1`。

四个 GPIO 只监听 `GPIO_INT_EDGE_TO_ACTIVE`。方向信号是运动脉冲，不使用按键式延迟消抖，
否则快速滚动时可能丢失脉冲。

### 6.4 Q16/Q8 定点指数加速

`trackballer_step_q8()` 根据相邻脉冲间隔计算当前轴的脉冲频率：

```text
frequency_hz = 1,000,000 / delta_us
```

通用默认曲线为：

- 首个脉冲或频率不超过 20 Hz：`1 step`。
- 频率达到或超过 120 Hz：`8 steps`。
- 20–120 Hz 之间：默认使用指数为 2 的二次曲线。

驱动先把阈值之间的频率归一化为 `x`：

```text
x = (frequency - acceleration_start_hz)
    / (acceleration_max_hz - acceleration_start_hz)

step = minimum_step
     + (maximum_step - minimum_step)
     × x^acceleration_exponent
```

归一化曲线使用 Q16 定点数，最终步长使用 Q8 定点数，即低 8 位表示小数：

```text
1.0 step = 1 << 8 = 256
```

默认指数 2 会让慢速区增长更缓，并把更明显的加速集中在高速区。指数 1 使用原有线性计算
路径；指数 3 或 4 会进一步强化这种效果。多个脉冲积累后，小数会转化为完整移动量，不会
因为每次取整持续丢失。若计时差无效或小于计时器可表达范围，驱动安全地使用最大步长。

### 6.5 运动累加与报告

方向中断执行以下操作：

1. 判断设备是否处于 suspend。
2. 获取该方向对应的轴和正负号。
3. 在 spinlock 内检查该方向与上一次已接受脉冲的间隔；间隔不足时直接丢弃。
4. 对通过过滤的脉冲计算 Q8 步长并进行有符号累加。
5. 将该轴剩余插值帧数重置为 `interpolation-frames`。
6. 调度 `report_work`。

过滤状态按四个方向独立保存，因此右转后立即左转不会被误判为同方向杂波。被丢弃的脉冲不会
更新轴测速时间戳，也不会错误触发高速加速。`minimum-pulse-interval-us = <0>` 保持原有的
有效沿即刻计数行为。

同一报告周期内的相反方向会直接抵消。例如 X 上先累加 `+1`，再累加 `-1`，最终不会产生
水平移动。X 和 Y 在同一次报告中发送，因此对角滚动不会被拆成无关的单轴动作。

`trackballer_take_axis_value()` 先取 Q8 积压中的完整整数位移，再按该轴剩余帧数平均分摊。
除法向远离零的方向取整，让不足帧数的小位移在第一帧立即发送；实际发送值会从 Q8 积压中
扣除，定点小数继续保留。结果限制在 `INT16_MIN` 到 `INT16_MAX`；若积压超过单次 HID
范围，后续 work 会继续排空。

例如步长 8、`interpolation-frames = <4>` 时会发送 `2、2、2、2`，总和仍为 8。
如果 listener 后方再使用 300% 速度档位，则主机收到 `6、6、6、6`，总和仍为 24。步长只有
1 时会在第一帧直接发送 1，不会为了凑满四帧生成空报告或增加额外延迟。

X/Y 分别维护 Q8 积压和帧状态，但在每个 work 中同步取值，因此 45° 对角移动仍会形成同步
X/Y 报告。新脉冲可叠加到尚未发完的积压，并重置对应轴的帧数；反向脉冲先与同轴未发送位移
进行有符号抵消，不会继续输出旧方向尾随。`interpolation-frames = <1>` 与原始一次报告行为
一致。

报告顺序为：

```text
INPUT_REL_X，sync=false
INPUT_REL_Y，sync=true
```

Y 事件上的 `sync=true` 表示 X/Y 属于同一组 input 报告。如果仍有至少一个完整 step 的余量，
work 会再次按照 `report-period-ms` 调度，避免突发脉冲溢出或丢失。

### 6.6 BTN 消抖与事件

BTN 使用 `GPIO_INT_EDGE_BOTH`，同时监听按下和释放。中断中不立即报告，而是通过
`k_work_reschedule()` 将采样推迟 `debounce-interval-ms`：

- 消抖时间内再次变化会重新开始计时。
- 延迟结束后读取 GPIO 的逻辑值。
- 只有状态与上一次不同才发送 `input_report_key()`。
- Devicetree 的 `GPIO_ACTIVE_LOW` 会由 GPIO API 转换，驱动接收到的仍是逻辑按下状态。

这保证一次实际按下只产生一个 press，释放只产生一个 release。

### 6.7 Suspend 和 Resume

收到 `PM_DEVICE_ACTION_SUSPEND` 时，驱动会：

1. 设置 suspended 标志，阻止新的 callback 处理事件。
2. 禁用四个方向和 BTN 中断。
3. 同步取消运动报告与 BTN 消抖 work。
4. 清除 X/Y 积压、插值帧状态、方向过滤时间戳、测速时间戳和首脉冲状态。
5. 如果 BTN 正处于按下状态，主动发送 release，避免主机端卡键。

收到 `PM_DEVICE_ACTION_RESUME` 时，驱动会重新清除旧运动、配置 GPIO、读取 BTN 基线、恢复
中断，最后清除 suspended 标志。因此唤醒后不会发送休眠前残留移动或虚假点击。

驱动注册 device PM callback，但不会自行调用 `pm_device_runtime_enable()`。ZMK input listener
是长期监听者，目前不会为输入设备调用 `pm_device_runtime_get()`；如果驱动在初始化时自行启用
runtime PM，Zephyr 会立即将其挂起并禁用所有方向及 BTN 中断。保持 runtime PM 未在该设备上
启用，可以确保启动后立即接收输入，同时仍允许系统休眠或显式 PM action 调用上述
suspend/resume 实现。全局启用 `CONFIG_PM_DEVICE_RUNTIME` 不会自动改变该设备的状态。

### 6.8 Devicetree 实例生成

`TRACKBALLER_DEFINE(inst)` 为每个启用节点生成：

- 一个 `trackballer_config_<inst>` 常量。
- 一个 `trackballer_data_<inst>` 运行时对象。
- 一个 PM device。
- 一个 Zephyr Input device。

驱动通过 `DT_INST_FOREACH_STATUS_OKAY(TRACKBALLER_DEFINE)` 支持多个实例，不提供额外的 C
runtime API；所有公开配置都来自 Devicetree 和现有 ZMK Input Processor 接口。

## 7. 速度调校

| 现象               | 建议                                                     |
| ------------------ | -------------------------------------------------------- |
| 慢滚过快           | 降低 `minimum-step`；已经为 1 时选择低倍率运行时速度档位 |
| 加速出现太早       | 提高 `acceleration-start-hz`                             |
| 很难进入高速       | 降低 `acceleration-max-hz`                               |
| 中低速增长仍太快   | 提高 `acceleration-exponent`，最大可设为 4               |
| 希望恢复等比例增长 | 设置 `acceleration-exponent = <1>`                       |
| 高速仍然太慢       | 提高 `maximum-step`                                      |
| 高速档移动有跳跃感 | 设置 `interpolation-frames = <4>` 并降低报告周期到 4 ms  |
| 指针更新不够平滑   | 适当降低 `report-period-ms`                              |
| 报告频繁、功耗偏高 | 适当提高 `report-period-ms`                              |
| BTN 偶尔重复       | 将 `debounce-interval-ms` 提高到 8–10 ms                 |
| 静止时指针抖动     | 设置 `minimum-pulse-interval-us = <4000>`                |

建议每次只修改一个参数，并依次检查慢滚、持续快滚、突然反向和对角滚动。

### 7.1 运行时 DPI 式速度档位

基础加速曲线适合描述“滚得越快，单个脉冲移动越远”；运行时速度档位则用于在不同屏幕尺寸
之间快速切换整体倍率。`zmk,input-processor-speed-profile` 在驱动完成加速后，对指定的
Zephyr Input code 按百分比缩放：

```dts
trackball_speed: trackball_speed {
    compatible = "zmk,input-processor-speed-profile";
    #input-processor-cells = <0>;
    type = <INPUT_EV_REL>;
    codes = <INPUT_REL_X INPUT_REL_Y>;
    speed-percentages = <50 100 150 200 300>;
    default-speed-level = <2>;
    persist-speed-level;
    track-remainders;
};

trackball_listener: trackball_listener {
    compatible = "zmk,input-listener";
    device = <&trackball>;
    input-processors = <&trackball_speed>;
};
```

| 属性                  | 说明                                                               |
| --------------------- | ------------------------------------------------------------------ |
| `type`                | 需要缩放的事件类型，鼠标移动使用 `INPUT_EV_REL`                    |
| `codes`               | 需要缩放的 code；Trackball 通常为 `INPUT_REL_X INPUT_REL_Y`        |
| `speed-percentages`   | 从第 1 档开始排列的 1–65535 百分比，最多 255 档                    |
| `default-speed-level` | 没有有效 settings 时使用的档位，采用从 1 开始的编号                |
| `persist-speed-level` | 可选；用 `input_speed/<processor-device-name>` 保存一字节当前档位  |
| `track-remainders`    | 保留各相对轴的小数余量；50% 档下两个单位脉冲仍会累计为一个移动单位 |

百分比必须全部大于 0，默认档位必须存在。缩放使用宽整数计算并饱和到 `INT16` 范围。X、Y
分别保存余数，切换档位不会清除已经累计的余数；不在 `codes` 中的 BTN、滚轮等事件原样继续。

用 behavior 将档位切换放进 keymap：

```dts
#include <dt-bindings/zmk/input_speed.h>

/ {
    behaviors {
        tbspd: trackball_speed_behavior {
            compatible = "zmk,behavior-input-speed";
            #binding-cells = <2>;
            processor = <&trackball_speed>;
        };
    };
};

/* layer bindings 片段 */
bindings = <
    &tbspd INPUT_SPEED_DOWN
    &tbspd INPUT_SPEED_UP
    &tbspd INPUT_SPEED_SET(1)
    &tbspd INPUT_SPEED_SET(5)
>;
```

- `INPUT_SPEED_DOWN`：降低一档，到第 1 档后停止。
- `INPUT_SPEED_UP`：提高一档，到最后一档后停止。
- `INPUT_SPEED_SET(level)`：直选从 1 开始的档位；越界值不会改变当前档位。

behavior 使用 central locality。相对的减档/加档会在 central 根据当前状态转换成绝对
`SET`，因此 split peripheral 产生按键事件时不会在两侧各自维护不同档位。速度 processor
也应放在 central 的 Trackball listener 上。启用 `persist-speed-level` 时还需要
`CONFIG_SETTINGS=y`；写入遵循 `CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE`，缺失或越界的存储值会
恢复 `default-speed-level`。

## 8. 安装方向调整

使用现有 ZMK input transformer 调整 XY，而不是修改驱动：

```dts
#include <input/processors.dtsi>
#include <dt-bindings/zmk/input_transform.h>

&trackball_listener {
    input-processors = <
        &zip_xy_transform (INPUT_TRANSFORM_XY_SWAP | INPUT_TRANSFORM_Y_INVERT)
    >;
};
```

可用 flag：

- `INPUT_TRANSFORM_XY_SWAP`：交换 X/Y。
- `INPUT_TRANSFORM_X_INVERT`：反转 X。
- `INPUT_TRANSFORM_Y_INVERT`：反转 Y。

## 9. BTN 可设置的功能

### 9.1 直接作为鼠标键

| `button-code` | 功能                       |
| ------------- | -------------------------- |
| `INPUT_BTN_0` | 鼠标左键，默认值           |
| `INPUT_BTN_1` | 鼠标右键                   |
| `INPUT_BTN_2` | 鼠标中键                   |
| `INPUT_BTN_3` | Mouse Button 4，通常为后退 |
| `INPUT_BTN_4` | Mouse Button 5，通常为前进 |

例如固定为鼠标右键：

```dts
&trackball {
    button-code = <INPUT_BTN_1>;
};
```

### 9.2 映射为任意 ZMK behavior

需要键盘键、layer、macro、hold-tap 或鼠标 behavior 时，使用
`zmk,input-processor-behaviors`。processor 捕获 BTN code 后会停止原始事件继续进入鼠标报告，
只执行绑定的 behavior。

```dts
#include <behaviors.dtsi>
#include <input/processors.dtsi>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/pointing.h>

/ {
    trackball_button_behavior: trackball_button_behavior {
        compatible = "zmk,input-processor-behaviors";
        #input-processor-cells = <0>;
        codes = <INPUT_BTN_0>;

        bindings = <&kp ESC>;
    };
};

&trackball_listener {
    input-processors = <&trackball_button_behavior>;
};
```

将 `bindings` 替换为以下任意一个示例：

```dts
bindings = <&kp ENTER>;     /* 普通键 */
bindings = <&kp LC(C)>;     /* Ctrl+C */
bindings = <&kp C_MUTE>;    /* 静音 */
bindings = <&mkp LCLK>;     /* 鼠标左键 */
bindings = <&mkp RCLK>;     /* 鼠标右键 */
bindings = <&mkp MCLK>;     /* 鼠标中键 */
bindings = <&mkp MB4>;      /* 后退 */
bindings = <&mkp MB5>;      /* 前进 */
bindings = <&mo 1>;         /* 按住时进入 layer 1 */
bindings = <&tog 1>;        /* 切换 layer 1 */
bindings = <&mt LCTRL ESC>; /* 按住为 Ctrl，点击为 Esc */
bindings = <&lt 1 SPACE>;   /* 按住进入 layer 1，点击为空格 */
bindings = <&my_macro>;     /* 用户定义的 macro */
```

`codes` 必须与 Trackball 节点的 `button-code` 一致。例如把 `button-code` 改为
`INPUT_BTN_1` 后，processor 也必须使用 `codes = <INPUT_BTN_1>`。

如果 listener 同时使用方向 transformer，按顺序放在同一个列表中：

```dts
&trackball_listener {
    input-processors = <
        &zip_xy_transform INPUT_TRANSFORM_Y_INVERT
        &trackball_button_behavior
    >;
};
```

## 10. 四颗 PWM LED

输入驱动只处理运动和 BTN，不直接控制 LED。四颗灯使用标准 `pwm-leds`：

```dts
#include <zephyr/dt-bindings/pwm/pwm.h>

/ {
    trackball_leds: trackball_leds {
        compatible = "pwm-leds";

        wht {
            pwms = <&pwm0 0 PWM_MSEC(1) PWM_POLARITY_NORMAL>;
        };

        grn {
            pwms = <&pwm0 1 PWM_MSEC(1) PWM_POLARITY_NORMAL>;
        };

        red {
            pwms = <&pwm0 2 PWM_MSEC(1) PWM_POLARITY_NORMAL>;
        };

        blu {
            pwms = <&pwm0 3 PWM_MSEC(1) PWM_POLARITY_NORMAL>;
        };
    };
};
```

然后将四颗标量 LED 交给 LED Matrix，并建立 independent group：

```dts
/ {
    trackball_led_matrix: trackball_led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "matrix";
        led-device = <&trackball_leds>;
        led-count = <4>;
        led-indexes = <0 1 2 3>;

        trackball_group: independent0 {
            independent-id = <0>;
            independent-leds = <0 1 2 3>;

            profile0 {
                profile-id = <0>;
                renderer = "solid";
                colors = <0xFFFFFF 0x000000 0x000000 0x000000>;
                brightness = <25>;
                period-ms = <1000>;
            };

            profile1 {
                profile-id = <1>;
                renderer = "solid";
                colors = <0x000000 0x000000 0x000000 0x000000>;
                brightness = <100>;
                period-ms = <1000>;
            };
        };
    };
};
```

示例中 `colors` 的四项依次对应 WHT、GRN、RED、BLU。对于标量 PWM LED，非零 RGB 值会
转换为该物理 LED 的亮度；profile 的 `brightness` 再作为该组合的亮度系数。

可在 keymap 中切换 profile：

```dts
#include <dt-bindings/zmk/led_matrix.h>

&lm LM_CFS(0, 0) /* 选择 group 0 的 profile 0 */
&lm LM_CFF 0     /* 循环 group 0 的 profiles */
```

`LM_BRT` 调整整个 LED Matrix 的全局亮度；需要同一灯光组合使用多个固定亮度时，应建立多个
local profile。

## 11. `n52test` 实例

### 11.1 GPIO

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

矩阵为 4×5，二极管方向为 `row2col`。WHT 使用 P0.30，避免与矩阵 row P0.20 冲突。

### 11.2 Trackball 配置

```dts
trackball: trackball {
    compatible = "sparkfun,blackberry-trackballer";
    left-gpios = <&gpio0 26 GPIO_ACTIVE_HIGH>;
    right-gpios = <&gpio0 25 GPIO_ACTIVE_HIGH>;
    up-gpios = <&gpio0 28 GPIO_ACTIVE_HIGH>;
    down-gpios = <&gpio0 27 GPIO_ACTIVE_HIGH>;
    button-gpios = <&gpio1 6 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    zephyr,axis-x = <INPUT_REL_X>;
    zephyr,axis-y = <INPUT_REL_Y>;
    button-code = <INPUT_BTN_0>;
    report-period-ms = <4>;
    interpolation-frames = <4>;
    debounce-interval-ms = <5>;
    acceleration-start-hz = <5>;
    acceleration-max-hz = <30>;
    acceleration-exponent = <2>;
    minimum-step = <1>;
    maximum-step = <8>;
};

trackball_speed: trackball_speed {
    compatible = "zmk,input-processor-speed-profile";
    #input-processor-cells = <0>;
    type = <INPUT_EV_REL>;
    codes = <INPUT_REL_X INPUT_REL_Y>;
    speed-percentages = <150 300 450 600 900>;
    default-speed-level = <2>;
    persist-speed-level;
    track-remainders;
};

trackball_listener: trackball_listener {
    compatible = "zmk,input-listener";
    device = <&trackball>;
    input-processors = <&trackball_speed>;
};
```

`n52test` 未设置 `minimum-pulse-interval-us`，因此使用默认值 `0`，不会过滤方向脉冲。
根据实测约 5–23 Hz 的方向脉冲，默认二次曲线在约 10、20、23 Hz 时分别产生约
1.28、3.52、4.63 步，达到 30 Hz 时使用最大步长 8。之后由 `trackball_speed` 选择
150%、300%、450%、600% 或 900% 倍率，默认第 2 档 300%；高速最大移动量约为
12、24、36、48、72。档位使用 settings 持久化。

`n52test` 使用 4 ms 报告周期和四帧插值。默认 300% 档下，一个高速 8-step 脉冲先由驱动
拆成四个 2，再由速度 processor 变成四个 6；900% 档则变成四个 18。两种情况下总移动量
仍分别为 24 和 72，插值只改变发送时序，不伪造方向脉冲，也不增加总距离。没有新脉冲时，
最长尾窗为 `4 ms × 4 = 16 ms`。

当前 BTN 默认为鼠标左键。P1.09 独立测试键在三个 keymap layer 中都绑定：

```dts
&lm LM_CFF 0
```

它依次循环以下 profile：

1. 四灯 25%。
2. WHT 25%。
3. GRN 25%。
4. RED 25%。
5. BLU 25%。
6. 全关。

### 11.3 默认 keymap

`n52test.keymap` 参照 `numpadv2`，包含 Base、Media、Fn 三层。Base 层为：

```text
NumLock   Delete    Backspace   Divide
7         8         9           Multiply
4         5         6           Minus
1         2         3           Plus
Fn        0         Dot         Enter
```

Fn 层左上两个键分别绑定：

```dts
&tbspd INPUT_SPEED_DOWN
&tbspd INPUT_SPEED_UP
```

其余按键提供 BLE profile、清除配对和 LED Matrix 控制，右下角用于切换 Media 层。需要
改变档位数量或倍率时，修改 `speed-percentages`；需要固定选择某档时，也可以在任意 keymap
位置使用 `&tbspd INPUT_SPEED_SET(level)`。

### 11.4 电源与 UF2 分区

`n52test` 启用：

- `reg0`：VDDH 高压电源模式。
- `reg1`：主电源 DC/DC 模式。
- 外部 32.768 kHz 晶振。
- USB 和 BLE。
- 已预刷 BMK/Adafruit UF2 bootloader 的 Flash 布局。

| 分区           | 地址范围            |       大小 |
| -------------- | ------------------- | ---------: |
| MBR            | `0x000000–0x000FFF` | `0x001000` |
| ZMK code       | `0x001000–0x0D3FFF` | `0x0D3000` |
| Settings/NVS   | `0x0D4000–0x0F3FFF` | `0x020000` |
| UF2 bootloader | `0x0F4000–0x0FFFFF` | `0x00C000` |

PCB 必须按照 Nordic nRF52840 参考电路配置 VDDH、REG0/REG1 所需电感和去耦。不要用 ZMK
固件覆盖 `0xF4000–0xFFFFF` 的 bootloader 区域。

### 11.5 构建和刷写

在 ZMK 根目录运行：

```sh
west build -s app -p -b n52test/nrf52840/zmk
```

输出文件：

```text
build/zephyr/zmk.uf2
```

双击复位进入 UF2 bootloader 后，将 `zmk.uf2` 复制到出现的 UF2 磁盘。

## 12. 测试与硬件验收

在支持 Zephyr `native_sim` 的 Linux 环境运行全部 Trackball 测试：

```sh
ZMK_SRC_DIR=app ./app/run-test.sh app/tests/pointing/blackberry-trackballer
```

当前 Zephyr POSIX `native_sim` 不支持直接在 macOS 上运行，macOS 开发环境应使用 Linux
容器或 CI 执行 simulator 测试。

硬件验收建议：

1. 慢速滚动，确认可以逐步精确移动。
2. 持续快速滚动，确认移动速度明显提升并达到上限。
3. 使用 Fn+减档/加档遍历五档，确认边界不循环，重启后恢复最后档位。
4. 分别检查左、右、上、下符号。
5. 检查对角滚动和快速反向，不应出现明显卡顿或溢出。
6. 按下和释放 BTN，确认没有重复点击或卡住。
7. 进入休眠再唤醒，确认没有旧移动、虚假点击或卡键。
8. 循环六个 LED profile，确认四灯顺序、组合和 PWM 亮度稳定。
9. 分别检查 USB、BLE 和 UF2 双击复位。

## 13. 常见问题

### 完全没有移动

- 检查 breakout 是否使用 3.3 V 并与 MCU 共地。
- 检查 LFT/RHT/UP/DWN 是否连接到 DTS 中对应 GPIO。
- 检查方向 GPIO 是否为 `GPIO_ACTIVE_HIGH`。
- 检查 `trackball_listener` 的 `device` 是否指向正确的 Trackball 节点。
- 检查构建配置中是否出现 `CONFIG_INPUT_SPARKFUN_BLACKBERRY_TRACKBALLER=y`。

### 方向相反或 XY 交换

使用 `zip_xy_transform` 的 `INPUT_TRANSFORM_X_INVERT`、`INPUT_TRANSFORM_Y_INVERT` 或
`INPUT_TRANSFORM_XY_SWAP`，不要修改驱动的方向符号。

### 指针始终很慢

- 降低 `acceleration-max-hz`，使最大步长更容易达到。
- 提高 `maximum-step`。
- 使用 Fn+加档或提高 `speed-percentages` 中的对应倍率。
- 检查 Trackball 方向输出是否有脉冲丢失或错误的按键式消抖。

### 指针太快或难以精确定位

- 保持 `minimum-step = <1>`。
- 提高 `acceleration-start-hz`，让加速更晚开始。
- 使用 Fn+减档或降低 `speed-percentages` 中的对应倍率。

### 静止时指针抖动

- 设置 `minimum-pulse-interval-us = <4000>`，过滤同方向 4 ms 内的重复边沿。
- 检查 breakout 是否稳定使用 3.3 V、是否与 MCU 共地，以及方向线是否受到 PWM 串扰。
- 适当放松 Trackball 安装板螺丝；SparkFun 说明螺丝过紧可能导致 Hall 传感器异常。
- 若跳变间隔超过过滤值，软件无法将它与真实滚动区分，应使用逻辑分析仪检查四路方向信号。

### BTN 没有响应

- 检查 BTN 是否为 `GPIO_ACTIVE_LOW | GPIO_PULL_UP`。
- 检查 `button-code` 与 behavior processor 的 `codes` 是否相同。
- 如果机械抖动明显，提高 `debounce-interval-ms`。
- 确认 processor 已加入正确的 `trackball_listener`。

### 四颗 LED 不亮

- 确认 `CONFIG_PWM=y` 和 `CONFIG_ZMK_LED_MATRIX=y`。
- 检查 PWM controller、channel 和 pinctrl。
- 检查 WHT/GRN/RED/BLU 的顺序与 `led-indexes`、`colors` 一致。
- 检查 LED Matrix 是否开启、全局亮度是否为 0、当前 profile 是否为全关。
- 如果硬件为低有效 LED，调整 PWM polarity，不能继续使用高有效示例。

### 唤醒后出现移动或卡键

- 确认启用了 Zephyr device PM。
- 检查方向与 BTN GPIO 是否被其他外设复用。
- 开启日志，检查 suspend/resume 时是否有 GPIO 或 input report 错误。

## 14. 延伸阅读

- [BlackBerry Trackball 通用配置与使用指南](docs/docs/hardware-integration/blackberry-trackball.mdx)
- [ZMK Pointing 硬件集成](docs/docs/hardware-integration/pointing.mdx)
- [Trackball 输入驱动](app/module/drivers/input/input_blackberry_trackballer.c)
- [Trackball Devicetree binding](app/module/dts/bindings/input/sparkfun,blackberry-trackballer.yaml)
- [`n52test` 板级说明](app/boards/bmk/n52test/README.md)
- [`n52test` Devicetree](app/boards/bmk/n52test/n52test_nrf52840_zmk.dts)
- [SparkFun BlackBerry Trackballer Breakout](https://github.com/sparkfun/Blackberry_Trackballer_Breakout)
