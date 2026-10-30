/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import macosLanguage from "../components/footnotes/macos-language.mdx";
import macosInternational from "../components/footnotes/macos-international.mdx";
import example from "../components/footnotes/example.mdx";
import iosApplication from "../components/footnotes/ios-application.mdx";
import iosPower from "../components/footnotes/ios-power.mdx";
import macosEditing from "../components/footnotes/macos-editing.mdx";
import macosPower from "../components/footnotes/macos-power.mdx";
import macosUndoRedo from "../components/footnotes/macos-undo-redo.mdx";
import globe from "../components/footnotes/globe.mdx";

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
