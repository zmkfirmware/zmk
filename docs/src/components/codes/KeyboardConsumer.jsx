/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import Admonition from "@theme/Admonition";

export default function KeyboardConsumer() {
  return (
    <Admonition type="info" title="Keyboard vs. Consumer keycodes">
      <p>
        In the below tables, there are keycode pairs with similar names where
        one variant has a <code>K_</code> prefix and another <code>C_</code>.
        These variants correspond to similarly named usages from different{" "}
        <a href="https://usb.org/sites/default/files/hut1_2.pdf#page=16">
          HID usage pages
        </a>
        , namely the "keyboard/keypad" and "consumer" ones respectively.
      </p>
      <p>
        In practice, some OS and applications might listen to only one of the
        variants. You can use the values in the compatibility columns below to
        assist you in selecting which one to use.
      </p>
    </Admonition>
  );
}

KeyboardConsumer.propTypes = {};
