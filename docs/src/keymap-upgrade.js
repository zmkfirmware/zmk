import Parser from "web-tree-sitter";

import { Codes, Behaviors } from "./data/keymap-upgrade";

let Devicetree;

export async function initParser() {
  await Parser.init();
  Devicetree = await Parser.Language.load("/tree-sitter-devicetree.wasm");
}

function createParser() {
  if (!Devicetree) {
    throw new Error("Parser not loaded. Call initParser() first.");
  }

  const parser = new Parser();
  parser.setLanguage(Devicetree);
  return parser;
}

export function upgradeKeymap(text) {
  const parser = createParser();
  const tree = parser.parse(text);

  const edits = [...upgradeBehaviors(tree), ...upgradeKeycodes(tree)];

  return applyEdits(text, edits);
}

class TextEdit {
  /**
   * Creates a text edit to replace a range or node with new text.
   * Construct with one of:
   *
   * * `Edit(startIndex, endIndex, newText)`
   * * `Edit(node, newText)`
   */
  constructor(startIndex, endIndex, newText) {
    if (typeof startIndex !== "number") {
      const node = startIndex;
      newText = endIndex;
      startIndex = node.startIndex;
      endIndex = node.endIndex;
    }

    /** @type number */
    this.startIndex = startIndex;
    /** @type number */
    this.endIndex = endIndex;
    /** @type string */
    this.newText = newText;
  }
}

/**
 * Upgrades deprecated behavior references.
 * @param {Parser.Tree} tree
 */
function upgradeBehaviors(tree) {
  /** @type TextEdit[] */
  let edits = [];

  const query = Devicetree.query("(reference label: (identifier) @ref)");
  const matches = query.matches(tree.rootNode);

  for (const { captures } of matches) {
    const node = findCapture("ref", captures);
    if (node) {
      edits.push(...getUpgradeEdits(node, Behaviors));
    }
  }

  return edits;
}

/**
 * Upgrades deprecated key code identifiers.
 * @param {Parser.Tree} tree
 */
function upgradeKeycodes(tree) {
  /** @type TextEdit[] */
  let edits = [];

  // No need to filter to the bindings array. The C preprocessor would have
  // replaced identifiers anywhere, so upgrading all identifiers preserves the
  // original behavior of the keymap (even if that behavior wasn't intended).
  const query = Devicetree.query("(identifier) @name");
  const matches = query.matches(tree.rootNode);

  for (const { captures } of matches) {
    const node = findCapture("name", captures);
    if (node) {
      edits.push(...getUpgradeEdits(node, Codes, keycodeReplaceHandler));
    }
  }

  return edits;
}

/**
 * @param {Parser.SyntaxNode} node
 * @param {string | null} replacement
 * @returns TextEdit[]
 */
function keycodeReplaceHandler(node, replacement) {
  if (replacement) {
    return [new TextEdit(node, replacement)];
  }

  const nodes = findBehaviorNodes(node);

  if (nodes.length === 0) {
    console.warn(
      `Found deprecated code "${node.text}" but it is not a parameter to a behavior`
    );
    return [new TextEdit(node, `/* "${node.text}" no longer exists */`)];
  }

  const oldText = nodes.map((n) => n.text).join(" ");
  const newText = `&none /* "${oldText}" no longer exists */`;

  const startIndex = nodes[0].startIndex;
  const endIndex = nodes[nodes.length - 1].endIndex;

  return [new TextEdit(startIndex, endIndex, newText)];
}

/**
 * Returns the node for the named capture.
 * @param {string} name
 * @param {any[]} captures
 * @returns {Parser.SyntaxNode | null}
 */
function findCapture(name, captures) {
  for (const c of captures) {
    if (c.name === name) {
      return c.node;
    }
  }

  return null;
}

/**
 * Given a parameter to a keymap behavior, returns a list of nodes beginning
 * with the behavior and including all parameters.
 * Returns an empty array if no behavior was found.
 * @param {Parser.SyntaxNode} paramNode
 */
function findBehaviorNodes(paramNode) {
  // Walk backwards from the given parameter to find the behavior reference.
  let behavior = paramNode.previousNamedSibling;
  while (behavior && behavior.type !== "reference") {
    behavior = behavior.previousNamedSibling;
  }

  if (!behavior) {
    return [];
  }

  // Walk forward from the behavior to collect all its parameters.

  let nodes = [behavior];
  let param = behavior.nextNamedSibling;
  while (param && param.type !== "reference") {
    nodes.push(param);
    param = param.nextNamedSibling;
  }

  return nodes;
}

/**
 * Gets a list of text edits to apply based on a node and a map of text
 * replacements.
 *
 * If replaceHandler is given, it will be called if the node matches a
 * deprecated value and it should return the text edits to apply.
 *
 * @param {Parser.SyntaxNode} node
 * @param {Map<string, string | null>} replacementMap
 * @param {(node: Parser.SyntaxNode, replacement: string | null) => TextEdit[]} replaceHandler
 */
function getUpgradeEdits(node, replacementMap, replaceHandler = undefined) {
  for (const [deprecated, replacement] of Object.entries(replacementMap)) {
    if (node.text === deprecated) {
      if (replaceHandler) {
        return replaceHandler(node, replacement);
      } else {
        return [new TextEdit(node, replacement)];
      }
    }
  }
  return [];
}

/**
 * Sorts a list of text edits in ascending order by position.
 * @param {TextEdit[]} edits
 */
function sortEdits(edits) {
  return edits.sort((a, b) => a.startIndex - b.startIndex);
}

/**
 * Returns a string with text replacements applied.
 * @param {string} text
 * @param {TextEdit[]} edits
 */
function applyEdits(text, edits) {
  edits = sortEdits(edits);

  /** @type string[] */
  const chunks = [];
  let currentIndex = 0;

  for (const edit of edits) {
    if (edit.startIndex < currentIndex) {
      console.warn("discarding overlapping edit", edit);
      continue;
    }

    chunks.push(text.substring(currentIndex, edit.startIndex));
    chunks.push(edit.newText);
    currentIndex = edit.endIndex;
  }

  chunks.push(text.substring(currentIndex));

  return chunks.join("");
}
