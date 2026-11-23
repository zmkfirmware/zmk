import { createRequire } from "node:module";
import path from "node:path";

const require = createRequire(import.meta.url);

// Resolve the plugin from pre-commit's NODE_PATH to load from its isolated node_modules.
const nodePath = process.env.NODE_PATH?.split(path.delimiter).filter(Boolean);
const astroPlugin = require.resolve(
  "prettier-plugin-astro",
  nodePath?.length ? { paths: nodePath } : undefined
);

/** @type {import("prettier").Config} */
export default {
  plugins: [astroPlugin],
  endOfLine: "auto",
  trailingComma: "es5",
  overrides: [
    {
      files: "*.astro",
      options: {
        parser: "astro",
      },
    },
  ],
};
