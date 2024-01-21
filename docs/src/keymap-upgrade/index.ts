import { createParser } from "./parser";
import { applyEdits } from "./textedit";

import { upgradeBehaviors } from "./behaviors";
import { upgradeHeaders } from "./headers";
import { upgradeKeycodes } from "./keycodes";
import { upgradeProperties } from "./properties";

export { initParser } from "./parser";

const upgradeFunctions = [
  upgradeBehaviors,
  upgradeHeaders,
  upgradeKeycodes,
  upgradeProperties,
];

export function upgradeKeymap(text: string) {
  const parser = createParser();
  const tree = parser.parse(text);

  const edits = upgradeFunctions.map((f) => f(tree)).flat();

  return applyEdits(text, edits);
}
