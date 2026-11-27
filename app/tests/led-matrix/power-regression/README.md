# LED Matrix 电源回归

## 熄灯验收契约

- `LM_OFF` 和全局亮度归零停止周期刷新；异步输出收尾完成后，受控电源关闭。
- 用户开关仍为 ON、最终输出全黑且没有可见的时间变化需求时，停止 tick 并关闭可管理的
  输出电源；`state.on`、profile 和持久化设置不变。按键、layer、状态或配置事件再次需要
  显示时能够恢复；动画瞬时黑帧保留时间推进，不逐帧开关电源。
- 周期刷新仅由当前生效的颜色分支、实际配置的 zone 和覆盖后可见的动态内容维持。
  Caps OFF 配黑色时，未选中的虹彩 ON 色不应维持 tick；BLE pairing 真正闪烁仍须正常运行。
- composite 的 `power(false)` 必须逐 child 完成清零或可靠失能，0 表示全部 child 完成关闭，
  不等同于全体硬件断电。无门控 child 清屏失败须报错；首个失败不能阻止其他 child 和共享
  外电源关闭；重试跳过已成功关闭的 child。恢复失败须尝试回退全部输出和共享电源。
- composite 临时关断／恢复仅调用外电源 runtime API，不改变持久化开关或创建额外保存工作。

以上是功能验收要求，不能代替各项测试的运行记录。尤其需覆盖静态黑色、Caps／USB／BLE／
Battery 状态切换、动态内容被覆盖、按键余辉结束、动画短暂黑帧、混合门控故障及恢复失败。

## 本机故障测试

从仓库根目录运行：

```sh
python3 app/tests/led-matrix/power-regression/host/run.py
```

需要 C11 编译器及 AddressSanitizer/UndefinedBehaviorSanitizer；支持 macOS 和 Linux。
临时编译产物自动清理，测试失败返回非零。

- 直接编译真实 IS31FL3737 驱动及公共电源辅助函数，模拟 I²C 寄存器页、GPIO 和互斥锁。
- 验证 I²C/设备未就绪、软件 shutdown 失败仍尝试 SDB，GPIO 失败正确返回错误。
- 在恢复初始化的每个 I²C 操作注入一次失败，验证回退关断、下一次恢复和寄存器页缓存；重复 100 次关断/恢复。
- 渲染需求用例包含 67 项真实生产合成检查：当前 HID／USB／BLE／Battery 颜色、覆盖优先级、
  全黑输出、动画短暂黑帧、按键余辉和涟漪结束、idle 后首键／释放、雨滴长时间隐藏后恢复。
- 编译完整真实 composite 驱动，覆盖无门控、部分门控和完整门控组合；逐 child 关断失败
  继续遍历，清屏失败保留或由可靠失能收敛，已关闭 child 重试不再访问，runtime 外电源、
  恢复失败全体回退、未知状态重新初始化及 100 次关断／恢复循环。
- 提取真实直连 RGB、scalar LED、scalar triplet 输出函数及量化逻辑，在首／中／末输出注入
  错误；验证仍清零其他健康输出、返回首个错误、后续重试成功及再次点亮。
- 提取原样输出服务、同步睡眠、清屏关断和 USB 策略函数，模拟工作队列时间与设备错误；
  验证 100/500/2000 ms 三次重试、停止渲染、旧请求失效、sleep 取消、全黑门控及事件恢复，
  并覆盖首帧写入失败、外电源开启失败时的关断回退。
- 编译完整真实 generic 外电源驱动，覆盖多个控制 GPIO 中间失败仍处理其余引脚、失败后
  状态缓存标为未知并强制重试、runtime 与持久化保存的分离，以及初始化／PM 状态转换。
- API 用例运行真实亮度 setter、OFF／ON／getter 和异步收尾，验证全局亮度归零转入 OFF、
  保留先前亮度并在 ON 时恢复、getter 不改状态、显式命令的保存请求及保存错误返回语义。
- 重复 100 次“USB 下已空闲，再拔线/插回”策略切换，覆盖最新事件状态、手动 OFF、sleep、活动时拔线及重复 USB 事件。

这些测试运行生产 C 代码，但时间调度、总线、设备和状态提示外围依赖为确定性 fake，**不能证明 Zephyr 真实并发时序或硬件电流**。提取函数使用明确的定义边界；USB auto-off 的测试选用启用配置的实现，不使用禁用配置的空实现。上述列表说明覆盖内容，实际通过情况以本次运行退出码和各套件输出为准。

## Linux 集成测试

从仓库根目录运行，使用独立构建目录，关闭快照自动接受：

```sh
unset ZMK_TESTS_AUTO_ACCEPT
export ZMK_SRC_DIR=app
export ZMK_BUILD_DIR=build/led-matrix-power-tests
./app/run-test.sh app/tests/led-matrix/usb-idle-unplug-powerdown
./app/run-test.sh app/tests/led-matrix/usb-idle-unplug-expired-window
./app/run-test.sh app/tests/led-matrix
```

两个新增用例覆盖 USB 下已空闲后拔线、临时 Battery 提示后实际外部电源关闭、插线恢复、再次拔线、手动 OFF 后状态提示收尾，以及 sleep 期间插拔不恢复输出。第二个用例将自动状态唤醒窗口设为零，要求直接关断。

新增快照为预期电源转换序列，未设 `pending`；必须在 Linux 实际运行通过后才能确认集成验收。现有带 `pending` 的用例发生快照差异不能视为通过，应审阅具体差异，不能通过自动接受快照掩盖失败。本机 macOS 无法运行 native_sim，配置成功后会在 Zephyr POSIX 架构检查处停止。

本轮按 composite 新的确定顺序手工调整四份预期快照：`composite-mixed-power`、
`manual-off-auto-status-transient-powerdown`、`manual-off-persistent-status-forced-transient`
和 `sleep-managed-power-sync-shutdown`。matrix child 清零后立即 blank，再清零 strip；
恢复时先完成 matrix reinit，再开启共享 strip 电源。快照只移动现有日志，不增加或删除
事件；外电源日志保留 `runtime`。后三个用例原有 `pending` 标记保留，未自动接受快照，
也未在 Linux 执行；这些手工预期仍须真实 native_sim 运行确认。

## 固件与硬件验收

```sh
.venv/bin/west build -s app -d build/numpad-power-fix -b numpad//zmk -- \
  -DZEPHYR_TOOLCHAIN_VARIANT=zephyr
```

SDK 未自动发现时，添加本机实际的 `-DZEPHYR_SDK_INSTALL_DIR=...`。检查生成配置包含屏幕 blanking、背光 idle auto-off、LED Matrix idle auto-off，空闲阈值 30000 ms，未启用 ZMK_SLEEP。

板级默认策略也需重新生成配置验证：`corne54`／`corne54v3` 左右保持 15 s idle，禁用 deep
sleep 以保留 BLE 连接；`corne54v3` 左右与 `bmk4SGK` 的两份 defconfig 启用 LED Matrix
idle auto-off。`bmk4SGK` 保留 30 s idle；其他 idle 阈值及状态唤醒窗口不变。配置生成成功
只确认选项解析正确，不等于完整固件、真实唤醒或电流验收通过。

测量时固定 BLE 连接条件、亮度、灯效、电池电压和显示配置，断开调试器。电流在电池供电端测量；USB 插拔场景应记录插拔后、状态提示结束后的电流。SDB 低只代表芯片 shutdown，不能当作整机断电。

| 项目                              | 测量/记录  |
| --------------------------------- | ---------- |
| 固件 SHA-256、构建日期            | 待实测填写 |
| 电池容量、电压、BLE 主机/连接状态 | 待实测填写 |
| 相同外设条件下正常关断基线        | 待实测填写 |
| 逻辑 ON、静态全黑／Caps OFF 后关断 | 待实测填写 |
| 黑色收尾后按键／状态恢复与动画黑帧 | 待实测填写 |
| 纯电池闲置，屏幕和背光关闭        | 待实测填写 |
| USB 已空闲后拔线，提示结束        | 待实测填写 |
| BLE 断开/重连，提示结束           | 待实测填写 |
| 100 次空闲/唤醒和插拔循环         | 待实测填写 |
| 48 小时静置电流轨迹及消耗         | 待实测填写 |

通过条件：每次提示结束都回到正常关断基线，无持续高电流平台；P0.06/SDB 保持低电平，LED 刷新定时器停止，无连续 LED I²C 写入。按键恢复显示和先前背光开关状态，LED 恢复正常；手动关灯保持关闭，仅允许既有规则下的临时提示。没有硬件测量数据时不得填写“实机通过”或推算保证续航。
