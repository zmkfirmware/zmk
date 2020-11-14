---
title: Tests
sidebar_label: Tests
---

Running tests requires [native posix support](posix-board). Any folder under `/app/tests`
containing `native_posix.keymap` will be selected when running `west test`.

Run a single test with `west test <testname>`, like `west test tests/toggle-layer/normal`.

## Creating a New Test Set

1. Copy the test set that most closely resembles the tests you will be creating.
2. Rename the newly created test set to the behavior you're testing e.g, toggle-layer
3. Modify `behavior_keymap.dtsi` to create a keymap using the behavior and related behaviors
4. Modify `test_case/native_posix.keymap` for a simulated use case
5. Modify `test_case/events.patterns` to collect relevant logs to the test
   - See: [sed manual](https://www.gnu.org/software/sed/manual/sed.html) and
     [tutorial](https://www.digitalocean.com/community/tutorials/the-basics-of-using-the-sed-stream-editor-to-manipulate-text-in-linux)
6. Modify `test_case/keycode_events.snapshot` for to include the expected output
7. Rename the `test_case` folder to describe the test.
8. Repeat steps 4 to 7 for every test case
