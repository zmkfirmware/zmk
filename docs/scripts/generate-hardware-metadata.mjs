import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { glob } from "glob";
import yaml from "js-yaml";
import { compileFromFile } from "json-schema-to-typescript";

const projectRoot = fileURLToPath(new URL("..", import.meta.url));
const repoRoot = path.resolve(projectRoot, "..");
const boardsGlob = path.posix.join(repoRoot, "app/boards/**/*.zmk.yml");
const schemaPath = path.posix.join(
  repoRoot,
  "schema/hardware-metadata.schema.json"
);

const dataDir = path.join(projectRoot, "src/data");
const metadataPath = path.join(dataDir, "hardware-metadata.json");
const interconnectsDir = path.join(dataDir, "interconnects");
const typesPath = path.join(projectRoot, "src/hardware-metadata.d.ts");

async function readMetadata(file) {
  const documents = yaml.loadAll(await fs.readFile(file, "utf8"));
  return documents.filter(Boolean);
}

export async function generateHardwareMetadata() {
  const files = await glob(boardsGlob);
  const aggregated = (await Promise.all(files.map(readMetadata))).flat();

  await fs.mkdir(dataDir, { recursive: true });
  await fs.writeFile(metadataPath, JSON.stringify(aggregated));

  for (const item of aggregated) {
    if (item.type !== "interconnect" || !item.design_guideline) continue;
    const dir = path.join(interconnectsDir, item.id);
    await fs.mkdir(dir, { recursive: true });
    // Rendered by <InterconnectTabs> as an MDX component.
    await fs.writeFile(
      path.join(dir, "design_guideline.mdx"),
      item.design_guideline
    );
  }

  const types = await compileFromFile(schemaPath);
  await fs.writeFile(typesPath, types);
}

export async function copyHardwareMetadataToDist(distDir) {
  await fs.copyFile(metadataPath, path.join(distDir, "hardware-metadata.json"));
}
