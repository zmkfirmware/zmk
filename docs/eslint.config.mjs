import { defineConfig, globalIgnores } from "eslint/config";
import globals from "globals";
import jseslint from "@eslint/js";
import tseslint from "typescript-eslint";
import pluginReact from "eslint-plugin-react";
import * as mdx from "eslint-plugin-mdx";

export default defineConfig([
  globalIgnores([
    ".cache-loader/",
    ".docusaurus/",
    "build/",
    "src/hardware-metadata.d.ts",
  ]),
  { files: ["**/*.{js,jsx,ts,tsx,md,mdx}"] },
  jseslint.configs.recommended,
  tseslint.configs.strict,
  mdx.flat,
  pluginReact.configs.flat.recommended,
  pluginReact.configs.flat["jsx-runtime"],
  {
    languageOptions: {
      globals: { ...globals.browser, ...globals.commonjs, ...globals.node },
    },
    rules: {
      "react/no-unescaped-entities": "off",
      "@typescript-eslint/no-require-imports": "off",
    },
    settings: {
      react: {
        version: "detect",
      },
    },
  },
]);
