# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ZMK (Zephyr Mechanical Keyboard) Firmware is an open source keyboard firmware built on the Zephyr RTOS. It provides modern, wireless keyboard support free of licensing issues.

**Key Architecture:**
- Built on Zephyr RTOS (v4.1.0+zmk-fixes)
- Uses `west` (Zephyr's meta-tool) for build and dependency management
- West workspace structure: `app/` contains the main ZMK application, `zephyr/` is the Zephyr RTOS, `modules/` contains additional dependencies
- Configuration via Kconfig (Zephyr's configuration system) and devicetree (DTS) files

## Repository Structure

- `app/` - Main ZMK firmware application
  - `src/` - Core firmware source code (behaviors, HID, keymap, BLE, etc.)
  - `boards/` - Board definitions and shield configurations (as `.yml` files)
  - `tests/` - Native simulation tests (one per feature/behavior)
  - `dts/` - Devicetree source files
  - `include/` - Public headers
  - `CMakeLists.txt` - Main build configuration
  - `Kconfig`, `Kconfig.behaviors` - Kconfig symbols
- `docs/` - Documentation (Docusaurus site)
- `modules/` - Additional Zephyr modules (HAL, libraries, etc.)
- `zephyr/` - Zephyr RTOS (managed by west)
- `schema/` - JSON schemas for board/shield configuration validation

## Common Commands

### Building Firmware

ZMK uses the `west` build system. From the repository root:

```bash
# Build for a specific board (replace with your board)
west build -b <board> -- -DSHIELD=<shield>

# Example: build for corneish keyboard
west build -b nice_nano_v2 -- -DSHIELD=corneish_left

# Clean build
west build -b <board> -p -- -DSHIELD=<shield>
```

### Running Tests

Tests are run using native simulation (native_sim board):

```bash
# From app/ directory
./run-test.sh <path/to/testcase>
./run-test.sh all                    # Run all tests
./run-test.sh tests/hold-tap         # Run specific test directory

# Run tests in parallel (default 4 jobs)
J=8 ./run-test.sh all

# Auto-accept new test snapshots (for updating tests)
ZMK_TESTS_AUTO_ACCEPT=1 ./run-test.sh <path>

# Run BLE tests
./run-ble-test.sh <path/to/testcase>
```

Each test case has a `native_sim.keymap` file, `events.patterns` file, and `keycode_events.snapshot` for validation.

### Code Formatting

```bash
# Format C/C++ code (clang-format)
clang-format -i <files>

# Format board YAML files
cd app && npm run prettier:format

# Format documentation
cd docs && npm run prettier:format
```

### Linting

```bash
# Lint documentation
cd docs && npm run lint

# Check formatting without applying changes
cd app && npm run prettier:check
cd docs && npm run prettier:check
```

### Pre-commit Hooks

Install pre-commit hooks for automatic formatting:

```bash
pip3 install pre-commit
pre-commit install
```

The `.pre-commit-config.yaml` includes: clang-format, prettier, remove-tabs, gitlint, and other checks.

### Documentation

```bash
# From docs/ directory
npm ci           # Install dependencies
npm start        # Start dev server (http://0.0.0.0:3000)
npm run build    # Build static site
npm run typecheck  # TypeScript type checking
```

## Architecture Notes

### Behaviors System
ZMK's behaviors are modular key handlers in `app/src/behaviors/`. Each behavior implements a standard interface and is registered via Kconfig. Key behaviors include:
- `behavior_key_press.c` - Basic key press
- `behavior_hold_tap.c` - Hold/tap modulation
- `behavior_layer_*.c` - Layer switching
- `behavior_sticky_key.c` - Sticky keys
- `behavior_macro.c` - Macro execution

### Event System
ZMK uses an event-driven architecture with events in `app/src/events/`. The event manager (`event_manager.c`) coordinates event propagation between components.

### Split Keyboards
Split keyboard support is in `app/src/split/`. The central role processes all key events and sends state updates to peripheral nodes.

### HID Subsystem
HID functionality spans multiple files:
- `hid.c` - Core HID state management
- `hid_listener.c` - Event listener for HID state changes
- `usb_hid.c` - USB HID output
- `hog.c` - Bluetooth HID over GATT (HOG)

### Keymap and Physical Layouts
- `keymap.c` - Keymap layer management and behavior lookup
- `physical_layouts.c` - Physical-to-logical key position mapping
- `matrix_transform.c` - Matrix scan to position transformation

## Kconfig System

ZMK heavily uses Kconfig for feature selection. Important files:
- `app/Kconfig` - Main configuration symbols
- `app/Kconfig.behaviors` - Behavior-specific symbols
- `app/Kconfig.defaults` - Default configurations

Configuration is done via `app/prj.conf` or board-specific overlays.

## Devicetree

Hardware configuration uses Zephyr devicetree:
- Board-specific files in `app/dts/boards/`
- Shield definitions in `app/boards/shields/`
- Key bindings use devicetree property syntax

## Testing Philosophy

Tests are snapshot-based: each test defines a key sequence and expected HID output events. Tests run under native simulation (POSIX) and can be executed without hardware.
