# LED Matrix / LED Composite 上游覆盖包

## 基线

- 上游仓库：`https://github.com/zmkfirmware/zmk`
- 基线提交：`163bed680409afa92d2e46ade679f12ec6e20705`
- 基线分支：`main`

本目录是覆盖层，不是可独立构建的 ZMK checkout。它只包含通用的
LED Matrix、`zmk,led-composite`、IS31FL3737 与运行时外部电源支持；
不包含 bmk 板定义、keymap、测试行为、mock 驱动或测试夹具。

## 安装

在与上列提交一致的干净上游 checkout 根目录中执行：

```sh
cp -a /path/to/LED_matrix/app/. ./app/
cp -a /path/to/LED_matrix/docs/. ./docs/
```

不要把本目录整体复制到上游根目录：根目录中的 `README.md`、
`CODE_CATALOG.md` 和 `MANIFEST.json` 是此覆盖包的说明和校验资料，
不是上游源码的一部分。

之后，为目标板的 DTS 增加 `/chosen { zmk,led-matrix = &led_matrix; };`，
定义符合 `zmk,led-matrix` binding 的 `led_matrix` 节点，并在配置中启用
`CONFIG_ZMK_LED_MATRIX=y`。详见
`docs/docs/development/hardware-integration/lighting/led-matrix.mdx`。

## 兼容性与校验

- 此包按上述精确基线生成；其他版本的 `main` 可能需要手工处理合并冲突。
- `MANIFEST.json` 列出所有应复制的文件和 SHA-256。复制后可在包目录执行：

```sh
shasum -a 256 -c MANIFEST.sha256
```

- `CODE_CATALOG.md` 提供目录结构、文件职责、公开接口与数据流概览。
