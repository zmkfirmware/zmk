import type { SyntaxNode, Tree } from "web-tree-sitter";
import { Devicetree, findCapture } from "./parser";
import { getUpgradeEdits, MatchFunc, ReplaceFunc, TextEdit } from "./textedit";

// Map of { "deprecated": "replacement" } header paths.
const HEADERS = {
  "dt-bindings/zmk/matrix-transform.h": "dt-bindings/zmk/matrix_transform.h",
};

export function upgradeHeaders(tree: Tree) {
  const edits: TextEdit[] = [];

  const query = Devicetree.query(
    "(preproc_include path: [(string_literal) (system_lib_string)] @path)"
  );
  const matches = query.matches(tree.rootNode);

  for (const { captures } of matches) {
    const node = findCapture("path", captures);
    if (node) {
      edits.push(
        ...getUpgradeEdits(node, HEADERS, headerReplaceHandler, isHeaderMatch)
      );
    }
  }

  return edits;
}

const isHeaderMatch: MatchFunc = (node, text) => {
  return node.text === `"${text}"` || node.text === `<${text}>`;
};

const headerReplaceHandler: ReplaceFunc = (node, replacement) => {
  if (!replacement) {
    throw new Error("Header replacement does not support removing headers");
  }

  return [new TextEdit(node.startIndex + 1, node.endIndex - 1, replacement)];
};
