import type { SyntaxNode, Tree } from "web-tree-sitter";
import { captureHasText, Devicetree, findCapture } from "./parser";
import { TextEdit } from "./textedit";

/**
 * Upgrades deprecated properties.
 */
export function upgradeProperties(tree: Tree) {
  return removeLabels(tree);
}

/**
 * Renames "label" properties in keymap layers to "display-name". Removes all
 * other "label" properties.
 */
function removeLabels(tree: Tree) {
  const edits: TextEdit[] = [];

  const query = Devicetree.query("(property name: (identifier) @name) @prop");
  const matches = query.matches(tree.rootNode);

  for (const { captures } of matches) {
    const name = findCapture("name", captures);
    const node = findCapture("prop", captures);
    if (name?.text === "label" && node) {
      if (isLayerLabel(node)) {
        edits.push(TextEdit.fromNode(name, "display-name"));
      } else {
        edits.push(TextEdit.fromNode(node, ""));
      }
    }
  }

  return edits;
}

/**
 * Given a "label" property node, returns whether it is a label for a keymap layer.
 */
function isLayerLabel(node: SyntaxNode) {
  const maybeKeymap = node.parent?.parent;
  if (!maybeKeymap) {
    return false;
  }

  const query = Devicetree.query(
    `(property
        name: (identifier) @name
        value: (string_literal) @value
      ) @prop`
  );
  const matches = query.matches(maybeKeymap);

  for (const { captures } of matches) {
    if (
      findCapture("prop", captures)?.parent?.equals(maybeKeymap) &&
      captureHasText("name", captures, "compatible") &&
      captureHasText("value", captures, '"zmk,keymap"')
    ) {
      return true;
    }
  }

  return false;
}
