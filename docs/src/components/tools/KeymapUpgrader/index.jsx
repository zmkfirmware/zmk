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
} from "../../../keymap-upgrade";

import styles from "./styles.module.css";

export default function KeymapUpgrader() {
  const { error, isPending } = useAsync(initParser);

  if (isPending) {
    return <p className="not-content">Loading...</p>;
  }

  if (error) {
    return <p className="error not-content">Error: {error.message}</p>;
  }

  return <Editor />;
}

function Editor() {
  const [keymap, setKeymap] = React.useState("");

  const { text: upgraded, changedRanges } = upgradeKeymap(keymap);
  const highlights = rangesToLineNumbers(upgraded, changedRanges);
  const highlightedLines = parseHighlightedLines(highlights);

  let title = "Upgraded Keymap";

  if (keymap && upgraded === keymap) {
    title += " (No Changes)";
  }

  const lines = upgraded.split("\n");

  return (
    <div className="not-content">
      <textarea
        className={styles.editor}
        placeholder="Paste keymap here"
        spellCheck={false}
        value={keymap}
        onChange={(e) => setKeymap(e.target.value)}
      ></textarea>
      <div className={styles.result}>
        <div className={styles.resultTitle}>{title}</div>
        <div className="keymap-upgrader-result">
          <pre>
            <code>
              {lines.map((line, index) => (
                <React.Fragment key={index}>
                  <span
                    className={
                      highlightedLines.has(index + 1)
                        ? `${styles.highlighted} highlighted`
                        : undefined
                    }
                  >
                    {line}
                  </span>
                  {index < lines.length - 1 ? "\n" : null}
                </React.Fragment>
              ))}
            </code>
          </pre>
        </div>
      </div>
    </div>
  );
}

/**
 * Parses the line number list returned by rangesToLineNumbers(), which has the
 * format "{1,2-3}", into a set of line numbers.
 */
function parseHighlightedLines(highlights) {
  const lines = new Set();
  const body = highlights.replace(/[{}]/g, "").trim();

  if (!body) {
    return lines;
  }

  for (const part of body.split(",")) {
    const [startText, endText] = part.split("-");
    const start = Number.parseInt(startText, 10);
    const end = endText === undefined ? start : Number.parseInt(endText, 10);

    if (!Number.isFinite(start) || !Number.isFinite(end)) {
      continue;
    }

    const first = Math.min(start, end);
    const last = Math.max(start, end);

    for (let line = first; line <= last; line++) {
      lines.add(line);
    }
  }

  return lines;
}
