import type { Tree } from "web-tree-sitter";

import { findDevicetreeNode } from "./parser";
import { TextEdit } from "./textedit";

// Map of { "deprecated path": "replacement name" } for devicetree nodes.
// Relocating nodes to another place in the tree is not supported.
const NODES = {
  "/behaviors/behavior_backlight": "bcklight",
  "/behaviors/behavior_caps_word": "caps_word",
  "/behaviors/behavior_ext_power": "extpower",
  "/behaviors/behavior_key_press": "key_press",
  "/behaviors/behavior_key_repeat": "key_repeat",
  "/behaviors/behavior_key_toggle": "key_toggle",
  "/behaviors/behavior_layer_tap": "layer_tap",
  "/behaviors/behavior_mod_tap": "mod_tap",
  "/behaviors/behavior_momentary_layer": "momentary_layer",
  "/behaviors/behavior_none": "none",
  "/behaviors/behavior_outputs": "outputs",
  "/behaviors/behavior_behavior_reset": "sysreset",
  "/behaviors/behavior_reset_dfu": "bootload",
  "/behaviors/behavior_rgb_underglow": "rgb_ug",
  "/behaviors/behavior_sensor_rotate_key_press": "enc_key_press",
  "/behaviors/behavior_sticky_key": "sticky_key",
  "/behaviors/behavior_sticky_layer": "sticky_layer",
  "/behaviors/behavior_to_layer": "to_layer",
  "/behaviors/behavior_toggle_layer": "toggle_layer",
  "/behaviors/behavior_transparent": "transparent",
  "/behaviors/macro_control_mode_tap": "macro_tap",
  "/behaviors/macro_control_mode_press": "macro_press",
  "/behaviors/macro_control_mode_release": "macro_release",
  "/behaviors/macro_control_tap_time": "macro_tap_time",
  "/behaviors/macro_control_wait_time": "macro_wait_time",
};

export function upgradeNodeNames(tree: Tree) {
  const edits: TextEdit[] = [];

  for (const [path, newName] of Object.entries(NODES)) {
    for (const node of findDevicetreeNode(tree, path)) {
      const name = node.childForFieldName("name");
      if (name) {
        edits.push(TextEdit.fromNode(name, newName));
      }
    }
  }

  return edits;
}
