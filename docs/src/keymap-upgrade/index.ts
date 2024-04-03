import { createParser } from "./parser";
import { applyEdits, Range } from "./textedit";

import { upgradeBehaviors } from "./behaviors";
import { upgradeEncoderResolution } from "./encoder";
import { upgradeHeaders } from "./headers";
import { upgradeKeycodes } from "./keycodes";
import { upgradeNodeNames } from "./nodes";
import { upgradeProperties } from "./properties";

export { initParser } from "./parser";

const upgradeFunctions = [
  upgradeBehaviors,
  upgradeEncoderResolution,
  upgradeHeaders,
  upgradeKeycodes,
  upgradeNodeNames,
  upgradeProperties,
];

export function upgradeKeymap(text: string) {
  const parser = createParser();
  const tree = parser.parse(text);

  const edits = upgradeFunctions.map((f) => f(tree)).flat();

  return applyEdits(text, edits);
}

export function rangesToLineNumbers(
  text: string,
  changedRanges: Range[]
): string {
  const lineBreaks = getLineBreakPositions(text);

  const changedLines = changedRanges.map((range) => {
    const startLine = positionToLineNumber(range.startIndex, lineBreaks);
    const endLine = positionToLineNumber(range.endIndex, lineBreaks);

    return startLine === endLine ? `${startLine}` : `${startLine}-${endLine}`;
  });

  return `{${changedLines.join(",")}}`;
}

function getLineBreakPositions(text: string) {
  const positions: number[] = [];
  let index = 0;

  while ((index = text.indexOf("\n", index)) >= 0) {
    positions.push(index);
    index++;
  }

  return positions;
}

function positionToLineNumber(position: number, lineBreaks: number[]) {
  if (position >= lineBreaks[lineBreaks.length - 1]) {
    return lineBreaks.length + 1;
  }

  const line = lineBreaks.findIndex((lineBreak) => position < lineBreak);

  return line < 0 ? 0 : line + 1;
}
