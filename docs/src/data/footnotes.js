/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import macosLanguage from "../partials/footnotes/macos-language.mdx";
import macosInternational from "../partials/footnotes/macos-international.mdx";
import example from "../partials/footnotes/example.mdx";
import iosApplication from "../partials/footnotes/ios-application.mdx";
import iosPower from "../partials/footnotes/ios-power.mdx";
import macosEditing from "../partials/footnotes/macos-editing.mdx";
import macosPower from "../partials/footnotes/macos-power.mdx";
import macosUndoRedo from "../partials/footnotes/macos-undo-redo.mdx";
import globe from "../partials/footnotes/globe.mdx";

const footnotes = {
  macosLanguage,
  macosInternational,
  example,
  iosApplication,
  iosPower,
  macosEditing,
  macosPower,
  macosUndoRedo,
  globe,
};

export function getFootnote(id) {
  return footnotes[id];
}

export default footnotes;
