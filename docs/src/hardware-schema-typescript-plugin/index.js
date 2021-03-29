/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

var PrebuildPlugin = require("prebuild-webpack-plugin");
const fs = require("fs");
const { compileFromFile } = require("json-schema-to-typescript");

async function generateHardwareMetadataTypescript() {
  const ts = await compileFromFile("../schema/hardware-metadata.schema.json");
  fs.writeFileSync("src/hardware-metadata.d.ts", ts);
}

module.exports = function () {
  return {
    name: "hardware-metadata-typescript-plugin",
    configureWebpack() {
      return {
        plugins: [
          new PrebuildPlugin({
            build: generateHardwareMetadataTypescript,
          }),
        ],
      };
    },
  };
};
