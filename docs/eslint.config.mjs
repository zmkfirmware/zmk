import { defineConfig, globalIgnores } from "eslint/config";
import globals from "globals";
import js from "@eslint/js";
import pluginReact from "eslint-plugin-react";
import * as mdx from "eslint-plugin-mdx";

export default defineConfig([
  globalIgnores([".cache-loader/", ".docusaurus/", "build/"]),
  { files: ["**/*.{js,jsx,md,mdx}"] },
  {
    plugins: { js },
    extends: ["js/recommended"],
  },
  mdx.flat,
  pluginReact.configs.flat.recommended,
  pluginReact.configs.flat["jsx-runtime"],
  {
    languageOptions: {
      globals: { ...globals.browser, ...globals.commonjs, ...globals.node },
    },
    rules: { "react/no-unescaped-entities": "off" },
    settings: {
      react: {
        version: "detect",
      },
    },
  },
]);
