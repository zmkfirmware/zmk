/**
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* eslint-disable no-undef */

Prism.languages.kconfig = {
  comment: {
    pattern: /(^|[^\\])#.*/,
    lookbehind: true,
    greedy: true,
  },
  string: /"(?:\\.|[^\\\r\n"])*"/,
  helptext: {
    // help text ends at the first line at a lower level of indentation than the
    // first line of text.
    pattern: /(^\s*)(?:help|---help---)\s*^(\s+)(?:.+)(?:\s*^\2[^\n]*)*/m,
    lookbehind: true,
    alias: "string",
    inside: {
      keyword: /^(?:help|---help---)/,
    },
  },
  keyword:
    /\b(?:allnoconfig_y|bool|boolean|choice|comment|config|def_bool|def_hex|def_int|def_string|def_tristate|default|defconfig_list|depends|endchoice|endif|endmenu|env|hex|if|imply|int|mainmenu|menu|menuconfig|modules|on|option|optional|orsource|osource|prompt|range|rsource|select|source|string|tristate|visible)\b/,
  expansion: {
    pattern: /\$\([\s\S]+\)/,
    alias: "variable",
    inside: {
      function: /\$\(|\)/,
      punctuation: /,/,
    },
  },
  number: /\b(?:0[xX][0-9a-fA-F]+|\d+)/,
  boolean: {
    pattern: /\b(?:y|n|m)\b/,
    alias: "number",
  },
  variable: /\b[A-Z_]+\b/,
  operator: /[<>]=?|[!=]=?|&&|\|\|/,
  punctuation: /[()]/,
};
