import { defineConfig } from "vite";
import motionCanvas from "@motion-canvas/vite-plugin";

export default defineConfig({
  plugins: [
    motionCanvas({
      project: ["./src/project.ts"],
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
