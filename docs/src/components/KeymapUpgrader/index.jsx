/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import React from "react";
import { useAsync } from "react-async";

import {
  initParser,
  upgradeKeymap,
  rangesToLineNumbers,
} from "@site/src/keymap-upgrade";
import CodeBlock from "@theme/CodeBlock";

import styles from "./styles.module.css";

export default function KeymapUpgrader() {
  const { error, isPending } = useAsync(initParser);

  if (isPending) {
    return <p>Loading...</p>;
  }

  if (error) {
    return <p className="error">Error: {error.message}</p>;
  }

  return <Editor />;
}

function Editor() {
  const [keymap, setKeymap] = React.useState("");

  const { text: upgraded, changedRanges } = upgradeKeymap(keymap);
  const highlights = rangesToLineNumbers(upgraded, changedRanges);

  let title = "Upgraded Keymap";

  if (keymap && upgraded === keymap) {
    title += " (No Changes)";
  }

  return (
    <div>
      <textarea
        className={styles.editor}
        placeholder="Paste keymap here"
        spellCheck={false}
        value={keymap}
        onChange={(e) => setKeymap(e.target.value)}
      ></textarea>
      <div className={styles.result}>
        <CodeBlock language="dts" metastring={`${highlights} title="${title}"`}>
          {upgraded}
        </CodeBlock>
      </div>
    </div>
  );
}
