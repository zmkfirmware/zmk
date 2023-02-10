import { makeProject } from "@motion-canvas/core/lib";

import tap_unless_interrupted from "../../scenes/hold_tap/tap_unless_interrupted/tap_unless_interrupted?scene";

export default makeProject({
  scenes: [tap_unless_interrupted],
  background: "#FFFFFF",
});
