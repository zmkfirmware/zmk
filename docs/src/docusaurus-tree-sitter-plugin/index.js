/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

module.exports = function () {
  return {
    configureWebpack(config, isServer) {
      let rules = [];

      // Tree-sitter is only used for client-side code.
      // Don't try to load it on the server.
      if (isServer) {
        rules.push({
          test: /web-tree-sitter/,
          loader: "null-loader",
        });
      }

      return {
        // web-tree-sitter tries to import "fs", which can be ignored.
        // https://github.com/tree-sitter/tree-sitter/issues/466
        resolve: {
          fallback: {
            fs: false,
            path: false,
          },
        },
        module: { rules },
      };
    },
  };
};
