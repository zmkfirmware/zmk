/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import Admonition from "@theme/Admonition";

export default function SpellingCaution() {
  return (
    <Admonition type="warning">
      <p>
        Take extra notice of the spelling of the keycodes, especially the
        shorthand spelling. Otherwise, it will result in an elusive parsing
        error!
      </p>
    </Admonition>
  );
}

SpellingCaution.propTypes = {};
