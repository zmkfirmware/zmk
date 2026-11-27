# LED Matrix / LED Composite 代码目录

本文件描述覆盖包中的生产代码与整合配置。路径均相对于上游 ZMK 根目录；
“覆盖”表示替换上游已有文件，“新增”表示上游基线中不存在该文件。

## 构建与配置

| 路径 | 状态 | 类别 | 功能简介 |
| --- | --- | --- | --- |
| `app/CMakeLists.txt` | 覆盖 | CMake | 按 Kconfig 编译 LED Matrix core、事件和行为实现。 |
| `app/Kconfig` | 覆盖 | Kconfig | 定义 `CONFIG_ZMK_LED_MATRIX`、后端 API 推导、亮度/速度/闲置/外部电源选项。 |
| `app/Kconfig.defaults` | 覆盖 | Kconfig | 提供 LED Matrix 默认亮度、速度、profile 和状态唤醒超时。 |
| `app/Kconfig.behaviors` | 覆盖 | Kconfig | 根据 DTS 的 `zmk,behavior-led-matrix` 节点启用行为。 |
| `app/dts/behaviors.dtsi` | 覆盖 | DTS 聚合 | 将 `led_matrix.dtsi` 纳入默认行为节点集合。 |
| `app/module/CMakeLists.txt` | 覆盖 | CMake | 让 module 驱动可使用应用层 public headers。 |
| `app/module/drivers/CMakeLists.txt` | 覆盖 | CMake | 注册 LED、LED strip 与 ext-power 子目录。 |
| `app/module/drivers/Kconfig` | 覆盖 | Kconfig | 引入 LED、LED strip 与 ext-power 驱动选项。 |
| `app/module/drivers/led/CMakeLists.txt` | 新增 | CMake | 编译 composite、IS31 电源辅助和 IS31FL3737 驱动。 |
| `app/module/drivers/led/Kconfig` | 新增 | Kconfig | 依据 DTS 启用 `ZMK_LED_COMPOSITE` 和 `ZMK_IS31FL3737`。 |
| `app/module/drivers/led_strip/CMakeLists.txt` | 新增 | CMake | 提供 LED strip driver 子目录，供 Zephyr LED_STRIP 集成。 |
| `app/module/drivers/led_strip/Kconfig` | 新增 | Kconfig | 提供 LED strip driver 的 Kconfig 容器。 |
| `app/module/drivers/ext_power/CMakeLists.txt` | 新增 | CMake | 提供 ext-power driver 子目录，避免启用 ext-power 时缺少目录。 |
| `app/module/drivers/ext_power/Kconfig` | 新增 | Kconfig | 提供 ext-power driver Kconfig 容器。 |

## LED Matrix 核心与公共接口

| 路径 | 状态 | 类别 | 功能简介 |
| --- | --- | --- | --- |
| `app/src/led_matrix.c` | 新增 | C | 子系统总装配：读取 chosen DTS、订阅 ZMK 事件、管理 settings 与渲染调度。 |
| `app/src/led_matrix/api.c` | 新增 | C | 实现对外状态、亮度、速度、profile、hue、独立子节点和状态显示 API。 |
| `app/src/led_matrix/core.c` | 新增 | C | 保存运行状态，仲裁 profile、layer color、independent 和状态覆盖层。 |
| `app/src/led_matrix/render.c` | 新增 | C | 将 renderer 结果写入 matrix 或 LED strip 后端。 |
| `app/src/led_matrix/status.c` | 新增 | C | 维护 Battery、USB、BLE、HID 状态快照与覆盖层优先级/超时。 |
| `app/src/led_matrix/power.c` | 新增 | C | 处理普通、自动闲置、强制显示与睡眠模式下的后端/外部电源控制。 |
| `app/src/led_matrix/internal.h` | 新增 | 内部头文件 | 共享状态、renderer、timer 与 runtime mode 声明。 |
| `app/include/zmk/led_matrix.h` | 新增 | 公共 C API | 声明 `zmk_led_matrix_on/off/toggle`、亮度/速度/hue/profile、状态读取与 `show_status`。 |
| `app/include/zmk/events/led_matrix_state_changed.h` | 新增 | ZMK 事件 | 定义状态变化原因和 `zmk_led_matrix_state_changed` 事件载荷。 |
| `app/src/events/led_matrix_state_changed.c` | 新增 | ZMK 事件 | 注册 LED Matrix 状态变化事件类型。 |
| `app/include/dt-bindings/zmk/led_matrix.h` | 新增 | DTS 宏 | 定义 `LM_ON`、`LM_BRT`、`LM_EFF`、`LM_STA`、子 profile 编码及状态位。 |
| `app/include/dt-bindings/zmk/led_matrix_colors.h` | 新增 | DTS 宏 | 定义 `LM_RGB`、基础颜色、状态颜色和 `LM_RAINBOW`/`LM_PALETTE` 语义值。 |

## Behavior 与 Devicetree

| 路径 | 状态 | 类别 | 功能简介 |
| --- | --- | --- | --- |
| `app/dts/behaviors/led_matrix.dtsi` | 新增 | DTS | 声明默认 `&lm` 行为节点。 |
| `app/dts/bindings/behaviors/zmk,behavior-led-matrix.yaml` | 新增 | DTS binding | 定义 `zmk,behavior-led-matrix` 兼容串及两个参数接口。 |
| `app/src/behaviors/behavior_led_matrix.c` | 新增 | C | 将 `LM_*` keymap 命令转换为 LED Matrix 公共 API 调用。 |
| `app/dts/bindings/zmk,led-matrix.yaml` | 新增 | DTS binding | 定义 `zmk,led-matrix`、后端选择、逻辑 LED 映射、profile、状态区和 renderer 子节点。 |

## LED Composite、IS31 与外部供电

| 路径 | 状态 | 类别 | 功能简介 |
| --- | --- | --- | --- |
| `app/include/drivers/led/led_composite.h` | 新增 | 公共 driver API | 声明 composite 通道数、能力查询和 `zmk_led_composite_power()`。 |
| `app/module/drivers/led/led_composite.h` | 新增 | 内部头文件 | 为 composite 驱动保留私有配置声明。 |
| `app/module/drivers/led/led_composite.c` | 新增 | C driver | 将多个 LED/LED strip child 合并为连续 RGB channel 空间，并实现校验、写入和电源管理。 |
| `app/module/dts/bindings/led/zmk,led-composite.yaml` | 新增 | DTS binding | 定义 `zmk,led-composite` 与 child 的后端、通道范围和共享 ext-power 属性。 |
| `app/include/drivers/led/is31_power.h` | 新增 | 公共辅助 API | 声明 IS31 软/硬关断、复位和 blank 操作的统一接口。 |
| `app/module/drivers/led/is31/power.c` | 新增 | C | 实现 IS31FL3733/3737 的总线校验、blank、SDB 与硬关断恢复。 |
| `app/include/drivers/led/is31fl3737.h` | 新增 | 公共 driver API | 声明 IS31FL3737 blank、电流、ABM 控制接口。 |
| `app/module/drivers/led/is31/is31fl3737.c` | 新增 | C driver | 实现 ZMK 管理的 IS31FL3737 LED API、分页写入与 ABM。 |
| `app/module/dts/bindings/led/zmk,is31fl3737.yaml` | 新增 | DTS binding | 定义 IS31FL3737 的 I2C、SDB GPIO 与 current-limit 属性。 |
| `app/include/drivers/ext_power.h` | 覆盖 | 公共 driver API | 增加不写入持久设置的 `ext_power_enable_runtime()` / `disable_runtime()`。 |
| `app/src/ext_power_generic.c` | 覆盖 | C driver | 区分运行时电源状态和持久化状态，供 LED Matrix 瞬态电源控制使用。 |

## 依赖关系

```text
keymap &lm / chosen zmk,led-matrix
  -> DTS bindings + Kconfig
  -> app/CMakeLists.txt
  -> LED Matrix core (api/core/render/status/power)
  -> LED API 或 LED_STRIP API
  -> zmk,led-composite（可选）-> LED/strip child 与 IS31 电源辅助
```

## 非编译交付文件

| 路径 | 用途 |
| --- | --- |
| `docs/sidebars.js` | 将 LED Matrix 加入 Lighting 文档导航。 |
| `docs/docs/development/hardware-integration/lighting/led-matrix.mdx` | 中英双语的安装、DTS 和运行参考。 |
| `docs/docs/led-matrix-led-composite-reference.mdx` | LED Matrix / composite 的详细设计参考。 |
| `README.md` | 基线、安装范围和校验方法。 |
| `MANIFEST.json`、`MANIFEST.sha256` | 机器可读文件清单与复制后完整性校验。 |
