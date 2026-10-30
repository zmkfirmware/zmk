/**
 * Map a Starlight content-collection file path back to the path used by the
 * ZMK repository (the docs site lives in the docs/ directory).
 */
export function getEntryEditPath(filePath) {
  if (!filePath) return undefined;

  const docsPrefix = "src/content/docs/docs/";
  const blogPrefix = "src/content/docs/blog/";

  if (filePath.startsWith(docsPrefix)) {
    return "docs/" + filePath.slice("src/content/docs/".length);
  }
  if (filePath.startsWith(blogPrefix)) {
    return "docs/" + filePath.slice("src/content/docs/".length);
  }
  return undefined;
}
