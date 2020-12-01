/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import React from "react";
import PropTypes from "prop-types";
import TableRow from "./TableRow";
import Footnotes from "./Footnotes";
import LinkIcon from "./LinkIcon";
import operatingSystems from "@site/src/data/operating-systems";
import { getCodes } from "@site/src/hid";
import { getGroup } from "@site/src/groups";
import { getFootnote } from "@site/src/footnotes";

function extractFootnoteIds(codes) {
  return Array.from(
    new Set(
      codes
        .flatMap(({ footnotes }) => Object.values(footnotes))
        .flatMap((refs) => (Array.isArray(refs) ? refs.flat() : refs))
    )
  );
}

export default function Table({ group }) {
  const codes = getCodes(getGroup(group));

  const footnotesAnchor = group + "-" + "footnotes";

  const tableFootnotes = extractFootnoteIds(codes).map((id, i) => {
    const Component = getFootnote(id);
    return {
      id,
      anchor: footnotesAnchor,
      symbol: i + 1,
      value: Component ? <Component /> : undefined,
    };
  });

  return (
    <div className="codes">
      <table>
        <thead>
          <tr>
            <th className="names">Names</th>
            <th className="description">Description</th>
            <th className="documentation" title="Documentation">
              <LinkIcon />
            </th>
            {operatingSystems.map(({ key, className, heading, title }) => (
              <th key={key} className={`os ${className}`} title={title}>
                {heading}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {Array.isArray(codes)
            ? codes.map((code) => (
                <TableRow
                  key={code.names[0]}
                  {...code}
                  tableFootnotes={tableFootnotes}
                />
              ))
            : undefined}
        </tbody>
      </table>
      {tableFootnotes.length > 0 ? (
        <Footnotes id={footnotesAnchor} footnotes={tableFootnotes} />
      ) : undefined}
    </div>
  );
}

Table.propTypes = {
  group: PropTypes.string.isRequired,
};
