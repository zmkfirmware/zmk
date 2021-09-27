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
      yaml.safeLoadAll(fs.readFileSync(f, "utf8"))
    );
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
