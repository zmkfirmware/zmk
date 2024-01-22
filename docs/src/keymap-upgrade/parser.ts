import Parser from "web-tree-sitter";

const TREE_SITTER_WASM_URL = new URL(
  "/node_modules/web-tree-sitter/tree-sitter.wasm",
  import.meta.url
);

export let Devicetree: Parser.Language;

export async function initParser() {
  await Parser.init({
    locateFile: (path: string, prefix: string) => {
      // When locating tree-sitter.wasm, use a path that Webpack can map to the correct URL.
      if (path == "tree-sitter.wasm") {
        return TREE_SITTER_WASM_URL.href;
      }
      return prefix + path;
    },
  });
  Devicetree = await Parser.Language.load("/tree-sitter-devicetree.wasm");
}

export function createParser() {
  if (!Devicetree) {
    throw new Error("Parser not loaded. Call initParser() first.");
  }

  const parser = new Parser();
  parser.setLanguage(Devicetree);
  return parser;
}

/**
 * Returns the node for the named capture.
 */
export function findCapture(name: string, captures: Parser.QueryCapture[]) {
  for (const c of captures) {
    if (c.name === name) {
      return c.node;
    }
  }

  return null;
}

/**
 * Returns whether the node for the named capture exists and has the given text.
 */
export function captureHasText(
  name: string,
  captures: Parser.QueryCapture[],
  text: string
) {
  const node = findCapture(name, captures);
  return node?.text === text;
}

/**
 * Get a list of SyntaxNodes representing a devicetree node with the given path.
 * (The same node may be listed multiple times within a file.)
 *
 * @param path Absolute path to the node (must start with "/")
 */
export function findDevicetreeNode(
  tree: Parser.Tree,
  path: string
): Parser.SyntaxNode[] {
  const query = Devicetree.query("(node) @node");
  const matches = query.matches(tree.rootNode);

  const result: Parser.SyntaxNode[] = [];

  for (const { captures } of matches) {
    const node = findCapture("node", captures);

    if (node && getDevicetreeNodePath(node) === path) {
      result.push(node);
    }
  }

  return result;
}

export function getDevicetreeNodePath(node: Parser.SyntaxNode | null) {
  const parts = getDevicetreeNodePathParts(node);

  if (parts.length === 0) {
    return "";
  }

  if (parts.length === 1) {
    return parts[0];
  }

  const path = parts.join("/");

  // The top-level node should be named "/", which is a special case since the
  // path should not start with "//".
  return parts[0] === "/" ? path.substring(1) : path;
}

export function getDevicetreeNodePathParts(
  node: Parser.SyntaxNode | null
): string[] {
  // There may be intermediate syntax nodes between devicetree nodes, such as
  // #if blocks, so if we aren't currently on a "node" node, traverse up the
  // tree until we find one.
  const dtnode = getContainingDevicetreeNode(node);
  if (!dtnode) {
    return [];
  }

  const name = dtnode.childForFieldName("name")?.text ?? "";

  return [...getDevicetreeNodePathParts(dtnode.parent), name];
}

function getContainingDevicetreeNode(node: Parser.SyntaxNode | null) {
  while (node && node.type !== "node") {
    node = node.parent;
  }
  return node;
}
