import { makeProject } from "@motion-canvas/core/lib";

import tap_unless_interrupted_invoke_hold from "../../scenes/hold_tap/tap_unless_interrupted/tap_unless_interrupted_invoke_hold?scene";

export default makeProject({
  scenes: [tap_unless_interrupted_invoke_hold],
  background: "#FFFFFF",
});
