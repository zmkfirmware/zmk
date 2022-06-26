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
      } else {
        // The way web-tree-sitter loads tree-sitter.wasm isn't something that
        // Docusaurus/Webpack identify as an asset. There is currently no way to
        // set location of the file other than patching web-tree-sitter.
        // (see https://github.com/tree-sitter/tree-sitter/issues/559)
        rules.push({
          test: /tree-sitter\.js$/,
          loader: "string-replace-loader",
          options: {
            multiple: [
              // Replace the path to tree-sitter.wasm with a "new URL()" to clue
              // Webpack in that it is an asset.
              {
                search: '"tree-sitter.wasm"',
                replace: '(new URL("tree-sitter.wasm", import.meta.url)).href',
                strict: true,
              },
              // Webpack replaces "new URL()" with the full URL to the asset, but
              // web-tree-sitter will still add a prefix to it unless there is a
              // Module.locateFile() function.
              {
                search: "var Module=void 0!==Module?Module:{};",
                replace: `var Module = {
                  locateFile: (path, prefix) => path.startsWith('http') ? path : prefix + path,
                };`,
                strict: true,
              },
            ],
          },
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
