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
        // web-tree-sitter has a hard-coded path to tree-sitter.wasm,
        // (see https://github.com/tree-sitter/tree-sitter/issues/559)
        // which some browsers treat as absolute and others as relative.
        // This breaks everything. Rewrite it to always use an absolute path.
        rules.push({
          test: /tree-sitter\.js$/,
          loader: "string-replace-loader",
          options: {
            search: '"tree-sitter.wasm"',
            replace: '"/tree-sitter.wasm"',
            strict: true,
          },
        });
      }

      return {
        // web-tree-sitter tries to import "fs", which can be ignored.
        // https://github.com/tree-sitter/tree-sitter/issues/466
        node: {
          fs: "empty",
        },
        module: { rules },
      };
    },
  };
};
