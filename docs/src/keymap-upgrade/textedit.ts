import type { SyntaxNode } from "web-tree-sitter";

export class Range {
  constructor(public startIndex: number, public endIndex: number) {}
}

export class TextEdit extends Range {
  newText: string;

  /**
   * Creates a text edit to replace a range with new text.
   */
  constructor(startIndex: number, endIndex: number, newText: string) {
    super(startIndex, endIndex);
    this.newText = newText;
  }

  static fromNode(node: SyntaxNode | Range, newText: string) {
    return new TextEdit(node.startIndex, node.endIndex, newText);
  }
}

export type MatchFunc = (node: SyntaxNode, text: string) => boolean;
export type ReplaceFunc = (
  node: SyntaxNode,
  replacement: string | null
) => TextEdit[];

/**
 * Gets a list of text edits to apply based on a node and a map of text
 * replacements.
 *
 * If replaceHandler is given, it will be called if the node matches a
 * deprecated value and it should return the text edits to apply.
 * Otherwise, the full node is replaced by the new text.
 *
 * If isMatch is given, it will be called to check if a node matches a
 * deprecated value. Otherwise, the node's text is matched against the
 * deprecated text.
 *
 * @param {SyntaxNode} node
 * @param {Record<string, string | null>} replacementMap
 * @param {ReplaceFunc} [replaceHandler]
 * @param {MatchFunc} [isMatch]
 */
export function getUpgradeEdits(
  node: SyntaxNode,
  replacementMap: Record<string, string | null>,
  replaceHandler?: ReplaceFunc,
  isMatch?: MatchFunc
) {
  const defaultReplace: ReplaceFunc = (node, replacement) => [
    TextEdit.fromNode(node, replacement ?? ""),
  ];
  const defaultMatch: MatchFunc = (node, text) => node.text === text;

  replaceHandler = replaceHandler ?? defaultReplace;
  isMatch = isMatch ?? defaultMatch;

  for (const [deprecated, replacement] of Object.entries(replacementMap)) {
    if (isMatch(node, deprecated)) {
      return replaceHandler(node, replacement);
    }
  }
  return [];
}

/**
 * Sorts a list of text edits in ascending order by position.
 */
function sortEdits(edits: TextEdit[]) {
  return edits.sort((a, b) => a.startIndex - b.startIndex);
}

export interface EditResult {
  text: string;
  changedRanges: Range[];
}

interface TextChunk {
  text: string;
  changed?: boolean;
}

/**
 * Returns a string with text replacements applied and a list of ranges within
 * that string that were modified.
 */
export function applyEdits(text: string, edits: TextEdit[]) {
  // If we are removing text and it's the only thing on a line, remove the whole line.
  edits = edits.map((e) => (e.newText ? e : expandEditToLine(text, e)));
  edits = sortEdits(edits);

  const chunks: TextChunk[] = [];
  let currentIndex = 0;

  for (let edit of edits) {
    if (edit.startIndex < currentIndex) {
      console.warn("discarding overlapping edit", edit);
      continue;
    }

    chunks.push({ text: text.substring(currentIndex, edit.startIndex) });
    chunks.push({ text: edit.newText, changed: true });
    currentIndex = edit.endIndex;
  }

  chunks.push({ text: text.substring(currentIndex) });

  // Join all of the text chunks while recording the ranges of any chunks that were changed.
  return chunks.reduce<EditResult>(
    (prev, current) => {
      return {
        text: prev.text + current.text,
        changedRanges: reduceChangedRanges(prev, current),
      };
    },
    { text: "", changedRanges: [] }
  );
}

function reduceChangedRanges(prev: EditResult, current: TextChunk): Range[] {
  if (current.changed) {
    return [
      ...prev.changedRanges,
      new Range(prev.text.length, prev.text.length + current.text.length),
    ];
  }

  return prev.changedRanges;
}

/**
 * If the given edit is surrounded by only whitespace on a line, expands it to
 * replace the entire line, else returns it unmodified.
 */
function expandEditToLine(text: string, edit: TextEdit) {
  // Expand the selection to adjacent whitespace
  let newStart = edit.startIndex;
  let newEnd = edit.endIndex;

  while (newStart > 0 && text[newStart - 1].match(/[ \t]/)) {
    newStart--;
  }

  while (newEnd < text.length && text[newEnd].match(/[ \t]/)) {
    newEnd++;
  }

  // Check that we selected the entire line
  if (
    (newEnd !== text.length && text[newEnd] !== "\n") ||
    (newStart > 0 && text[newStart - 1] !== "\n")
  ) {
    return edit;
  }

  // Select one of the line breaks to remove.
  if (newEnd !== text.length) {
    newEnd++;
  } else if (newStart !== 0) {
    newStart--;
  }

  return new TextEdit(newStart, newEnd, edit.newText);
}
