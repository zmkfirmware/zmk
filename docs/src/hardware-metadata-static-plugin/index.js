/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

//@ts-check
"use strict";

/** @typedef {import('../hardware-metadata').HardwareMetadata} HardwareMetadata */
/** @typedef {HardwareMetadata & { directory: string}} LocatedHardwareMetadata */

const fs = require("fs").promises;
const glob = require("util").promisify(require("glob"));
const path = require("path");
const yaml = require("js-yaml");

const BASE_DIR = "..";
const METADATA_GLOB = path.posix.join(BASE_DIR, "app/boards/**/*.zmk.yml");

/**
 * @param {string} file
 */
async function readMetadata(file) {
  /** @type HardwareMetadata[] */
  // @ts-ignore
  const documents = yaml.loadAll(await fs.readFile(file, "utf-8"));

  return documents.map(
    (metadata) =>
      /** @type LocatedHardwareMetadata */
      ({
        ...metadata,
        // External tools need a way to locate this hardware within the repo.
        // Append each item's relative path.
        directory: path.posix.dirname(path.posix.relative(BASE_DIR, file)),
      })
  );
}

async function aggregateMetadata() {
  const files = await glob(METADATA_GLOB);
  const data = await Promise.all(files.map(readMetadata));

  return data.flat();
}

/**
 * Aggregates .zmk.yml files and writes hardware-metadata.json to the site's
 * static files post-build.
 *
 * This is very similar to hardware-metadata-collection-plugin but adjusts the
 * output for consumption by external tools rather than the website build system.
 *
 * @type {import('@docusaurus/types').PluginModule}
 */
module.exports = function () {
  return {
    name: "hardware-metadata-static-plugin",

    async postBuild({ outDir }) {
      const hardware = await aggregateMetadata();

      const hardwarePath = path.join(outDir, "hardware-metadata.json");
      await fs.writeFile(hardwarePath, JSON.stringify(hardware));
    },
  };
};
