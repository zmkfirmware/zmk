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
