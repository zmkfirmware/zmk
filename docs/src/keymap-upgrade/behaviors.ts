import type { Tree } from "web-tree-sitter";

import { Devicetree, findCapture } from "./parser";
import { TextEdit, getUpgradeEdits } from "./textedit";

// Map of { "deprecated": "replacement" } behavior names (not including "&" prefixes).
const BEHAVIORS = {
  cp: "kp",
  inc_dec_cp: "inc_dec_kp",
  reset: "sys_reset",
};

export function upgradeBehaviors(tree: Tree) {
  const edits: TextEdit[] = [];

  const query = Devicetree.query("(reference label: (identifier) @ref)");
  const matches = query.matches(tree.rootNode);

  for (const { captures } of matches) {
    const node = findCapture("ref", captures);
    if (node) {
      edits.push(...getUpgradeEdits(node, BEHAVIORS));
    }
  }

  return edits;
}
