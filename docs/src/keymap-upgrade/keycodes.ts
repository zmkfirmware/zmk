import type { SyntaxNode, Tree } from "web-tree-sitter";
import { Devicetree, findCapture } from "./parser";
import { getUpgradeEdits, TextEdit } from "./textedit";

// Map of { "DEPRECATED": "REPLACEMENT" } key codes.
const CODES = {
  NUM_1: "N1",
  NUM_2: "N2",
  NUM_3: "N3",
  NUM_4: "N4",
  NUM_5: "N5",
  NUM_6: "N6",
  NUM_7: "N7",
  NUM_8: "N8",
  NUM_9: "N9",
  NUM_0: "N0",
  BKSP: "BSPC",
  SPC: "SPACE",
  EQL: "EQUAL",
  TILD: "TILDE",
  SCLN: "SEMI",
  QUOT: "SQT",
  GRAV: "GRAVE",
  CMMA: "COMMA",
  PRSC: "PSCRN",
  SCLK: "SLCK",
  PAUS: "PAUSE_BREAK",
  PGUP: "PG_UP",
  PGDN: "PG_DN",
  RARW: "RIGHT",
  LARW: "LEFT",
  DARW: "DOWN",
  UARW: "UP",
  KDIV: "KP_DIVIDE",
  KMLT: "KP_MULTIPLY",
  KMIN: "KP_MINUS",
  KPLS: "KP_PLUS",
  UNDO: "K_UNDO",
  CUT: "K_CUT",
  COPY: "K_COPY",
  PSTE: "K_PASTE",
  VOLU: "K_VOL_UP",
  VOLD: "K_VOL_DN",
  CURU: "DLLR",
  LPRN: "LPAR",
  RPRN: "RPAR",
  LCUR: "LBRC",
  RCUR: "RBRC",
  CRRT: "CARET",
  PRCT: "PRCNT",
  LABT: "LT",
  RABT: "GT",
  COLN: "COLON",
  KSPC: null,
  ATSN: "AT",
  BANG: "EXCL",
  LCTL: "LCTRL",
  LSFT: "LSHIFT",
  RCTL: "RCTRL",
  RSFT: "RSHIFT",
  M_NEXT: "C_NEXT",
  M_PREV: "C_PREV",
  M_STOP: "C_STOP",
  M_EJCT: "C_EJECT",
  M_PLAY: "C_PP",
  M_MUTE: "C_MUTE",
  M_VOLU: "C_VOL_UP",
  M_VOLD: "C_VOL_DN",
  GUI: "K_CMENU",
  MOD_LCTL: "LCTRL",
  MOD_LSFT: "LSHIFT",
  MOD_LALT: "LALT",
  MOD_LGUI: "LGUI",
  MOD_RCTL: "RCTRL",
  MOD_RSFT: "RSHIFT",
  MOD_RALT: "RALT",
  MOD_RGUI: "RGUI",
};

// Regex matching names of properties that can have keymap bindings.
const BINDINGS_PROPS = /^(bindings|sensor-bindings)$/;

/**
 * Upgrades deprecated key code identifiers.
 */
export function upgradeKeycodes(tree: Tree) {
  const edits: TextEdit[] = [];

  const query = Devicetree.query("(identifier) @name");
  const matches = query.matches(tree.rootNode);

  for (const { captures } of matches) {
    const node = findCapture("name", captures);

    // Some of the codes are still valid to use in other properties such as
    // "mods", so only replace those that are inside a "bindings" array.
    if (node && isInBindingsArray(node)) {
      edits.push(...getUpgradeEdits(node, CODES, keycodeReplaceHandler));
    }
  }

  return edits;
}

function keycodeReplaceHandler(node: SyntaxNode, replacement: string | null) {
  if (replacement) {
    return [TextEdit.fromNode(node, replacement)];
  }

  const nodes = findBehaviorNodes(node);

  if (nodes.length === 0) {
    console.warn(
      `Found deprecated code "${node.text}" but it is not a parameter to a behavior`
    );
    return [TextEdit.fromNode(node, `/* "${node.text}" no longer exists */`)];
  }

  const oldText = nodes.map((n) => n.text).join(" ");
  const newText = `&none /* "${oldText}" no longer exists */`;

  const startIndex = nodes[0].startIndex;
  const endIndex = nodes[nodes.length - 1].endIndex;

  return [new TextEdit(startIndex, endIndex, newText)];
}

/**
 * Given a parameter to a keymap behavior, returns a list of nodes beginning
 * with the behavior and including all parameters.
 * Returns an empty array if no behavior was found.
 */
function findBehaviorNodes(paramNode: SyntaxNode) {
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
 * Given a identifier, returns whether it is inside a "bindings" property value.
 */
function isInBindingsArray(identifier: SyntaxNode) {
  let node = identifier.parent;
  while (node) {
    if (node.type === "property") {
      return !!node.childForFieldName("name")?.text.match(BINDINGS_PROPS);
    }

    node = node.parent;
  }

  return false;
}
