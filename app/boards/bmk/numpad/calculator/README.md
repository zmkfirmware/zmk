# numpad 板载计算器

默认开启 `CONFIG_NUMPAD_CALCULATOR=y`。所有实现、绑定、头文件和测试都位于本板目录，
不修改 ZMK 的通用输入或显示核心。适用于当前 256×160 屏幕。

## 使用

| 按键 | 功能 |
| --- | --- |
| FN + NLCK | 切换键盘／计算器模式 |
| 数字、小数点、加减乘除 | 输入算式，先乘除后加减，可输入负数 |
| Enter | 等号，重复按保持结果 |
| BSPC | 删除算式最后一个字符；出结果后返回原算式删除 |
| DEL | 清空算式、结果和错误 |
| FN + Enter | 求值并向当前 USB／BLE 目标输入结果，不附加回车 |
| NLCK | 计算器模式下无操作，不改变电脑 Num Lock |

计算器模式下第一行中央显示 `Cal`，暂时隐藏锁定状态；中间显示算式和大字号结果，
退出后恢复原状态布局。FN 的灯光、蓝牙及输出切换仍可用。键盘模式的 FN + Enter
继续切换 Media 层；计算器模式下该组合专用于发送结果。

结果后输入数字开始新算式，输入运算符接续内部完整精度结果。BSPC 可编辑原算式。
连续运算符替换前一个运算符，减号可作为负号，例如 `2*-3`、`2--3`。
模式切换保留算式，重启恢复键盘模式并清空计算状态，不保存到 Flash。

算式最多 63 字符；超长、除零、不完整算式和溢出会显示英文提示，可退格修改或清空。
使用 double 计算，显示最多 12 位有效数字，必要时采用科学计数法；存在通常的浮点舍入。
显示的结果文本就是发送的内容，但接续运算使用内部 double 值。

发送每次按下／释放间隔 20 ms，发送过程中忽略重复发送请求。断连、切换目标、退出计算器
时停止后续字符；已输入的字符不会撤回，也不会在重连后自动续传。`Sent` 表示本机按键序列
完成，并不代表目标应用确认接收。发送前请松开其他按键，修饰键或相同数字键冲突时会停止。
主键区数字键不依赖 Num Lock；发送按常见英文／中文键盘布局设计，其他布局或输入法可能改变
小数点、负号和科学计数法 `e` 的实际输入效果。

## 结构与移植

- `engine.c/.h`：纯 C 计算引擎，不依赖 Zephyr、LVGL。
- `behavior.c`、`calculator.c/.h`：按键行为、模式状态和板级状态事件。
- `sender.c`：非阻塞 HID 发送；`widget.c/.h`：仅在显示队列中更新 LVGL。
- `dts/`、`include/`：本地行为 binding 及按键／层常量；`tests/`：本机测试。

移植时复制 `calculator/`，在目标板 `pre_dt_board.cmake` 将其加入 `DTS_ROOT`，
并接入本板 CMake、Kconfig 中的 calculator 配置。创建一个
`zmk,behavior-numpad-calculator` 行为节点，通过 `calculator-layer`、`function-layer`
指定新增的两个层，并加入对应键位。默认层号为 3、4，须位于原键盘层之上；目标键盘已有这些
层时同时修改头文件和键位层顺序。此实现面向单体或分体主端，每板只声明一个计算器行为实例。

在自定义状态屏构造函数中调用 `numpad_calculator_widget_init(screen, indicators)`，
第二个参数传入需要临时隐藏的锁定状态标签，无标签时传 `NULL`。
当前布局为上下各保留 24 px；更小屏幕需要调整布局。要求 LVGL 9 及 16、24、32 号
Montserrat 字体，Kconfig 自动选择字体和浮点格式化支持。
板级 defconfig 必须设置 `CONFIG_LV_USE_CUSTOM_SPRINTF=y`：当前 Zephyr LVGL 适配层将
`lv_snprintf`／`lv_vsnprintf` 别名为 C 库函数，CUSTOM 才会直接使用 C 库。
默认 BUILTIN 会覆盖 `snprintf`，未启用 LVGL 浮点时 `%.12g` 只输出 `g`；
CLIB 在这组别名下会递归调用 `vsnprintf`，也不可使用。编译检查防止移植时遗漏。

设置 `CONFIG_NUMPAD_CALCULATOR=n` 可禁用引擎、发送和显示；切换行为成为空操作，
现有 Base／Media／Fn 功能保留。默认 keymap 的两个计算器层仍存在但不会被切换行为激活。
使用自定义 keymap 时，需要复制计算器节点及层；未声明该节点时功能自动关闭。

## 验证

从仓库根目录执行：

```sh
python3 app/boards/bmk/numpad/calculator/tests/run_tests.py
west build -s app -d /tmp/zmk-numpad-calculator-on -b numpad//zmk
```

本机测试需要 C 编译器，启用 AddressSanitizer 和 UndefinedBehaviorSanitizer。
覆盖运算优先级、负数、小数、精度接续、错误恢复、输入上限，以及真实运行时代码在模拟
ZMK 环境中的模式切换、HID 隔离、发送配对、重复请求、断连及目标切换。
模拟环境不验证真实调度时序和传输链路。
已初始化 LVGL 子模块时，额外编译实际 LVGL 格式化源码，复现旧的 `g` 输出并验证 CUSTOM
配置下的全部运算结果；未初始化时会明确提示跳过该回归检查。

关闭功能时，用额外配置文件设置 `CONFIG_NUMPAD_CALCULATOR=n` 并传入
`-DEXTRA_CONF_FILE=<配置文件>`，使用独立构建目录。
若 SDK 未自动发现，显式设置 `ZEPHYR_TOOLCHAIN_VARIANT=zephyr` 和 `ZEPHYR_SDK_INSTALL_DIR`。

实机验收：Base／Media 下进入及退出；按住其他键切换模式后松开，确认不卡键；
检查 Cal、锁定状态恢复、长数字滚动、底部状态与休眠唤醒；分别在 USB／BLE 的文本框中
发送整数、负数、小数和科学计数法，并在发送中切换目标、断连，确认后续字符停止。
