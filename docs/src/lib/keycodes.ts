/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import { getFootnote } from "@data/footnotes.js";
import groups from "@data/groups.js";
import codesData from "@data/hid.js";
import operatingSystems from "@data/operating-systems.js";

export interface CodeEntry {
  names: string[];
  description: string;
  context?: string;
  clarify?: boolean;
  documentation: string;
  os: Record<string, boolean | null>;
  footnotes?: Record<string, string | string[] | undefined>;
}

export interface KeycodeFootnote {
  id: string;
  anchor: string;
  symbol: number;
  value?: unknown;
}

export interface KeycodeRow {
  names: string[];
  description: string;
  context: string;
  clarify: boolean;
  documentation: string;
  os: Record<string, boolean | null>;
  footnotesByOs: Record<string, KeycodeFootnote[]>;
}

const codeMap = new Map<string, CodeEntry>();
for (const code of codesData as unknown as CodeEntry[]) {
  for (const name of code.names) {
    codeMap.set(name, code);
  }
}

const groupMap = groups as unknown as Record<string, string[] | undefined>;

export function getKeycodeTable(group: string) {
  const groupCodes = groupMap[group];
  if (!groupCodes) {
    console.warn(`Unknown keycode group: ${group}`);
  }

  const codes = (groupCodes ?? [])
    .map((id) => codeMap.get(id))
    .filter((code): code is CodeEntry => code !== undefined);

  const footnotesAnchor = `${group}-footnotes`;

  // Collect every footnote referenced by at least one code in this group, in
  // first-seen order, and number them for this table.
  const footnoteIds = Array.from(
    new Set(
      codes
        .flatMap(({ footnotes }) => Object.values(footnotes ?? {}))
        .flatMap((refs) => (Array.isArray(refs) ? refs.flat() : refs))
        .filter((id): id is string => typeof id === "string")
    )
  );

  const footnotes: KeycodeFootnote[] = footnoteIds.map((id, index) => ({
    id,
    anchor: `${footnotesAnchor}-${id}`,
    symbol: index + 1,
    value: getFootnote(id),
  }));

  const rows: KeycodeRow[] = codes.map((code) => {
    const footnotesByOs = Object.fromEntries(
      operatingSystems.map(({ key }) => [
        key,
        footnotes.filter(({ id }) => {
          const refs = code.footnotes?.[key];
          return (Array.isArray(refs) && refs.includes(id)) || refs === id;
        }),
      ])
    );

    return {
      names: code.names,
      description: code.description,
      context: code.context ?? "",
      clarify: code.clarify ?? false,
      documentation: code.documentation,
      os: code.os,
      footnotesByOs,
    };
  });

  return { rows, footnotes, footnotesAnchor };
}
