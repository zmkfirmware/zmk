/** @type {import("prettier").Config} */
export default {
  plugins: ["prettier-plugin-astro"],
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
