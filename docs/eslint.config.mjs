import js from "@eslint/js";
import { defineConfig, globalIgnores } from "eslint/config";
import astro from "eslint-plugin-astro";
import react from "eslint-plugin-react";
import globals from "globals";
import tseslint from "typescript-eslint";

export default defineConfig([
  globalIgnores(["dist/", ".astro/", "node_modules/", "src/env.d.ts"]),
  { files: ["**/*.{js,mjs,jsx,ts,tsx,astro}"] },
  js.configs.recommended,
  ...tseslint.configs.strict,
  ...astro.configs.recommended,
  {
    ...react.configs.flat.recommended,
    files: ["**/*.{jsx,tsx}"],
  },
  {
    ...react.configs.flat["jsx-runtime"],
    files: ["**/*.{jsx,tsx}"],
  },
  {
    files: ["**/*.{js,mjs,jsx,ts,tsx,astro}"],
    languageOptions: {
      globals: {
        ...globals.browser,
        ...globals.commonjs,
        ...globals.node,
        URL: "readonly",
        process: "readonly",
      },
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
