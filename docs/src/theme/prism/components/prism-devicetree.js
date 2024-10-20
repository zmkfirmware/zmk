/**
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

/* eslint-disable no-undef */

Prism.languages.devicetree = {
  comment: Prism.languages.c["comment"],
  string: Prism.languages.c["string"],
  keyword:
    /\/(?:bits|delete-node|delete-property|dts-v1|incbin|include|memreserve|omit-if-no-ref|plugin)\//,
  label: {
    pattern: /\b(?:[a-z_]\w*):/i,
    alias: "symbol",
  },
  reference: {
    pattern: /&(?:[a-z_]\w*|\{[\w,.+*#?@/-]*\})/i,
    alias: "variable",
  },
  node: {
    pattern: /(?:\/|\b[\w,.+\-@]+)(?=\s*\{)/,
    alias: "class-name",
    inside: {
      // Node address
      number: {
        pattern: /(@)[0-9a-f,]/i,
        lookbehind: true,
      },
    },
  },
  function: Prism.languages.c["function"],
  "attr-name": /\\?[\w,.+*#?@-]+(?=\s*[=;])/,
  number: [/\b[0-9a-f]{2}\b/i, /\b(?:0[xX][0-9a-fA-F]+|\d+)(?:ULL|UL|LL|U|L)?/],
  macro: Prism.languages.c["macro"],
  operator: /<<|>>|[<>]=?|[!=]=?|&&?|\|\|?|[+\-*/%~?^]/,
  punctuation: /[{}[\];(),.]/,
};

Prism.languages.dts = Prism.languages.devicetree;
