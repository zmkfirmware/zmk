/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

var PrebuildPlugin = require("prebuild-webpack-plugin");
const path = require("path");
const fs = require("fs");
const glob = require("glob");
const yaml = require("js-yaml");
const Mustache = require("mustache");

function generateSetupScripts() {
  return glob("../app/boards/**/*.zmk.yml", (error, files) => {
    const aggregated = files.map((f) => ({
      ...yaml.load(fs.readFileSync(f, "utf8")),
      __base_dir: path.basename(path.dirname(f)),
    }));

    const data = aggregated.reduce(
      (agg, item) => {
        switch (item.type) {
          case "shield":
            item.compatible = true;
            item.split = item.siblings?.length > 1;
            agg.keyboards.push(item);
            break;
          case "board":
            if (item.features?.includes("keys")) {
              agg.keyboards.push(item);
            } else {
              agg.boards.push(item);
            }
            break;
        }
        return agg;
      },
      { keyboards: [], boards: [] }
    );

    data.keyboards.sort((a, b) => a.name.localeCompare(b.name));
    data.boards.sort((a, b) => a.name.localeCompare(b.name));

    for (let script_ext of ["sh", "ps1"]) {
      const templateBuffer = fs.readFileSync(
        `src/templates/setup.${script_ext}.mustache`,
        "utf8"
      );
      const script = Mustache.render(templateBuffer, data);
      fs.writeFileSync(`static/setup.${script_ext}`, script);
    }
  });
}

module.exports = function () {
  return {
    name: "setup-script-generation-plugin",
    configureWebpack() {
      return {
        plugins: [
          new PrebuildPlugin({
            build: generateSetupScripts,
          }),
        ],
      };
    },
  };
};
