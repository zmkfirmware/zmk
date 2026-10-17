# Repository Guidance

This repository is a ZMK firmware workspace. Future work in this checkout should focus on the
LED Matrix subsystem unless the user explicitly asks for something else. Touch non-LED-Matrix code
only when it is needed for LED Matrix build integration, tests, documentation, or hardware support.

## Project Shape

- ZMK is a Zephyr RTOS application rooted at `app/`.
- Extra Zephyr modules live under `app/module/` and are registered from `app/CMakeLists.txt`.
- The docs site lives in `docs/` and uses Docusaurus.
- The west manifest is `app/west.yml`; this workspace expects Zephyr and modules as sibling
  checkouts such as `zephyr/` and `modules/`.

## LED Matrix Focus

- Treat `docs/docs/led-matrix-led-composite-reference.mdx` as the authoritative reference for
  LED Matrix behavior, Devicetree properties, Kconfig semantics, and runtime expectations.
- Main implementation paths:
  - `app/src/led_matrix.c`
  - `app/src/led_matrix/`
  - `app/include/zmk/led_matrix.h`
  - `app/include/zmk/events/led_matrix_state_changed.h`
  - `app/src/events/led_matrix_state_changed.c`
  - `app/src/behaviors/behavior_led_matrix.c`
- Main Devicetree and Kconfig surfaces:
  - `app/dts/behaviors/led_matrix.dtsi`
  - `app/dts/bindings/behaviors/zmk,behavior-led-matrix.yaml`
  - `app/dts/bindings/zmk,led-matrix.yaml`
  - `app/include/dt-bindings/zmk/led_matrix.h`
  - `app/include/dt-bindings/zmk/led_matrix_colors.h`
  - `app/Kconfig`
  - `app/Kconfig.behaviors`
  - `app/Kconfig.defaults`
- LED Matrix tests live under `app/tests/led-matrix/`.
- The composite LED backend lives under `app/module/drivers/led/`, especially
  `app/module/drivers/led/led_composite.c` and its binding
  `app/module/dts/bindings/led/zmk,led-composite.yaml`.

## LED Matrix Change Rules

- Preserve the existing split between core configuration, rendering, status overlays, power
  management, runtime API, behavior commands, events, and backend drivers.
- When adding or changing a renderer, update the renderer implementation, allowed Devicetree
  values, docs, and LED Matrix tests together.
- When adding an `LM_*` command, update the dt-bindings header, behavior command handling,
  runtime API surface as needed, docs, and tests together.
- When changing persisted settings, version the stored format, add migration logic, and add a
  regression test that injects the prior format.
- When changing status overlay timing or priority, verify Battery, USB, BLE, and HID overlay
  interactions in `app/tests/led-matrix/`.
- When touching `led_composite`, keep it in `app/module/drivers/led/`; it is a Zephyr LED driver,
  not application-level LED Matrix code.

## Workspace Setup

Initialize or refresh the west workspace from the repo root:

```sh
west init -l app
west update --fetch-opt=--filter=tree:0
west zephyr-export
```

CI uses the `docker.io/zmkfirmware/zmk-build-arm:4.1` image. The dev container configuration lives
in `.devcontainer/`.

## Builds

Run firmware builds from the repo root and always pass `-s app`:

```sh
west build -s app -p -b nrf52840_dk
west build -s app -p -b nice_nano_v2 -- -DSHIELD="corne_left"
west build -s app -p -b nice_nano_v2 -S studio-rpc-usb-uart -- -DSHIELD="corne_left"
```

Build artifacts usually appear under `build/zephyr/`, for example `zmk.uf2`, `zmk.hex`, or
`zmk.bin` depending on the board.

## Tests

Use the local test runner for simulator tests:

```sh
./app/run-test.sh app/tests/led-matrix/basic
./app/run-test.sh app/tests/led-matrix
./app/run-test.sh all
```

To intentionally update a snapshot:

```sh
ZMK_TESTS_AUTO_ACCEPT=1 ./app/run-test.sh app/tests/led-matrix/<test-name>
```

The CI-style test entry point is run from `app/`:

```sh
west test tests/led-matrix
```

BLE-specific tests use `app/run-ble-test.sh` and require BabbleSim.

## Docs

Docs commands run from `docs/`:

```sh
npm ci
npm start
npm run build
npm run lint
npm run typecheck
npm run prettier:check
```

Use `npm run prettier:format` only when intentionally formatting docs.

## Formatting

- C and C++ follow `.clang-format`; use `clang-format -i <file>` only on files you intentionally
  touched.
- Docs, JS, JSON, YAML, and MDX use Prettier through the docs package.
- Board metadata formatting for `app/boards/**/*.yml` is managed by the `app/package.json`
  Prettier scripts.
- Pre-commit hooks are configured in `.pre-commit-config.yaml`.

## Worktree Safety

- This checkout may contain substantial uncommitted and untracked user work. Inspect
  `git status --short` before editing and never revert unrelated changes.
- At init time, the active dirty worktree included many LED Matrix files and generated build
  outputs. Work with those changes instead of discarding them.
- Treat `app/_bmkbuild/`, `.codex-corne54-left-buildsrc/`, `output/`, and `.claude/worktrees/` as
  generated or external unless the user explicitly asks to modify them.
- Keep edits tightly scoped to the user request and the LED Matrix feature area.
