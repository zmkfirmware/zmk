/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

var PrebuildPlugin = require("prebuild-webpack-plugin");
const fs = require("fs");
const yaml = require("js-yaml");
const glob = require("glob");

function generateHardwareMetadataAggregate() {
  glob("../app/boards/**/*.zmk.yml", (error, files) => {
    const aggregated = files.flatMap((f) =>
      yaml.loadAll(fs.readFileSync(f, "utf8"))
    );

    aggregated
      .filter((agg) => agg.type === "interconnect")
      .forEach((agg) => {
        let baseDir = `src/data/interconnects/${agg.id}`;
        if (!fs.existsSync(baseDir)) {
          fs.mkdirSync(baseDir);
        }

        if (agg.design_guideline) {
          fs.writeFileSync(
            `${baseDir}/design_guideline.md`,
            agg.design_guideline
          );
        }
      });
    fs.writeFileSync(
      "src/data/hardware-metadata.json",
      JSON.stringify(aggregated)
    );
  });
}

module.exports = function () {
  return {
    name: "hardware-metadata-collection-plugin",
    configureWebpack() {
      return {
        plugins: [
          new PrebuildPlugin({
            build: generateHardwareMetadataAggregate,
          }),
        ],
      };
    },
  };
};
