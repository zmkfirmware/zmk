---
title: Tests
sidebar_label: Tests
---

ZMK's tests build the firmware for a simulated board, feed it a
scripted sequence of virtual key events, and compare the resulting log output
against expected output (the "snapshot"). If the filtered log output
differs from the snapshot, the test fails.

There are two test suites, driven by two scripts in the `app` directory:

| Suite                                                    | Runner script                   | Target board                | Typical contents                  |
| -------------------------------------------------------- | ------------------------------- | --------------------------- | --------------------------------- |
| Regular tests                                            | `run-test.sh` (via `west test`) | `native_sim//zmk_test_mock` | behaviors, layers, keymaps        |
| BLE tests with [BabbleSim](https://babblesim.github.io/) | `run-ble-test.sh`               | `nrf52_bsim//zmk_test_mock` | profiles/bonding, split keyboards |

## Running the Tests

### Prerequisites

- Running the regular tests requires [native posix support](posix-board.md).

#### BLE Tests

BabbleSim is fetched via west as an optional module group, then built locally.
West checks it out into `tools/bsim` inside your west workspace (zmk root if you followed the [local setup](./setup/index.md) instructions):

```sh
west update --group-filter +babblesim

# From the root of your West workspace - use west topdir if uncertain
make -C tools/bsim everything
```

Then point the test runner at it with two environment variables:

```sh
export BSIM_OUT_PATH=/path/to/tools/bsim
export BSIM_COMPONENTS_PATH=/path/to/tools/bsim/components
```

If you are using the [ZMK devcontainer](./setup/container.mdx), this setup is handled for you: the
`.bashrc` sets `BSIM_OUT_PATH` and `BSIM_COMPONENTS_PATH` automatically once
`tools/bsim` exists.

### Running the Regular Tests

Tests should be run from within the `app` directory:

```sh
cd zmk/app
```

The regular test suite is run via the `west test` command:

```sh
# Runs all regular tests
west test
# Runs a particular regular test
west test tests/toggle-layer/normal
# Runs all tests for a particular test set
west test tests/toggle-layer
```

Build failures and log mismatches are reported as `FAILED`; otherwise the
test `PASS`es. Results for a full run are collected in
`app/build/tests/pass-fail.log`, and per-test artifacts (including
`build.log`, `keycode_events_full.log`, and `keycode_events.log`) are kept in
`app/build/tests/<testcase>/` for inspection. Some tests may be reported as `PENDING` — these are tests which fail, but for which the failure is currently accepted.

### Running the BLE Tests

Run `./run-ble-test.sh` from the `app` directory:

```sh
# A single test case
./run-ble-test.sh tests/ble/split/basic

# All BLE tests
./run-ble-test.sh all
```

On the first run (and whenever `tests/ble/central` changes), the runner builds
the central test app and copies the resulting executables into
`$BSIM_OUT_PATH/bin`.

## Writing Tests

### Regular Tests

Any folder under `app/tests` containing a `native_sim.keymap` file will be
picked up as a test case.

Each test case directory contains:

| File                      | Purpose                                                                                                                                              |
| ------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| `native_sim.keymap`       | The devicetree keymap for the test. Usually includes a shared `behavior_keymap.dtsi` from the test set directory and adds a list of mock key events. |
| `events.patterns`         | A [sed](https://www.gnu.org/software/sed/manual/sed.html) script that extracts the relevant lines from the test run's log output.                    |
| `keycode_events.snapshot` | The expected filtered log output.                                                                                                                    |
| `native_sim.conf`         | (Optional) Extra Kconfig fragments for the test.                                                                                                     |
| `extra-cmake-args`        | (Optional) Extra CMake arguments, one per line, passed to `west build`.                                                                              |
| `pending`                 | (Optional) Marks the test as "pending"; a mismatching snapshot is reported as `PENDING` instead of `FAILED`.                                         |

Mock key presses are defined on the `kscan` node using the
`ZMK_MOCK_PRESS(row, col, msec)` and `ZMK_MOCK_RELEASE(row, col, msec)`
macros from `dt-bindings/zmk/kscan_mock.h`, where `msec` is a delay that is
scheduled before that event fires:

```dts
&kscan {
    events =
        <ZMK_MOCK_PRESS(0,0,10)
        ZMK_MOCK_RELEASE(0,0,10)>;
};
```

Writing a new test or test set is best done by copying and modifying an existing one:

1. Copy the test set that most closely resembles the tests you will be creating.
2. Rename the newly created test set to the behavior you're testing e.g, toggle-layer
3. Modify `behavior_keymap.dtsi` to create a keymap using the behavior and related behaviors
4. Modify `test_case/native_sim.keymap` for a simulated use case
5. Modify `test_case/events.patterns` to collect relevant logs to the test
   - See: [sed manual](https://www.gnu.org/software/sed/manual/sed.html) and
     [tutorial](https://www.digitalocean.com/community/tutorials/the-basics-of-using-the-sed-stream-editor-to-manipulate-text-in-linux)
6. Modify `test_case/keycode_events.snapshot` to include the expected output
7. Rename the `test_case` folder to describe the test.
8. Repeat steps 4 to 7 for every test case

Instead of editing the snapshot file by hand, you can also ask the test script to generate it:

```sh
ZMK_TESTS_AUTO_ACCEPT=1 ./run-test.sh tests/none/normal
```

If you do so, please doublecheck and read it carefully to validate that the test is working as expected.

### BLE Tests

BLE tests exercise ZMK's Bluetooth stack by running the firmware inside
[BabbleSim](https://babblesim.github.io/), which simulates an nRF52 radio link
in software. Multiple simulated devices can participate in one simulation: the
ZMK device under test, plus "sibling" devices such as a simulated BLE host
(the `tests/ble/central` test app) or additional split keyboard halves.

BLE tests live under `app/tests/ble`:

- `central/` — the BLE central test application. This is a helper _used by_
  the other tests, not a test itself.
- `profiles/` — profile/bond management (`&bt` behaviors, profile selection,
  reconnects, etc.)
- `security/` — pairing/security behavior.
- `split/` — split keyboard behavior (central/peripheral communication,
  multiple peripherals, HID indicators, ...).

Each BLE test case directory contains:

| File                  | Purpose                                                                                                                                                                                                                         |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `nrf52_bsim.keymap`   | The devicetree keymap for the (primary) ZMK device under test, including mock key events.                                                                                                                                       |
| `nrf52_bsim.conf`     | (Optional) Extra Kconfig fragments for the test, e.g. `CONFIG_ZMK_SPLIT=y`.                                                                                                                                                     |
| `peripheral*.overlay` | (Optional) Devicetree overlays describing additional split peripherals. If present, the main device is built as the split central (`CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y`) and one image is built per overlay.                       |
| `siblings.txt`        | One line per additional simulated device. Each line is a command executed inside `$BSIM_OUT_PATH/bin`, e.g. `./ble_test_central.exe -d=2 -disconnect_and_reconnect`. The `-d=<n>` argument assigns the BabbleSim device number. |
| `events.patterns`     | An extended-regex sed script that extracts the relevant lines from the simulation's log output.                                                                                                                                 |
| `snapshot.log`        | The expected filtered log output.                                                                                                                                                                                               |
| `pending`             | (Optional) Marks the test as "pending"; a mismatching snapshot is reported as `PENDING` instead of `FAILED`.                                                                                                                    |

The simulated BLE host (`tests/ble/central`) scans, connects, pairs, and
subscribes to the ZMK device's HID reports, logging what it observes. It can
be steered with command line options in `siblings.txt`, including:

- `-disconnect_and_reconnect`
- `-halt_after_bonding`
- `-clear_bond_on_disconnect`
- `-skip_set_security_on_connect`
- `-read_hid_report_on_connect`
- `-subscribe_to_pointer_report`
- `-skip_discovery_on_connect`
- `-read_directly_on_discovery`
- `-write_hid_indicators_on_discovery`
- `-wait_on_start=<ms>`

(The full list is defined in `app/tests/ble/central/src/main.c`.)

To add BLE tests, follow the same process as for regular tests but name the keymap
`nrf52_bsim.keymap`, store the expected output in `snapshot.log`, and add a
`siblings.txt` file. When creating a split BLE test, add a
`peripheral*.overlay` file per peripheral describing its (mocked) key events,
and reference the resulting peripheral executables from `siblings.txt`.

Like with the regular tests, the snapshots for BLE tests can also be generated:

```sh
ZMK_TESTS_AUTO_ACCEPT=1 ./run-ble-test.sh tests/ble/split/basic
```
