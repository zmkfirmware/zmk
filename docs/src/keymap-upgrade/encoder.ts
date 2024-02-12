import type { SyntaxNode, Tree } from "web-tree-sitter";

import {
  getContainingDevicetreeNode,
  getDevicetreeNodePath,
  findDevicetreeProperty,
} from "./parser";
import { TextEdit } from "./textedit";

const ALPS_EC11_COMPATIBLE = '"alps,ec11"';
const DEFAULT_RESOLUTION = 4;
const TRIGGERS_PER_ROTATION = 20;
const TRIGGERS_PER_ROTATION_DT = `

&sensors {
    // Change this to your encoder's number of detents.
    // If you have multiple encoders with different detents, see
    // https://zmk.dev/docs/config/encoders#keymap-sensor-config
    triggers-per-rotation = <${TRIGGERS_PER_ROTATION}>;
};`;

export function upgradeEncoderResolution(tree: Tree) {
  const edits: TextEdit[] = [];

  const resolutionProps = findEncoderResolution(tree);
  edits.push(...resolutionProps.flatMap(upgradeResolutionProperty));

  if (resolutionProps.length > 0) {
    edits.push(...addTriggersPerRotation(tree));
  }

  return edits;
}

function findEncoderResolution(tree: Tree): SyntaxNode[] {
  const props = findDevicetreeProperty(tree.rootNode, "resolution", {
    recursive: true,
  });

  return props.filter((prop) => {
    const node = getContainingDevicetreeNode(prop);
    return node && isEncoderNode(node);
  });
}

function isEncoderNode(node: SyntaxNode) {
  // If a compatible property is set, then we know for sure if this is an encoder.
  const compatible = findDevicetreeProperty(node, "compatible");
  if (compatible) {
    return compatible.childForFieldName("value")?.text === ALPS_EC11_COMPATIBLE;
  }

  // Compatible properties rarely appear in a keymap though, so just guess based
  // on the node path/reference otherwise.
  return getDevicetreeNodePath(node).toLowerCase().includes("encoder");
}

function upgradeResolutionProperty(prop: SyntaxNode): TextEdit[] {
  const name = prop.childForFieldName("name");
  const value = prop.childForFieldName("value");

  if (!name || !value) {
    return [];
  }

  // Try to set the new steps to be triggers-per-rotation * resolution, but fall
  // back to a default if the value is something more complex than a single int.
  const resolution = value.text.trim().replaceAll(/^<|>$/g, "");
  const steps =
    (parseInt(resolution) || DEFAULT_RESOLUTION) * TRIGGERS_PER_ROTATION;

  const hint = `/* Change this to your encoder's number of detents times ${resolution} */`;

  return [
    TextEdit.fromNode(name, "steps"),
    TextEdit.fromNode(value, `<${steps}> ${hint}`),
  ];
}

function addTriggersPerRotation(tree: Tree): TextEdit[] {
  // The keymap might already contain "triggers-per-rotation" for example if the
  // user already upgraded some but not all "resolution" properties. Don't add
  // another one if it already exists.
  if (keymapHasTriggersPerRotation(tree)) {
    return [];
  }

  // Inserting a new property into an existing node while keeping the code
  // readable in all cases is hard, so just append a new &sensors node to the
  // end of the keymap.
  const end = tree.rootNode.endIndex;
  return [new TextEdit(end, end, TRIGGERS_PER_ROTATION_DT)];
}

function keymapHasTriggersPerRotation(tree: Tree) {
  const props = findDevicetreeProperty(tree.rootNode, "triggers-per-rotation", {
    recursive: true,
  });

  return props.length > 0;
}
