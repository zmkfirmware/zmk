# ZMK LED Matrix 功能说明

## 目录

1. [概述](#1-概述)
2. [架构概览](#2-架构概览)
3. [后端驱动类型](#3-后端驱动类型)
4. [显示配置文件（Profile）](#4-显示配置文件profile)
5. [渲染器（8 种）](#5-渲染器8-种)
6. [图层颜色（Layer Color）](#6-图层颜色layer-color)
7. [独立 LED 组（Independent）](#7-独立-led-组independent)
8. [状态叠加层（Status Overlay）](#8-状态叠加层status-overlay)
9. [颜色系统](#9-颜色系统)
10. [空间布局（matrix-led-grid）](#10-空间布局matrix-led-grid)
11. [行为命令（LM_*）](#11-行为命令lm_)
12. [电源管理](#12-电源管理)
13. [设置持久化](#13-设置持久化)
14. [Kconfig 配置项](#14-kconfig-配置项)
15. [Devicetree 属性](#15-devicetree-属性)
16. [运行时 API](#16-运行时-api)
17. [事件系统](#17-事件系统)
18. [键映射使用示例](#18-键映射使用示例)
19. [常见错误](#19-常见错误)

---

## 1. 概述

LED Matrix 是 ZMK 固件中的**逻辑 RGB 渲染和控制层**。它位于用户输入/状态与底层 LED 硬件之间，提供：

- 统一的逻辑 LED 数量和布局管理
- 多来源渲染仲裁（配置文件、图层颜色、独立 LED 组、状态叠加）
- 将逻辑 RGB 输出映射到具体后端设备
- 运行时行为控制：亮度、速度、手动色相、状态超时、自动关闭、电源管理、设置持久化

**与其他子系统的区别：**
- **RGB Underglow**：条带/像素级控制
- **Backlight**：单色全局亮度
- **LED Matrix**：按键级 RGB，支持状态叠加、配置文件切换、独立子配置文件、行为命令

---

## 2. 架构概览

### 源码文件布局

| 文件 | 职责 |
|------|------|
| `app/src/led_matrix.c` | 入口，通过 `#include "led_matrix/*.c"` 包含子模块 |
| `app/src/led_matrix/core.c` | 静态配置展开、配置文件查找、独立配置文件选择、图层颜色 LED 映射 |
| `app/src/led_matrix/render.c` | 渲染优先级仲裁、配置文件渲染器执行、后端输出分发、tick/work 调度 |
| `app/src/led_matrix/status.c` | 电池/USB/BLE/HID 状态快照、自动状态显示、瞬态/持久状态、超时行为 |
| `app/src/led_matrix/power.c` | 运行时模式判定、空闲自动关闭、睡眠/唤醒、后端托管电源、外部电源协调 |
| `app/src/led_matrix/api.c` | 公共运行时 API 实现、设置加载/保存、SYS_INIT |
| `app/src/behaviors/behavior_led_matrix.c` | `&lm` 行为驱动：将 LM_* 命令转为运行时 API 调用 |
| `app/src/events/led_matrix_state_changed.c` | 状态变更事件实现 |
| `app/include/zmk/led_matrix.h` | 公共 API 头文件：状态结构体、渲染源枚举、API 声明 |
| `app/include/dt-bindings/zmk/led_matrix.h` | LM_* 命令宏、状态掩码宏、网格 token |
| `app/include/dt-bindings/zmk/led_matrix_colors.h` | 颜色常量和语义别名 |
| `app/dts/behaviors/led_matrix.dtsi` | 行为 DTS 节点定义 |

### 渲染源优先级（从低到高）

| 优先级 | 渲染源 | 说明 |
|--------|--------|------|
| 0 | `OFF` | 无有效渲染输出 |
| 1 | `PROFILE` | 当前活动显示配置文件 |
| 2 | `INDEPENDENT` | 独立 LED 组活动 |
| 3 | `LAYER_COLOR` | 图层颜色叠加 |
| 4 | `STATUS_OVERLAY` | 状态叠加层（最高优先级） |

---

## 3. 后端驱动类型

### 3.1 `led-driver = "matrix"`（Zephyr `led` API）

使用 `led_write_channels`、`set_color`、`set_brightness`、`on`、`off` 方法。
典型后端：IS31 系列 LED 矩阵芯片、`zmk,led-mock` 测试设备。

### 3.2 `led-driver = "strip"`（Zephyr `led_strip` API）

使用 `led_strip_update_rgb` 方法。
典型用途：直接 LED 条带输出（如 WS2812、SK6812）。
支持 `CONFIG_ZMK_LED_MATRIX_EXT_POWER` 运行时外部电源控制。

### 3.3 隐藏自动派生 Kconfig

- `CONFIG_ZMK_LED_MATRIX_USES_LED_API` — 当 `led-driver = "matrix"` 时自动设置
- `CONFIG_ZMK_LED_MATRIX_USES_LED_STRIP_API` — 当 `led-driver = "strip"` 时自动设置
- `CONFIG_ZMK_LED_MATRIX` 然后 `select LED` 或 `select LED_STRIP`

### 3.4 `led_composite` 后端驱动

`led_composite` 是一个**虚拟 LED 控制器**，将多个异构 LED 后端（矩阵控制器 + LED 条带）合并为单一扁平化 RGB 通道地址空间。

**子设备类型：**
- **Matrix 子设备**（`led-device`）：Zephyr `led` API 设备，使用 `write_channels`
- **Strip 子设备**（`led-strip-device`）：Zephyr `led_strip` API 设备

**子设备属性：**

| 属性 | 必需 | 说明 |
|------|------|------|
| `led-device` | 二选一 | Matrix 样式 LED 设备 |
| `led-strip-device` | 二选一 | LED 条带设备 |
| `channel-offset` | 是 | 扁平通道空间中的起始偏移 |
| `channel-count` | 是 | RGB 通道数（条带子设备必须为 3 的倍数） |
| `ext-power-device` | 否 | 共享外部电源控制器（仅条带子设备） |

**能力标志：**

| 标志 | 含义 |
|------|------|
| `ZMK_LED_COMPOSITE_CAP_MANAGED_POWER` | 至少一个子设备支持软件/硬件电源控制 |
| `ZMK_LED_COMPOSITE_CAP_FULL_HARD_OFF` | 所有子设备均可完全硬件关断 |

**写入路径：**
- Matrix 子设备：直接调用 `led_write_channels()`
- Strip 子设备：写入内部缓冲区，优先使用 `led_strip_update_channels()`，不支持时回退到 `led_strip_update_rgb()`

**支持的矩阵硬件：**

| 枚举值 | 硬件 | 电源管理 |
|--------|------|----------|
| `GENERIC` | 通用 Zephyr LED 设备 | 无托管电源 |
| `IS31FL3733` | ISSI IS31FL3733 | 完整：寄存器 blank + SDB GPIO 硬关断 + 恢复重编程 |
| `IS31FL3737` | ZMK 定制 IS31FL3737 | 完整：同上，I2C PWM/ABM 控制 |
| `MOCK_IS31FL3733` | 测试模拟 IS31FL3733 | 模拟完整电源行为 |
| `MOCK_IS31FL3737` | 测试模拟 IS31FL3737 | 模拟完整电源行为 |

### 3.5 LED 输出 Sink 类型（6 种）

| Sink 类型 | 说明 |
|-----------|------|
| `CHANNELS` | 直接 `led_write_channels()`（IS31FL3733 等） |
| `RGB_SET_COLOR` | 每 LED `led_set_color()` + RGB slot 映射 |
| `SCALAR_LED` | 每 LED `led_set_brightness()` 或开/关 |
| `SCALAR_TRIPLET` | 每通道标量写入，使用 `led-indexes` |
| `BINARY_LED` | 每 LED 二进制开/关 |
| `BINARY_TRIPLET` | 每通道二进制开/关，使用 `led-indexes` |

---

## 4. 显示配置文件（Profile）

用户可选择的基础光照配置文件。每个配置文件子节点具有：

| 属性 | 必需 | 默认值 | 说明 |
|------|------|--------|------|
| `profile-id` | 是 | — | 0 起始唯一 ID |
| `display-name` | 否 | — | 人类可读名称 |
| `renderer` | 否 | `"solid"` | 渲染算法 |
| `colors` | 否 | — | `0xRRGGBB` 或 `LM_*` 动态颜色数组；长度 = 1（广播）或 `led-count` |
| `brightness` | 否 | 100 | 亮度标量 0-100 |
| `period-ms` | 否 | 1000 | 动画周期 |
| `keypress-afterglow-ms` | 否 | 0 | 按键释放后保持时间（仅 keypress 渲染器） |
| `keypress-fadeout-ms` | 否 | 0 | afterglow 后淡出时间（仅 keypress 渲染器） |
| `keypress-range` | 否 | 0 | 辉光半径，网格单元为单位（仅 keypress 渲染器；0 = 仅按下的键） |

---

## 5. 渲染器（8 种）

| 渲染器 | 需要网格 | 说明 |
|--------|----------|------|
| `solid` | 否 | 静态颜色显示 |
| `breathe` | 否 | 脉动亮度动画，三角波 + 方形缓动 |
| `spectrum` | 否 | 色彩光谱循环，所有 LED 同步变化 |
| `swirl` | 否 | 旋转/漩涡颜色动画，每 LED 色相偏移 + 全局旋转 |
| `keypress` | 是 | 按键位置照明，支持辉光半径、afterglow、淡出 |
| `ripple` | 是 | 从按键位置扩展的环形动画（曼哈顿距离） |
| `rain` | 是 | 按键触发的随机列状下落雨滴动画 |
| `starlight` | 是 | 闪烁星点 + 周期性流星条纹动画 |

**位置驱动渲染器**（keypress、ripple、rain）需要 `matrix-led-grid` 提供空间布局信息。

**独立 LED 组支持的渲染器子集：** solid、breathe、spectrum、swirl（不支持 keypress/ripple/rain/starlight）。

---

## 6. 图层颜色（Layer Color）

当特定图层激活时覆盖 LED 颜色。

| 属性 | 必需 | 默认值 | 说明 |
|------|------|--------|------|
| `layer-id` | 是 | — | 键映射图层 ID |
| `layer-colors` | 否 | — | 颜色数组；长度 = 1（广播）或目标 LED 数 |
| `layer-leds` | 否 | — | 显式目标 LED 索引；省略时使用 `matrix-led-grid` 遍历顺序 |
| `brightness` | 否 | 100 | 亮度标量 0-100 |
| `display-time-ms` | 否 | 1000 | 显示持续时间；0 = 图层保持活动期间持久显示 |

---

## 7. 独立 LED 组（Independent）

始终点亮的 LED 组，独立于主配置文件运行。

### 7.1 容器属性

| 属性 | 必需 | 说明 |
|------|------|------|
| `independent-id` | 是 | 0 起始唯一 ID |
| `independent-leds` | 是 | 此容器控制的逻辑 LED 索引数组 |

### 7.2 独立本地配置文件（嵌套在容器下）

| 属性 | 必需 | 默认值 | 说明 |
|------|------|--------|------|
| `profile-id` | 是 | — | 此容器内的 0 起始本地配置文件 ID |
| `renderer` | 否 | `"solid"` | 渲染算法 |
| `colors` | 否 | — | 颜色数组；长度 = 1 或 `independent-leds` 数量 |
| `brightness` | 否 | 100 | 亮度标量 0-100 |
| `period-ms` | 否 | 1000 | 动画周期 |

**约束：**
- `independent-leds` 不能有重复
- 同一 LED 不能出现在多个独立容器中
- `independent-leds` 不能与 `matrix-led-grid` 条目重叠
- ID 和本地配置文件 ID 必须连续且唯一

---

## 8. 状态叠加层（Status Overlay）

### 8.1 四种状态源

| 状态源 | 属性前缀 | 状态 |
|--------|----------|------|
| **Battery** | `battery-leds` | 开/关、基于电量等级的颜色（低/中/高） |
| **USB** | `usb-leds` | HID 活跃、已供电、未连接 |
| **BLE** | `ble-profile-leds` | 开放/配对中、已连接、已断开 |
| **HID** | `hid-*-leds` | Caps Lock/Num Lock/Scroll Lock/Compose/Kana 各自开/关 |

### 8.2 状态源优先级（固定覆盖，无混合）

```
Battery < USB < BLE < HID
```

### 8.3 HID 内部优先级

```
Caps < Num < Scroll < Compose < Kana
```

### 8.4 状态位掩码宏

| 宏 | 值 | 说明 |
|----|----|------|
| `LM_STA_BATTERY` | 0x01 | 电池状态源 |
| `LM_STA_USB` | 0x02 | USB 状态源 |
| `LM_STA_BLE` | 0x04 | BLE 状态源 |
| `LM_STA_HID` | 0x08 | HID 状态源 |
| `LM_STA_ALL` | 0x0F | 所有状态源 |

### 8.5 超时行为

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `status-timeout-ms` | 3000 | 所有状态源的默认超时 |
| `usb-timeout-ms` | 继承 `status-timeout-ms` | USB 专用超时 |
| `ble-timeout-ms` | 继承 `status-timeout-ms` | BLE 专用超时 |
| `hid-timeout-ms` | 继承 `status-timeout-ms` | HID 专用超时 |
| `battery-timeout-ms` | 继承 `status-timeout-ms` | 电池专用超时 |
| `ble-pairing-hold` | 1 | 配置文件开放期间保持 BLE 可见 |

- **超时 > 0**：瞬态显示（显示 N ms 后清除）
- **超时 = 0**：持久模式（状态在运行时输出允许期间保持可见）

### 8.6 电池等级颜色

`battery-level-colors` 定义 N 个桶颜色。0-100% 电量被分成 N 个均匀大小的桶。当存在时覆盖 `battery-color-on` 的电池 LED 点亮颜色。

---

## 9. 颜色系统

### 9.1 自定义颜色构造

- `LM_RGB(r, g, b)` — 构建 `0xRRGGBB` 值

### 9.2 动态颜色（非静态）

| 宏 | 值 | 说明 |
|----|----|------|
| `LM_RAINBOW` | `0x01000000` | 彩虹循环颜色 |
| `LM_PALETTE` | `0x01000002` | 基于手动色相索引的调色板颜色 |

### 9.3 预定义基础颜色（12 种）

`LM_BLACK`、`LM_WHITE`、`LM_RED`、`LM_GREEN`、`LM_BLUE`、`LM_YELLOW`、`LM_CYAN`、`LM_MAGENTA`、`LM_ORANGE`、`LM_AMBER`、`LM_PURPLE`、`LM_PINK`

### 9.4 灰度（5 种）

`LM_GRAY_08`、`LM_GRAY_10`、`LM_GRAY_20`、`LM_GRAY_40`、`LM_GRAY_80`

### 9.5 暗色（7 种）

`LM_DIM_WHITE`、`LM_DIM_RED`、`LM_DIM_GREEN`、`LM_DIM_BLUE`、`LM_DIM_YELLOW`、`LM_DIM_CYAN`、`LM_DIM_AMBER`

### 9.6 语义颜色别名

| 类别 | 别名 | 映射到 |
|------|------|--------|
| 基础层 | `LM_BASE_OFF` | `LM_BLACK` |
| 基础层 | `LM_BASE_IDLE` | `LM_DIM_WHITE` |
| 基础层 | `LM_BASE_SOFT` | `LM_GRAY_20` |
| USB | `LM_USB_HID` | `LM_GREEN` |
| USB | `LM_USB_POWERED` | `LM_DIM_WHITE` |
| USB | `LM_USB_NONE` | `LM_BLACK` |
| BLE | `LM_BLE_OPEN` | `LM_BLUE` |
| BLE | `LM_BLE_CONNECTED` | `LM_CYAN` |
| BLE | `LM_BLE_DISCONNECTED` | `LM_RED` |
| 电池 | `LM_BATTERY_ON` | `LM_GREEN` |
| 电池 | `LM_BATTERY_OFF` | `LM_BLACK` |
| 电池 | `LM_BATTERY_LOW` | `LM_RED` |
| 电池 | `LM_BATTERY_MID` | `LM_YELLOW` |
| 电池 | `LM_BATTERY_HIGH` | `LM_GREEN` |
| HID | `LM_HID_ON` | `LM_WHITE` |
| HID | `LM_HID_OFF` | `LM_BLACK` |
| HID | `LM_HID_CAPS_ON` | `LM_WHITE` |
| HID | `LM_HID_NUM_ON` | `LM_WHITE` |
| HID | `LM_HID_SCROLL_ON` | `LM_WHITE` |

---

## 10. 空间布局（matrix-led-grid）

定义本地 RC（行/列）空间网格的 token 流。

### 10.1 Token 类型

| Token | 值 | 说明 |
|-------|----|------|
| 数字 | 0..N-1 | 此网格单元的逻辑 LED 索引 |
| `LM_NOLED` | 0xFFFF | 键存在但无 LED |
| `LM_PH` | 0xFFFD | 占位单元，无键位置 |
| `LM_BREAK` | 0xFFFE | 行分隔符 |

### 10.2 要求

- 行数必须匹配 kscan 行数
- 每行宽度必须匹配 kscan 列数
- 位置驱动渲染器（keypress、ripple、rain）必需
- 省略 `layer-leds` 的图层颜色条目必需
- 纯状态 LED 或仅独立配置可选
- 分体键盘：每半部分仅写入本地网格

---

## 11. 行为命令（LM_*）

### 11.1 电源和亮度

| 命令 | ID | 参数 | 说明 |
|------|----|----|------|
| `LM_ON` | 0 | 0 | 开启 LED Matrix |
| `LM_OFF` | 1 | 0 | 关闭 LED Matrix |
| `LM_TOG` | 2 | 0 | 切换 LED Matrix 开/关 |
| `LM_BRI` | 3 | 0 | 亮度增加 `BRT_STEP` |
| `LM_BRD` | 4 | 0 | 亮度减少 `BRT_STEP` |
| `LM_BRT` | 8 | value | 直接设置亮度为指定值（0-100） |

### 11.2 显示配置文件

| 命令 | ID | 参数 | 说明 |
|------|----|----|------|
| `LM_EFF` | 5 | 0 | 下一个显示配置文件 |
| `LM_EFR` | 6 | 0 | 上一个显示配置文件 |
| `LM_EFS` | 7 | profile_id | 直接设置显示配置文件 |

### 11.3 速度

| 命令 | ID | 参数 | 说明 |
|------|----|----|------|
| `LM_SPI` | 10 | 0 | 速度增加一级 |
| `LM_SPD` | 11 | 0 | 速度减少一级 |
| `LM_SPS` | 12 | value | 直接设置速度（1-N） |

### 11.4 手动色相

| 命令 | ID | 参数 | 说明 |
|------|----|----|------|
| `LM_HUI` | 13 | 0 | 色相索引增加一级 |
| `LM_HUD` | 14 | 0 | 色相索引减少一级 |
| `LM_HUS` | 15 | value | 直接设置色相索引（0-11） |

### 11.5 状态显示

| 命令 | ID | 参数 | 说明 |
|------|----|----|------|
| `LM_STA` | 9 | mask | 手动触发状态显示（状态源位掩码） |

### 11.6 独立子配置文件

| 命令 | ID | 参数 | 说明 |
|------|----|----|------|
| `LM_CFF` | 16 | 0 | 循环独立子配置文件（前进） |
| `LM_CFR` | 17 | 0 | 循环独立子配置文件（后退） |
| `LM_CFS` | 18 | packed | 设置特定独立子的本地配置文件 |

**打包格式：** `LM_CHILD_PROFILE_PACK(child_id, profile_id)` = `(((child_id & 0xFF) << 8) | (profile_id & 0xFF))`

---

## 12. 电源管理

### 12.1 运行时模式（状态机）

| 模式 | 值 | 基础渲染 | 说明 |
|------|----|----------|------|
| `OFF` | 0 | 否 | LED Matrix 关闭 |
| `ACTIVE` | 1 | 是 | 正常运行，所有渲染允许 |
| `FORCE_SHOW_ONLY` | 2 | 否 | 仅状态显示允许，基础渲染被抑制 |
| `AUTO_SUSPENDED` | 3 | 否 | 空闲自动关闭已挂起渲染 |
| `AUTO_SUSPENDED_FORCE_SHOW` | 4 | 否 | 自动挂起但强制状态显示活动 |
| `SLEEP_SUSPENDED` | 5 | 否 | 系统进入深度睡眠 |

### 12.2 空闲自动关闭

需要 `CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE`：
- 电池供电时，系统进入空闲后 LED Matrix 自动挂起
- 活动或状态事件恢复渲染
- USB 供电时阻止自动关闭

### 12.3 外部电源

需要 `CONFIG_ZMK_LED_MATRIX_EXT_POWER` + `CONFIG_ZMK_EXT_POWER` + `DT_HAS_ZMK_EXT_POWER_GENERIC_ENABLED` + 条带后端：
- 渲染活动期间（包括强制瞬态状态）启用外部电源
- 无渲染时禁用外部电源

### 12.4 后端托管电源

- LED Matrix 查询 `led_composite` 能力（`CAP_MANAGED_POWER`、`CAP_FULL_HARD_OFF`）
- 基于能力决定仅清空输出还是硬关断
- IS31 矩阵：软 blank + 硬关断（通过 SDB GPIO）+ 恢复重编程
- 条带设备：共享外部电源门控

### 12.5 睡眠挂起

当系统进入深度睡眠（`ZMK_SLEEP`）时，LED Matrix 进入 `SLEEP_SUSPENDED` 模式：
- 同步挂起所有输出
- 取消所有工作项
- 清空逻辑 LED
- 硬关断后端

---

## 13. 设置持久化

通过 ZMK 设置子系统跨重启持久化。当前版本：**v5**。

### 13.1 版本历史

| 版本 | 持久化字段 |
|------|-----------|
| v1 | `on`、`brightness`、`selected_profile_id`（3 字节） |
| v2 | + `speed`（4 字节） |
| v3 | + `manual_hue_index`（5 字节） |
| v4 | + `selected_independent_profile_ids[LOGICAL_LED_COUNT]`（5 + N 字节） |
| v5 | + `selected_independent_profile_ids[INDEPENDENT_PROFILE_SLOT_COUNT]`（5 + slot_count 字节） |

所有版本均在加载时自动迁移到最新格式。

### 13.2 注意事项

- `*_START` Kconfig 值仅是存储尚未建立或已重置时的初始默认值
- 不会覆盖每次启动时已保存的用户偏好

---

## 14. Kconfig 配置项

### 14.1 主开关

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `CONFIG_ZMK_LED_MATRIX` | bool | n | 启用 LED Matrix 子系统。自动选择 `LED` 或 `LED_STRIP`、`ZMK_HID_INDICATORS`、`ZMK_LOW_PRIORITY_WORK_QUEUE` |

### 14.2 运行时选项

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `CONFIG_ZMK_LED_MATRIX_EXT_POWER` | bool | n | 条带后端的运行时外部电源控制 |
| `CONFIG_ZMK_LED_MATRIX_AUTO_OFF_IDLE` | bool | n | 空闲时电池供电自动关闭 |
| `CONFIG_ZMK_LED_MATRIX_BRT_STEP` | int | 10 | 亮度步进百分比（1-100） |
| `CONFIG_ZMK_LED_MATRIX_BRT_START` | int | 60 | 初始亮度百分比（1-100） |
| `CONFIG_ZMK_LED_MATRIX_PROFILE_START` | int | 0 | 初始显示配置文件 ID（0-255） |
| `CONFIG_ZMK_LED_MATRIX_SPD_LEVELS` | int | 7 | 总速度级别数（1-255） |
| `CONFIG_ZMK_LED_MATRIX_SPD_START` | int | 4（或 SPD_LEVELS） | 初始速度级别（1-SPD_LEVELS） |
| `CONFIG_ZMK_LED_MATRIX_ON_START` | bool | y | 启动时默认开/关状态 |

### 14.3 行为

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `CONFIG_ZMK_BEHAVIOR_LED_MATRIX` | bool | y（自动） | 启用 `&lm` 行为 |

### 14.4 LED Composite 驱动

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `CONFIG_ZMK_LED_COMPOSITE` | bool | y | 启用 `zmk,led-composite` 驱动 |

---

## 15. Devicetree 属性

### 15.1 顶层 `zmk,led-matrix` 属性

| 属性 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `led-driver` | string enum | 是 | `"matrix"` 或 `"strip"` |
| `led-device` | phandle | 条件 | `led-driver = "matrix"` 时必需 |
| `led-strip-device` | phandle | 条件 | `led-driver = "strip"` 时必需 |
| `led-count` | int | 是 | 逻辑 LED 数量 |
| `output-channel-count` | int | 否 | 物理输出通道数（默认 `led-count * 3`） |
| `channel-indexes` | array | 否 | 逻辑到物理通道映射（长度 = `led-count * 3`） |
| `led-indexes` | array | 否 | 逻辑到物理 LED 映射 |
| `matrix-led-grid` | array | 否 | 空间布局 token 流 |

### 15.2 状态 LED 属性

| 属性 | 说明 |
|------|------|
| `battery-leds` | 电池状态 LED 索引 |
| `battery-color-on` | 电池点亮颜色 |
| `battery-color-off` | 电池熄灭颜色 |
| `battery-level-colors` | 电池电量等级颜色 |
| `ble-profile-leds` | BLE 状态 LED 索引 |
| `ble-color-open` | BLE 开放/配对颜色 |
| `ble-color-connected` | BLE 已连接颜色 |
| `ble-color-disconnected` | BLE 已断开颜色 |
| `usb-leds` | USB 状态 LED 索引 |
| `usb-color-hid` | USB HID 颜色 |
| `usb-color-powered` | USB 已供电颜色 |
| `usb-color-none` | USB 未连接颜色 |
| `hid-caps-leds` | Caps Lock LED 索引 |
| `hid-caps-color-on/off` | Caps Lock 开/关颜色 |
| `hid-num-leds` | Num Lock LED 索引 |
| `hid-num-color-on/off` | Num Lock 开/关颜色 |
| `hid-scroll-leds` | Scroll Lock LED 索引 |
| `hid-scroll-color-on/off` | Scroll Lock 开/关颜色 |
| `hid-compose-leds` | Compose LED 索引 |
| `hid-compose-color-on/off` | Compose 开/关颜色 |
| `hid-kana-leds` | Kana LED 索引 |
| `hid-kana-color-on/off` | Kana 开/关颜色 |

### 15.3 超时属性

| 属性 | 默认值 | 说明 |
|------|--------|------|
| `status-timeout-ms` | 3000 | 默认状态显示超时 |
| `usb-timeout-ms` | 继承 `status-timeout-ms` | USB 专用超时 |
| `ble-timeout-ms` | 继承 `status-timeout-ms` | BLE 专用超时 |
| `hid-timeout-ms` | 继承 `status-timeout-ms` | HID 专用超时 |
| `battery-timeout-ms` | 继承 `status-timeout-ms` | 电池专用超时 |
| `ble-pairing-hold` | 1 | 配置文件开放期间保持 BLE 可见 |

---

## 16. 运行时 API

### 16.1 电源控制

| 函数 | 说明 |
|------|------|
| `zmk_led_matrix_on()` | 开启 |
| `zmk_led_matrix_off()` | 关闭 |
| `zmk_led_matrix_toggle()` | 切换 |
| `zmk_led_matrix_is_on()` | 查询开/关状态 |

### 16.2 亮度

| 函数 | 说明 |
|------|------|
| `zmk_led_matrix_set_brt(uint8_t)` | 直接设置亮度 |
| `zmk_led_matrix_get_brt()` | 获取当前亮度 |
| `zmk_led_matrix_calc_brt(int direction)` | 计算下一个亮度步进 |

### 16.3 速度

| 函数 | 说明 |
|------|------|
| `zmk_led_matrix_set_spd(uint8_t)` | 直接设置速度 |
| `zmk_led_matrix_get_spd()` | 获取当前速度 |
| `zmk_led_matrix_calc_spd(int direction)` | 计算下一个速度步进 |

### 16.4 手动色相

| 函数 | 说明 |
|------|------|
| `zmk_led_matrix_set_hue_index(uint8_t)` | 设置手动色相索引 |
| `zmk_led_matrix_get_hue_index()` | 获取当前色相索引 |
| `zmk_led_matrix_calc_hue_index(int direction)` | 计算下一个色相步进 |

### 16.5 显示配置文件

| 函数 | 说明 |
|------|------|
| `zmk_led_matrix_set_profile(uint8_t)` | 设置显示配置文件 |
| `zmk_led_matrix_get_profile()` | 获取当前配置文件 |
| `zmk_led_matrix_calc_profile(int direction)` | 计算下一个/上一个配置文件 |

### 16.6 独立配置文件

| 函数 | 说明 |
|------|------|
| `zmk_led_matrix_set_independent_profile(uint8_t child, uint8_t profile)` | 设置独立子的配置文件 |
| `zmk_led_matrix_get_independent_profile(uint8_t child)` | 获取独立子的配置文件 |
| `zmk_led_matrix_calc_independent_profile(uint8_t child, int direction)` | 计算下一个/上一个独立配置文件 |

### 16.7 状态查询

| 函数 | 说明 |
|------|------|
| `zmk_led_matrix_get_state(struct zmk_led_matrix_state *out)` | 读取当前状态快照 |
| `zmk_led_matrix_show_status(uint8_t sources_mask)` | 手动触发状态显示 |

### 16.8 状态结构体

```c
struct zmk_led_matrix_state {
    bool enabled;
    uint8_t brightness;
    enum zmk_led_matrix_render_source effective_source;
    uint8_t selected_profile_id;
    const char *selected_profile_name;
    const char *selected_renderer_name;
    uint8_t effective_status_sources_mask;
    zmk_keymap_layer_id_t effective_layer_color_layer_id;
};
```

---

## 17. 事件系统

### 17.1 状态变更事件

事件类型：`zmk_led_matrix_state_changed`

包含 `uint8_t reason_mask`，各标志位含义：

| 标志位 | 位 | 说明 |
|--------|----|------|
| `ZMK_LED_MATRIX_STATE_CHANGE_ENABLED` | BIT(0) | 开/关状态变更 |
| `ZMK_LED_MATRIX_STATE_CHANGE_BRIGHTNESS` | BIT(1) | 亮度变更 |
| `ZMK_LED_MATRIX_STATE_CHANGE_PROFILE` | BIT(2) | 显示配置文件变更 |
| `ZMK_LED_MATRIX_STATE_CHANGE_LAYER_COLOR` | BIT(3) | 图层颜色叠加变更 |
| `ZMK_LED_MATRIX_STATE_CHANGE_STATUS` | BIT(4) | 状态叠加变更 |
| `ZMK_LED_MATRIX_STATE_CHANGE_SPEED` | BIT(5) | 动画速度变更 |
| `ZMK_LED_MATRIX_STATE_CHANGE_COLOR` | BIT(6) | 调色板色相索引变更 |
| `ZMK_LED_MATRIX_STATE_CHANGE_CHILD_PROFILE` | BIT(7) | 独立子配置文件变更 |

### 17.2 订阅的事件

LED Matrix 状态层（`status.c`）订阅以下事件：

- `zmk_battery_state_changed`
- `zmk_usb_conn_state_changed`
- `zmk_hid_indicators_changed`（需 `CONFIG_ZMK_HID_INDICATORS`）
- `zmk_ble_active_profile_changed`（需 `CONFIG_ZMK_BLE`）
- `zmk_endpoint_changed`
- `zmk_activity_state_changed`
- `zmk_position_state_changed`
- `zmk_layer_state_changed`

---

## 18. 键映射使用示例

```dts
#include <dt-bindings/zmk/led_matrix.h>

&lm LM_ON 0          // 开启
&lm LM_OFF 0         // 关闭
&lm LM_TOG 0         // 切换开/关
&lm LM_BRI 0         // 亮度增加
&lm LM_BRD 0         // 亮度减少
&lm LM_BRT 80        // 直接设置亮度 80%
&lm LM_EFF 0         // 下一个配置文件
&lm LM_EFR 0         // 上一个配置文件
&lm LM_EFS 1         // 设置配置文件 1
&lm LM_SPI 0         // 速度增加
&lm LM_SPD 0         // 速度减少
&lm LM_SPS 3         // 设置速度 3
&lm LM_HUI 0         // 色相增加
&lm LM_HUD 0         // 色相减少
&lm LM_HUS 5         // 设置色相索引 5
&lm LM_STA LM_STA_BLE // 显示 BLE 状态
&lm LM_STA LM_STA_ALL // 显示所有状态
&lm LM_CFF 0         // 下一个独立配置文件
&lm LM_CFR 0         // 上一个独立配置文件
&lm LM_CFS(0, 1) 0   // 设置子 0 到配置文件 1
```

---

## 19. 常见错误

1. 缺少 `/chosen { zmk,led-matrix = ...; }` — 构建/初始化失败
2. `led-driver` 与设备 phandle 不匹配 — 后端建立失败
3. 无可用渲染源（无配置文件、无独立子、无图层颜色、无状态 LED）— 被拒绝
4. `channel-indexes` 长度不等于 `led-count * 3` — 验证失败
5. 位置驱动渲染器未使用 `matrix-led-grid` — 布局不确定
6. `layer-leds` / `independent-leds` 重复或重叠 — 被拒绝
7. `independent-id` / `profile-id` 不连续或重复 — 被拒绝
8. `led_composite` 子设备同时有或没有设备类型 — 被拒绝
9. 子设备通道范围不连续或重叠 — 被拒绝
10. 条带子设备 `channel-count` 不是 3 的倍数 — 被拒绝
11. 多个条带子设备引用不同的 `ext-power-device` — 被拒绝
12. 在无 `write_channels` 支持的后端上使用 `channel-indexes` — 被拒绝

---

## 20. DTS 配置示例

以下示例来自 `app/boards/bmk/` 目录下的实际板级配置。

### 20.1 条带后端 — Corne54 分体键盘（WS2812）

**左半部分**（`corne54/corne54_left.dts`）：

```dts
#include <dt-bindings/zmk/ext_power.h>
#include <dt-bindings/zmk/led_matrix.h>
#include <dt-bindings/zmk/led_matrix_colors.h>

/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    EXT_POWER {
        compatible = "zmk,ext-power-generic";
        control-gpios = <&gpio0 17 GPIO_ACTIVE_LOW>;
        init-delay-ms = <100>;
    };

    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "strip";
        led-strip-device = <&led_strip>;
        led-count = <28>;
        matrix-led-grid = <
            0 1 2 3 4 5 LM_BREAK
            6 7 8 9 10 11 LM_BREAK
            12 13 14 15 16 17 LM_BREAK
            18 19 20 21 22 23 LM_BREAK
            LM_PH LM_PH 24 25 26 27
        >;
        status-timeout-ms = <3000>;
        ble-pairing-hold = <1>;

        // 电池状态：5 级电量指示
        battery-leds = <1 2 3 4 5>;
        battery-color-on = <LM_RED LM_ORANGE LM_YELLOW LM_GREEN LM_GREEN>;
        battery-color-off = <LM_BLACK>;

        // HID Caps Lock 指示
        hid-caps-leds = <12>;
        hid-caps-color-on = <LM_WHITE>;
        hid-caps-color-off = <LM_BLACK>;

        // BLE 状态指示
        ble-profile-leds = <19 20 21 22>;
        ble-color-open = <LM_BLUE>;
        ble-color-connected = <LM_CYAN>;
        ble-color-disconnected = <LM_RED>;

        // 8 个显示配置文件
        profile0 {
            profile-id = <0>;
            display-name = "Palette";
            renderer = "solid";
            colors = <LM_PALETTE>;
            brightness = <100>;
            period-ms = <1500>;
        };

        profile1 {
            profile-id = <1>;
            display-name = "Breathing Blue";
            renderer = "breathe";
            colors = <LM_DIM_BLUE>;
            brightness = <100>;
            period-ms = <1800>;
        };

        profile4 {
            profile-id = <4>;
            display-name = "Keypress";
            renderer = "keypress";
            independent-led-mode = "static";
            keypress-afterglow-ms = <1000>;
            keypress-fadeout-ms = <250>;
            keypress-range = <0>;
            colors = <LM_DIM_CYAN>;
            brightness = <100>;
            period-ms = <350>;
        };

        profile5 {
            profile-id = <5>;
            display-name = "Ripple";
            renderer = "ripple";
            colors = <LM_RAINBOW>;
            brightness = <100>;
            period-ms = <1500>;
        };
        // ... profile2/3/6/7 省略，模式类似
    };
};
```

**右半部分**（`corne54v3/corne54v3_right.dts`）— 不同的网格布局，使用 `LM_NOLED` 和 `LM_PH`：

```dts
    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "strip";
        led-strip-device = <&led_strip>;
        led-count = <28>;
        matrix-led-grid = <
            0 1 2 3 4 5 LM_BREAK
            6 7 8 9 10 11 LM_BREAK
            12 13 14 15 16 17 LM_BREAK
            18 19 20 21 22 23 LM_BREAK
            LM_PH 24 25 26 LM_PH LM_PH LM_BREAK
            LM_NOLED LM_NOLED LM_NOLED LM_NOLED 27 LM_PH
        >;
        // ... 其余配置同左半部分
    };
```

**关键点：**
- `LM_BREAK` 分隔行，行数必须与 kscan 行数匹配
- `LM_PH` 表示占位单元（有键位置但无 LED）
- `LM_NOLED` 表示键存在但无 LED 映射
- 分体键盘左右半部分各自定义本地网格

---

### 20.2 矩阵后端 — Numpad（IS31FL3737）

**完整配置**（`numpad/numpad_nrf52840_zmk.dts`）：

```dts
#include <dt-bindings/zmk/led_matrix.h>
#include <dt-bindings/zmk/led_matrix_colors.h>

/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "matrix";
        led-device = <&is31_led>;
        led-count = <20>;
        output-channel-count = <144>;

        // IS31FL3737 密集通道空间映射（R, G, B 顺序）
        channel-indexes = <
            12 0 24   48 36 60   84 72 96   120 108 132
            13 1 25   49 37 61   85 73 97   121 109 133
            14 2 26   50 38 62   86 74 98   122 110 134
            15 3 27   51 39 63   87 75 99   123 111 135
            16 4 28   52 40 64   88 76 100  124 112 136
        >;

        matrix-led-grid = <
            0 1 2 3 LM_BREAK
            4 5 6 7 LM_BREAK
            8 9 10 11 LM_BREAK
            12 13 14 15 LM_BREAK
            16 17 18 19
        >;
        status-timeout-ms = <3000>;
        ble-pairing-hold = <1>;

        // 电池：4 级电量颜色（动态桶）
        battery-leds = <4 5 6 7>;
        battery-color-off = <LM_BLACK>;
        battery-level-colors = <LM_RED LM_ORANGE LM_YELLOW LM_GREEN>;

        // BLE 状态
        ble-profile-leds = <12 13 14 15>;
        ble-color-open = <LM_BLUE>;
        ble-color-connected = <LM_CYAN>;
        ble-color-disconnected = <LM_RED>;

        // HID Num Lock 指示
        hid-num-leds = <0>;
        hid-num-color-on = <LM_WHITE>;
        hid-num-color-off = <LM_BLACK>;

        // 配置文件...
        profile0 {
            profile-id = <0>;
            display-name = "Palette Static";
            renderer = "solid";
            colors = <LM_PALETTE>;
            brightness = <100>;
            period-ms = <1500>;
        };
        // ... 其余配置文件
    };
};

// I2C 总线上的 IS31FL3737 设备
&i2c0 {
    compatible = "nordic,nrf-twi";
    pinctrl-0 = <&i2c0_default>;
    pinctrl-1 = <&i2c0_sleep>;
    pinctrl-names = "default", "sleep";
    clock-frequency = <400000>;
    status = "okay";

    is31_led: is31fl3737@50 {
        compatible = "zmk,is31fl3737";
        reg = <0x50>;
        sdb-gpios = <&gpio0 6 GPIO_ACTIVE_HIGH>;
    };
};
```

**关键点：**
- `led-driver = "matrix"` 配合 `led-device = <&is31_led>`
- `output-channel-count = <144>` 指定 IS31FL3737 的物理通道数（20 LED × 3 RGB = 60 逻辑通道，但芯片有 144 个 PWM 通道）
- `channel-indexes` 将逻辑 RGB 通道映射到芯片的密集通道空间
- `battery-level-colors` 替代 `battery-color-on`，提供基于电量等级的颜色
- IS31FL3737 设备需要 `sdb-gpios` 用于硬件关断控制

---

### 20.3 条带后端 + 独立 LED 组 — Numpadv2（WS2812）

**完整配置**（`numpadv2/numpadv2_nrf52840_zmk.dts`）：

```dts
#include <dt-bindings/zmk/led_matrix.h>
#include <dt-bindings/zmk/led_matrix_colors.h>

/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    EXT_POWER {
        compatible = "zmk,ext-power-generic";
        control-gpios = <&gpio1 14 GPIO_ACTIVE_LOW>;
        init-delay-ms = <100>;
    };

    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "strip";
        led-strip-device = <&led_strip>;
        led-count = <24>;

        matrix-led-grid = <
            4 5 6 7 LM_BREAK
            8 9 10 11 LM_BREAK
            12 13 14 15 LM_BREAK
            16 17 18 19 LM_BREAK
            20 21 22 23
        >;

        // BLE 状态
        ble-profile-leds = <16 17 18 19>;
        ble-color-open = <LM_BLUE>;
        ble-color-connected = <LM_CYAN>;
        ble-color-disconnected = <LM_RED>;

        // HID Num Lock
        hid-num-leds = <4>;
        hid-num-color-on = <LM_GREEN>;
        hid-num-color-off = <LM_BLACK>;

        // 电池：4 级电量颜色
        battery-leds = <0 1 2 3>;
        battery-level-colors = <LM_RED LM_ORANGE LM_YELLOW LM_GREEN>;
        battery-color-off = <LM_BLACK>;

        // 独立 LED 组：电池指示灯（始终点亮）
        independent0 {
            independent-id = <0>;
            independent-leds = <0 1 2 3>;

            profile0 {
                profile-id = <0>;
                renderer = "solid";
                colors = <LM_BLACK>;
                brightness = <100>;
                period-ms = <1000>;
            };
        };

        // 主配置文件...
        profile0 {
            profile-id = <0>;
            display-name = "Palette";
            renderer = "solid";
            colors = <LM_PALETTE>;
            brightness = <100>;
            period-ms = <1000>;
        };
        // ... 其余配置文件
    };
};

// WS2812 LED 条带设备
&spi3 {
    compatible = "nordic,nrf-spim";
    status = "okay";
    pinctrl-0 = <&spi3_default>;
    pinctrl-1 = <&spi3_sleep>;
    pinctrl-names = "default", "sleep";

    led_strip: ws2812@0 {
        compatible = "worldsemi,ws2812-spi";
        reg = <0>;
        spi-max-frequency = <4000000>;
        chain-length = <24>;
        spi-one-frame = <0x70>;
        spi-zero-frame = <0x40>;
        color-mapping = <LED_COLOR_ID_GREEN
                        LED_COLOR_ID_RED
                        LED_COLOR_ID_BLUE>;
    };
};
```

**关键点：**
- `independent0` 容器将 LED 0-3 分配为独立组，不受主配置文件控制
- 独立组的 `independent-leds` 不能与 `matrix-led-grid` 中的 LED 重叠（此例中 grid 从 LED 4 开始）
- WS2812 设备通过 SPI 接口驱动，需要 `spi-one-frame` 和 `spi-zero-frame` 配置
- `color-mapping` 指定 GRB 顺序（WS2812 标准）

---

### 20.4 最小配置 — BMK4SGK（GPIO LED 仅 HID 指示）

**完整配置**（`bmk4SGK/bmk4SGK_nrf52840_zmk.dts`）：

```dts
#include <dt-bindings/zmk/led_matrix_colors.h>

/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    // GPIO LED 设备（非 RGB，仅开/关）
    status_leds: leds {
        compatible = "gpio-leds";

        caps_lock_led: caps_lock_led {
            gpios = <&gpio1 5 GPIO_ACTIVE_LOW>;
            label = "Caps lock LED";
        };

        scroll_lock_led: scroll_lock_led {
            gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
            label = "Scroll lock LED";
        };
    };

    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "matrix";
        led-device = <&status_leds>;
        led-count = <2>;
        led-indexes = <0 1>;
        hid-timeout-ms = <0>;  // 持久显示，不超时

        hid-caps-leds = <0>;
        hid-caps-color-on = <LM_WHITE>;
        hid-caps-color-off = <LM_BLACK>;

        hid-scroll-leds = <1>;
        hid-scroll-color-on = <LM_WHITE>;
        hid-scroll-color-off = <LM_BLACK>;
    };
};
```

**关键点：**
- 使用 `gpio-leds` 作为后端，仅支持二进制开/关（非 RGB）
- `led-indexes = <0 1>` 映射逻辑 LED 到物理 GPIO LED
- `hid-timeout-ms = <0>` 使 HID 状态持久显示（不超时清除）
- 无配置文件、无网格、无渲染器 — 仅用于 HID 指示灯
- 最小化配置，适合只有几个状态 LED 的简单键盘

---

### 20.5 配置模式总结

| 板型 | 后端类型 | LED 设备 | LED 数 | 网格 | 状态源 | 独立组 | 配置文件数 |
|------|----------|----------|--------|------|--------|--------|-----------|
| Corne54 | strip | WS2812 | 28 | 5行×6列 | 电池+HID Caps+BLE | 无 | 8 |
| Corne54v3 | strip | WS2812 | 28 | 6行×6列 | 电池+HID Caps+BLE | 无 | 8 |
| Numpad | matrix | IS31FL3737 | 20 | 5行×4列 | 电池(等级)+BLE+HID Num | 无 | 7 |
| Numpadv2 | strip | WS2812 | 24 | 5行×4列 | 电池(等级)+BLE+HID Num | 1组 | 8 |
| BMK4SGK | matrix | GPIO LED | 2 | 无 | HID Caps+HID Scroll | 无 | 无 |

---

## 21. LED Composite 驱动详解

### 21.1 概述

`zmk,led-composite` 是一个**虚拟 LED 控制器**，将多个异构 LED 后端（矩阵控制器 + LED 条带）合并为一个扁平化的 RGB 通道地址空间。LED Matrix 子系统向这一个设备写入，composite 驱动负责将写入分发到正确的物理子设备。

```
┌─────────────────────────────────────────────────┐
│              LED Matrix 子系统                    │
│         (渲染 → write_channels)                  │
└────────────────────┬────────────────────────────┘
                     │
         ┌───────────▼───────────┐
         │   zmk,led-composite   │
         │   (扁平化通道空间)     │
         └───┬───────────────┬───┘
             │               │
    ┌────────▼──────┐ ┌──────▼────────┐
    │ Matrix 子设备  │ │ Strip 子设备   │
    │ (IS31FL3737)  │ │ (WS2812)      │
    │ write_channels│ │ led_strip_*   │
    └───────────────┘ └───────────────┘
```

### 21.2 子设备类型

#### Matrix 子设备（`led-device`）

Zephyr `led` API 设备，通过 `write_channels` 接收数据。

**支持的矩阵硬件：**

| 类型 | compatible | 电源管理 |
|------|-----------|----------|
| 通用 | 任意 Zephyr LED 设备 | 无托管电源 |
| IS31FL3733 | `issi,is31fl3733` | 软 blank + SDB GPIO 硬关断 + 恢复重编程 |
| IS31FL3737 | `zmk,is31fl3737` | 软 blank + SDB GPIO 硬关断 + 恢复重编程 |
| Mock IS31FL3733 | `zmk,led-mock` + `managed-power-kind` | 模拟完整电源行为 |
| Mock IS31FL3737 | `zmk,led-mock` + `managed-power-kind` | 模拟完整电源行为 |

#### Strip 子设备（`led-strip-device`）

Zephyr `led_strip` API 设备（如 WS2812、SK6812）。

- `channel-count` 必须为 3 的倍数（RGB）
- 像素数 = `channel_count / 3`
- 可引用 `ext-power-device` 进行硬件电源控制
- 多个 strip 子设备共享同一 `ext-power-device` 时必须引用相同设备

### 21.3 子设备属性

| 属性 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `led-device` | phandle | 二选一 | Matrix 样式 LED 设备 |
| `led-strip-device` | phandle | 二选一 | LED 条带设备 |
| `channel-offset` | int | 是 | 扁平通道空间中的起始偏移，必须从 0 开始连续递增 |
| `channel-count` | int | 是 | RGB 通道数（条带子设备必须为 3 的倍数） |
| `ext-power-device` | phandle | 否 | 共享外部电源控制器（仅条带子设备） |

### 21.4 能力标志

| 标志 | 含义 |
|------|------|
| `ZMK_LED_COMPOSITE_CAP_MANAGED_POWER` | 至少一个子设备支持软件/硬件电源控制（IS31 with SDB GPIO 或 strip with ext-power） |
| `ZMK_LED_COMPOSITE_CAP_FULL_HARD_OFF` | 所有子设备均可完全硬件关断 |

### 21.5 写入路径

```
zmk_led_composite_write_channels(start, count, buf)
    │
    ├── 遍历子设备，映射全局 [start, start+count) 到子设备本地偏移
    │
    ├── Matrix 子设备 → led_write_channels(child_dev, local_offset, len, buf)
    │
    └── Strip 子设备 → 拷贝到内部缓冲区
         ├── 优先: led_strip_update_channels() (flat channel API)
         └── 回退: led_strip_update_rgb() (pixel struct API)
```

### 21.6 电源管理

composite 驱动实现两层电源模型：

**软电源（Blanking）** — IS31 矩阵芯片：
- 关闭：写入芯片配置寄存器禁用输出
- 开启：重新启用配置寄存器

**硬电源（SDB GPIO 或 ext-power）**：
- IS31 矩阵 + SDB GPIO：断开 SDB 引脚物理断电，恢复时重新编程所有寄存器
- LED 条带 + ext-power-device：调用共享外部电源设备的 enable/disable

**关断顺序**：先 blank/关闭子设备，再禁用共享 ext-power。上电顺序相反。

### 21.7 Kconfig

| 配置项 | 默认值 | 依赖 | 说明 |
|--------|--------|------|------|
| `CONFIG_ZMK_LED_COMPOSITE` | y | `LED` + `DT_HAS_ZMK_LED_COMPOSITE_ENABLED` | 启用 composite 驱动，自动选择 `LED_STRIP` |

---

## 22. LED Composite DTS 配置示例

### 22.1 单一 Matrix 子设备

最简单的 composite 配置，只有一个 matrix 子设备：

```dts
#include <dt-bindings/led/led.h>

/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    // Matrix 子设备（mock，12 通道）
    composite_single_child: composite_single_child {
        compatible = "zmk,led-mock";
        mode = "channels";
        channel-count = <12>;

        led_0 {
            index = <0>;
            color-mapping = <LED_COLOR_ID_RED>;
        };
    };

    // Composite 驱动：1 个子设备
    led_chain: led_composite {
        compatible = "zmk,led-composite";

        child0 {
            led-device = <&composite_single_child>;
            channel-offset = <0>;
            channel-count = <12>;
        };
    };

    // LED Matrix 使用 composite 作为后端
    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "matrix";
        led-device = <&led_chain>;
        led-count = <4>;
        output-channel-count = <12>;
        channel-indexes = <
            0 1 2
            3 4 5
            6 7 8
            9 10 11
        >;

        profile0 {
            profile-id = <0>;
            display-name = "Composite Single";
            renderer = "solid";
            colors = <0x010203 0x040506 0x070809 0x0A0B0C>;
            brightness = <100>;
            period-ms = <1000>;
        };
    };
};
```

**要点：**
- composite 的 `channel-offset` 从 0 开始
- `channel-count` 总和 = 所有子设备通道数之和
- LED Matrix 的 `output-channel-count` 等于 composite 总通道数

---

### 22.2 多个 Matrix 子设备（分体/分区）

两个 matrix 子设备，通道连续排列：

```dts
/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    // 子设备 0：5 通道
    composite_split_child0: composite_split_child0 {
        compatible = "zmk,led-mock";
        mode = "channels";
        channel-count = <5>;

        led_0 {
            index = <0>;
            color-mapping = <LED_COLOR_ID_RED>;
        };
    };

    // 子设备 1：7 通道
    composite_split_child1: composite_split_child1 {
        compatible = "zmk,led-mock";
        mode = "channels";
        channel-count = <7>;

        led_0 {
            index = <0>;
            color-mapping = <LED_COLOR_ID_RED>;
        };
    };

    // Composite：5 + 7 = 12 通道
    led_chain: led_composite {
        compatible = "zmk,led-composite";

        child0 {
            led-device = <&composite_split_child0>;
            channel-offset = <0>;    // 起始 0
            channel-count = <5>;
        };

        child1 {
            led-device = <&composite_split_child1>;
            channel-offset = <5>;    // 紧接 child0 之后
            channel-count = <7>;
        };
    };

    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "matrix";
        led-device = <&led_chain>;
        led-count = <4>;
        output-channel-count = <12>;
        channel-indexes = <
            0 1 2
            3 4 5
            6 7 8
            9 10 11
        >;

        profile0 {
            profile-id = <0>;
            display-name = "Composite Split";
            renderer = "solid";
            colors = <0x010203 0x040506 0x070809 0x0A0B0C>;
            brightness = <100>;
            period-ms = <1000>;
        };
    };
};
```

**要点：**
- `channel-offset` 必须从 0 开始，连续递增，无间隙无重叠
- child0: offset=0, count=5 → 通道 0-4
- child1: offset=5, count=7 → 通道 5-11
- 总通道 = 5 + 7 = 12

---

### 22.3 混合后端（Matrix + Strip）

matrix 子设备与 strip 子设备混合，strip 使用 `led-strip-device`：

```dts
#include <dt-bindings/led/led.h>

/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    ext_power_mock: ext_power_mock {
        compatible = "zmk,ext-power-mock";
    };

    // Matrix 子设备：IS31FL3737 mock，6 通道，带 SDB GPIO
    matrix_power_child: matrix_power_child {
        compatible = "zmk,led-mock";
        mode = "channels";
        channel-count = <6>;
        managed-power-kind = "is31fl3737";
        sdb-gpios = <&gpio0 0 GPIO_ACTIVE_HIGH>;

        led_0 {
            index = <0>;
            color-mapping = <LED_COLOR_ID_RED>;
        };
    };

    // Strip 子设备：WS2812 mock，2 像素 = 6 通道
    strip_power_child: strip_power_child {
        compatible = "zmk,led-strip-mock";
        chain-length = <2>;
        support-update-channels;
    };

    // Composite：matrix(6) + strip(6) = 12 通道
    led_chain: led_composite {
        compatible = "zmk,led-composite";

        child0 {
            led-device = <&matrix_power_child>;      // Matrix 用 led-device
            channel-offset = <0>;
            channel-count = <6>;
        };

        child1 {
            led-strip-device = <&strip_power_child>;  // Strip 用 led-strip-device
            channel-offset = <6>;
            channel-count = <6>;                       // 2 像素 × 3 = 6 通道
            ext-power-device = <&ext_power_mock>;     // 共享外部电源
        };
    };

    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "matrix";
        led-device = <&led_chain>;
        led-count = <4>;
        output-channel-count = <12>;
        channel-indexes = <
            0 1 2
            3 4 5
            6 7 8
            9 10 11
        >;

        profile0 {
            profile-id = <0>;
            display-name = "Composite Mixed Power";
            renderer = "solid";
            colors = <0x010203 0x040506 0x070809 0x0A0B0C>;
            brightness = <100>;
            period-ms = <1000>;
        };
    };
};
```

**要点：**
- Matrix 子设备用 `led-device`，Strip 子设备用 `led-strip-device`
- Matrix 子设备支持 `managed-power-kind` 和 `sdb-gpios` 进行硬件电源管理
- Strip 子设备通过 `ext-power-device` 共享外部电源控制
- 所有 strip 子设备引用的 `ext-power-device` 必须相同

---

### 22.4 混合后端（Matrix + Strip，Strip 用 channels API）

strip 子设备支持 `led_strip_update_channels` flat channel API（非 RGB pixel API）：

```dts
/ {
    chosen {
        zmk,led-matrix = &led_matrix;
    };

    matrix_child: matrix_child {
        compatible = "zmk,led-mock";
        mode = "channels";
        channel-count = <6>;

        led_0 {
            index = <0>;
            color-mapping = <LED_COLOR_ID_RED>;
        };
    };

    // Strip 子设备：支持 update-channels 的 mock
    strip_child: strip_child {
        compatible = "zmk,led-strip-mock";
        chain-length = <2>;
        support-update-channels;  // 启用 flat channel API
    };

    led_chain: led_composite {
        compatible = "zmk,led-composite";

        child0 {
            led-device = <&matrix_child>;
            channel-offset = <0>;
            channel-count = <6>;
        };

        child1 {
            led-strip-device = <&strip_child>;
            channel-offset = <6>;
            channel-count = <6>;
        };
    };

    led_matrix: led_matrix {
        compatible = "zmk,led-matrix";
        led-driver = "matrix";
        led-device = <&led_chain>;
        led-count = <4>;
        output-channel-count = <12>;
        channel-indexes = <
            0 1 2
            3 4 5
            6 7 8
            9 10 11
        >;

        profile0 {
            profile-id = <0>;
            display-name = "Composite Mixed Strip Channels";
            renderer = "solid";
            colors = <0x010203 0x040506 0x070809 0x0A0B0C>;
            brightness = <100>;
            period-ms = <1000>;
        };
    };
};
```

**要点：**
- strip 子设备声明 `support-update-channels` 时，composite 优先使用 `led_strip_update_channels()`（flat channel API）
- 不支持时自动回退到 `led_strip_update_rgb()`（pixel struct API）

---

### 22.5 无效配置示例（常见错误）

以下配置会导致验证失败：

```dts
// ❌ 错误：子设备通道范围有间隙
child0 { channel-offset = <0>; channel-count = <5>; };  // 0-4
child1 { channel-offset = <6>; channel-count = <5>; };  // 6-10（间隙：5）

// ❌ 错误：子设备通道范围重叠
child0 { channel-offset = <0>; channel-count = <6>; };  // 0-5
child1 { channel-offset = <4>; channel-count = <6>; };  // 4-9（重叠：4-5）

// ❌ 错误：strip 子设备 channel-count 不是 3 的倍数
child0 { led-strip-device = <&strip>; channel-count = <5>; };

// ❌ 错误：matrix 子设备引用 ext-power-device
child0 { led-device = <&is31>; ext-power-device = <&ext>; };

// ❌ 错误：多个 strip 子设备引用不同的 ext-power-device
child0 { led-strip-device = <&s1>; ext-power-device = <&ext1>; };
child1 { led-strip-device = <&s2>; ext-power-device = <&ext2>; };

// ❌ 错误：子设备同时有 led-device 和 led-strip-device
child0 { led-device = <&dev>; led-strip-device = <&strip>; };

// ❌ 错误：子设备既没有 led-device 也没有 led-strip-device
child0 { channel-offset = <0>; channel-count = <6>; };
```

---

### 22.5 真实硬件配置参考

以下是 `app/boards/bmk/` 中的实际板级配置使用的模式：

| 板型 | Composite 使用 | 子设备类型 | 说明 |
|------|---------------|-----------|------|
| Corne54/Corne54v3 | 否 | 直接 strip | WS2812 条带直接作为 `led-strip-device` |
| Numpad | 否 | 直接 matrix | IS31FL3737 直接作为 `led-device` |
| Numpadv2 | 否 | 直接 strip | WS2812 条带直接作为 `led-strip-device` |
| BMK4SGK | 否 | 直接 matrix | GPIO LED 直接作为 `led-device` |

**注意：** 当前 BMK 板型均未使用 `zmk,led-composite`，而是直接将单一后端设备作为 `led-device` 或 `led-strip-device`。composite 驱动主要用于以下场景：

1. **同一键盘上混合使用矩阵芯片和 LED 条带**（如 IS31FL3737 + WS2812）
2. **多个独立的 LED 硬件需要统一地址空间**（如左右半部分各有独立 LED 控制器）
3. **需要统一电源管理**（matrix 的 SDB GPIO + strip 的 ext-power 共享控制）

---

### 22.6 验证规则速查

| 规则 | 说明 |
|------|------|
| `channel-offset` 从 0 开始 | 第一个子设备必须 offset=0 |
| 连续递增无间隙 | child[n+1].offset = child[n].offset + child[n].count |
| 无重叠 | 子设备通道范围不能交叉 |
| 每个子设备恰好一种设备类型 | `led-device` 和 `led-strip-device` 二选一 |
| strip `channel-count` 为 3 的倍数 | RGB 像素格式 |
| strip 共享 ext-power | 所有引用 ext-power 的 strip 子设备必须指向同一设备 |
| matrix 不可引用 ext-power | matrix 子设备不支持 `ext-power-device` |
| 子设备目标设备就绪 | DTS 中引用的设备必须存在且可用 |
