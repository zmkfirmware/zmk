/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import footnotes from "./data/footnotes";

export function getFootnote(id) {
  const footnote = footnotes[id];
  if (typeof footnote != "undefined") {
    return footnote;
  }
}
