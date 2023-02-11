import { defineConfig } from "vite";
import motionCanvas from "@motion-canvas/vite-plugin";

export default defineConfig({
  plugins: [
    motionCanvas({
      project: [
        "./src/hold_tap/hold_tap_comparison.ts",
        "./src/hold_tap/hold_tap_interrupted.ts",
        "./src/hold_tap/balanced/balanced_comparison.ts",
        "./src/hold_tap/balanced/balanced_other_key_up.ts",
        "./src/hold_tap/balanced/balanced_hold_tap_up.ts",
        "./src/hold_tap/tap_preferred/tap_preferred_comparison.ts",
        "./src/hold_tap/tap_preferred/tap_preferred_hold_tap_up.ts",
        "./src/hold_tap/tap_unless_interrupted/tap_unless_interrupted.ts",
        "./src/hold_tap/tap_unless_interrupted/tap_unless_interrupted_invoke_hold.ts",
      ],
    }),
  ],
  build: {
    rollupOptions: {
      output: {
        dir: "../../static/animations",
        entryFileNames: "[name].js",
      },
    },
  },
});
