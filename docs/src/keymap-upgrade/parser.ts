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
 * The same node may be listed multiple times within a file.
 *
 * This function does not evaluate which node a reference points to, so given
 * a file containing "/ { foo: bar {}; }; &foo {};" searching for "&foo" will
 * return the "&foo {}" node but not "foo: bar {}".
 *
 * @param path Path to the node to find. May be an absolute path such as
 * "/foo/bar", a node reference such as "&foo", or a node reference followed by
 * a relative path such as "&foo/bar".
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

export interface FindPropertyOptions {
  /** Search in children of the given node as well */
  recursive?: boolean;
}

/**
 * Find all instances of a devicetree property with the given name which are
 * descendants of the given syntax node.
 *
 * @param node Any syntax node
 */
export function findDevicetreeProperty(
  node: Parser.SyntaxNode,
  name: string,
  options: FindPropertyOptions & { recursive: true }
): Parser.SyntaxNode[];

/**
 * Find a devicetree node's property with the given name, or null if it doesn't
 * have one.
 *
 * @note If the node contains multiple instances of the same property, this
 * returns the last once, since that is the one that will actually be applied.
 *
 * @param node A syntax node for a devicetree node
 */
export function findDevicetreeProperty(
  node: Parser.SyntaxNode,
  name: string,
  options?: FindPropertyOptions
): Parser.SyntaxNode | null;

export function findDevicetreeProperty(
  node: Parser.SyntaxNode,
  name: string,
  options?: FindPropertyOptions
): Parser.SyntaxNode[] | Parser.SyntaxNode | null {
  const query = Devicetree.query(
    `(property name: (identifier) @name (#eq? @name "${name}")) @prop`
  );
  const matches = query.matches(node);
  const props = matches.map(({ captures }) => findCapture("prop", captures)!);

  if (options?.recursive) {
    return props;
  }

  // The query finds all descendants. Filter to just the properties that belong
  // to the given devicetree node.
  const childProps = props.filter((prop) =>
    getContainingDevicetreeNode(prop)?.equals(node)
  );

  // Sort in descending order to select the last instance of the property.
  childProps.sort((a, b) => b.startIndex - a.startIndex);
  return childProps[0] ?? null;
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

function getDevicetreeNodePathParts(node: Parser.SyntaxNode | null): string[] {
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

export function getContainingDevicetreeNode(node: Parser.SyntaxNode | null) {
  while (node && node.type !== "node") {
    node = node.parent;
  }
  return node;
}
