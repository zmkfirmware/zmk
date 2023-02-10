import { defineConfig } from "vite";
import motionCanvas from "@motion-canvas/vite-plugin";

export default defineConfig({
  plugins: [
    motionCanvas({
      project: [
        "./src/hold_tap/hold_tap_comparison.ts",
        "./src/hold_tap/hold_tap_interrupted.ts",
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
