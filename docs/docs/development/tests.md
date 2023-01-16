---
title: Tests
sidebar_label: Tests
---

- Running tests requires [native posix support](posix-board.md).
- Any folder under `/app/tests` containing `native_posix_64.keymap` will be selected when running `west test`.

## Running Tests

All the following commands assume you have a terminal opened to the `zmk/app` directory.

First, make sure all Python dependencies are installed:

```sh
python3 -m pip install -r scripts/requirements.txt
```

Tests can then be run from Zephyr's [west](https://docs.zephyrproject.org/3.2.0/develop/west/index.html) tool with the `test` subcommand:

```sh
west test
```

Running it with no arguments will run the entire test suite. You can run a single test case or test set by providing the relative path from `zmk/app/tests` to the test directory, e.g.

```sh
# Run all tests cases in the toggle-layer set
west test toggle-layer

# Run the toggle-layer/normal test case
west test toggle-layer/normal
```

Any additional arguments are passed through to [PyTest](https://docs.pytest.org/).

## Creating a New Test Set

1. Copy the test set that most closely resembles the tests you will be creating.
2. Rename the newly created test set to the behavior you're testing e.g, toggle-layer
3. Modify `behavior_keymap.dtsi` to create a keymap using the behavior and related behaviors
4. Modify `test_case/native_posix_64.keymap` for a simulated use case
5. Modify `test_case/events.patterns` to collect relevant logs to the test
   - See: [sed manual](https://www.gnu.org/software/sed/manual/sed.html) and
     [tutorial](https://www.digitalocean.com/community/tutorials/the-basics-of-using-the-sed-stream-editor-to-manipulate-text-in-linux)
6. Modify `test_case/keycode_events.snapshot` for to include the expected output
7. Rename the `test_case` folder to describe the test.
8. Repeat steps 4 to 7 for every test case
