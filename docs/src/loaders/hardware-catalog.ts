import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

import type { Loader } from "astro/loaders";
import { glob } from "glob";
import yaml from "js-yaml";

import {
  hardwareBoardName,
  hardwareSlug,
  type HardwareKind,
} from "../lib/hardware-catalog";

const repoRoot = fileURLToPath(new URL("../../../", import.meta.url));
const boardsRoot = path.join(repoRoot, "app", "boards");

const METADATA_GLOB = "**/*.zmk.yml";

type Metadata = Record<string, unknown> & {
  id?: unknown;
  name?: unknown;
  type?: unknown;
};

type HardwareTarget = Metadata & {
  id: string;
  name: string;
  type: HardwareKind;
};

interface Source {
  metadata: HardwareTarget;
  file: string;
  contents: string;
  readme?: string;
  readmeContents?: string;
  sourcePath: string;
  readmePath?: string;
}

function repoPath(file: string) {
  return path.relative(repoRoot, file).split(path.sep).join("/");
}

function stringArray(value: unknown): string[] {
  return Array.isArray(value)
    ? value.filter((item): item is string => typeof item === "string")
    : [];
}

function unique(values: string[]) {
  return [...new Set(values)];
}

function uniqueVariants(values: unknown[]) {
  const seen = new Set<string>();

  return values.filter((value) => {
    const key = JSON.stringify(value);

    if (seen.has(key)) {
      return false;
    }

    seen.add(key);

    return true;
  });
}

function normalizeHeadingText(text: string) {
  return text
    .replace(/\[([^\]]+)\]\([^)]+\)/g, "$1")
    .replace(/[*_`]/g, "")
    .trim()
    .toLocaleLowerCase();
}

/**
 * Remove a leading README heading that repeats the catalog entry name.
 *
 * The catalog page already renders the entry name as its title, so keeping the
 * README's matching title would show the same heading twice and mix up the
 * page outline.
 */
function stripDuplicateTitle(markdown: string, name: string) {
  const heading = markdown.match(/^\s*#\s+(.+?)\s*$/m);

  if (
    !heading ||
    heading.index === undefined ||
    normalizeHeadingText(heading[1]) !== name.trim().toLocaleLowerCase()
  ) {
    return markdown;
  }

  const before = markdown.slice(0, heading.index);
  const after = markdown.slice(heading.index + heading[0].length);

  return `${before}${after.replace(/^\s*\n/, "")}`;
}

/**
 * Find the first available documentation file for a hardware target.
 *
 * Full-target candidates use the target's path segments as directories, for
 * example `doc/nrf5340dk/nrf5340/cpuapp.zmk.md`. The final filename is matched
 * case-insensitively so existing `README.md` files keep working.
 */
async function findReadme(directory: string, target: HardwareTarget) {
  const docDirectory = path.join(directory, "doc");
  const fullTarget = target.id;
  const justName =
    target.type === "board" ? hardwareBoardName(target.id) : target.id;

  const candidates = [
    path.join(docDirectory, `${fullTarget}.zmk.md`),
    path.join(docDirectory, `${justName}.zmk.md`),
    path.join(docDirectory, `${fullTarget}.md`),
    path.join(docDirectory, `${justName}.md`),
    path.join(docDirectory, "readme.zmk.md"),
    path.join(docDirectory, "readme.md"),
    path.join(directory, "readme.zmk.md"),
    path.join(directory, "readme.md"),
  ];

  for (const candidate of new Set(candidates)) {
    try {
      const stat = await fs.stat(candidate);

      if (stat.isFile()) {
        return candidate;
      }
    } catch {
      // Try a case-insensitive match below.
    }

    const parent = path.dirname(candidate);
    const filename = path.basename(candidate).toLowerCase();

    try {
      const entries = await fs.readdir(parent, { withFileTypes: true });
      const match = entries.find(
        (entry) => entry.isFile() && entry.name.toLowerCase() === filename
      );

      if (match) {
        return path.join(parent, match.name);
      }
    } catch {
      // Candidate directory does not exist.
    }
  }

  return undefined;
}

/**
 * Merge all targets of one board or shield into a single catalog entry.
 *
 * Board targets like `nrfmicro/nrf52840/zmk` and
 * `nrfmicro/nrf52840/flipped_zmk` share the board name `nrfmicro`, while
 * shields normally have a single target. The complete target data is retained
 * on `targets`; common fields are merged for the catalog page.
 *
 * The actual values accepted by `-b` and `-DSHIELD=` are exposed as
 * `buildTargets`. Split hardware lists its parts in `siblings`, so those are
 * the build targets even though the catalog entry keeps the logical device id.
 */
function buildData(
  type: HardwareKind,
  name: string,
  sources: Source[],
  primary: Source,
  readmeSource: Source | undefined
) {
  const features = unique(
    sources.flatMap((source) => stringArray(source.metadata.features))
  );
  const exposes = unique(
    sources.flatMap((source) => stringArray(source.metadata.exposes))
  );
  const siblings = unique(
    sources.flatMap((source) => stringArray(source.metadata.siblings))
  );
  const revisions = unique(
    sources.flatMap((source) => stringArray(source.metadata.revisions))
  );
  const targetVariants = uniqueVariants(
    sources.flatMap((source) =>
      Array.isArray(source.metadata.variants) ? source.metadata.variants : []
    )
  );
  const defaultRevision =
    primary.metadata.default_revision ??
    sources.find((source) => source.metadata.default_revision)?.metadata
      .default_revision;

  const targets = sources.map((source) => ({
    ...source.metadata,
    sourcePath: source.sourcePath,
    ...(source.readmePath ? { readmePath: source.readmePath } : {}),
  }));

  const buildTargets = siblings.length
    ? siblings
    : type === "board"
      ? targets.map((target) => target.id)
      : [name];

  return {
    ...primary.metadata,
    id: name,
    name: sources.length === 1 ? primary.metadata.name : name,
    type,
    slug: name,
    sourcePath: primary.sourcePath,
    ...(readmeSource?.readmePath
      ? { readmePath: readmeSource.readmePath }
      : {}),
    targets,
    buildTargets,
    ...(features.length ? { features } : {}),
    ...(exposes.length ? { exposes } : {}),
    ...(siblings.length ? { siblings } : {}),
    ...(revisions.length ? { revisions } : {}),
    ...(targetVariants.length ? { variants: targetVariants } : {}),
    ...(type === "board"
      ? {
          category: features.includes("keys") ? "keyboard" : "controller",
          outputs: unique(
            sources.flatMap((source) => stringArray(source.metadata.outputs))
          ),
        }
      : {
          category: "shield",
          requires: unique(
            sources.flatMap((source) => stringArray(source.metadata.requires))
          ),
        }),
    ...(defaultRevision !== undefined
      ? { default_revision: defaultRevision }
      : {}),
  };
}

/**
 * Load every board and shield metadata file into the `hardware` collection.
 *
 * A board's Zephyr target id contains the board name, revision, and qualifiers
 * (for example `nrf5340dk/nrf5340/cpuapp`). The catalog id is the board name,
 * so all targets for one board become a single page and the complete target
 * ids are retained in `targets`.
 *
 * If a README exists in the target directory it is injected as the entry body
 * and rendered with the markdown renderer, so README images and links keep
 * resolving relative to the README file.
 */
export const hardwareLoader: Loader = {
  name: "zmk-hardware-catalog",
  load: async ({
    store,
    parseData,
    renderMarkdown,
    generateDigest,
    logger,
    watcher,
    config,
  }) => {
    const siteRoot = fileURLToPath(config.root);

    async function loadAll() {
      store.clear();

      const files = (
        await glob(METADATA_GLOB, { cwd: boardsRoot, absolute: true })
      ).sort();
      const sources: Source[] = [];
      const targetIds = new Set<string>();

      for (const file of files) {
        const contents = await fs.readFile(file, "utf8");
        const documents = yaml
          .loadAll(contents)
          .filter((document): document is Metadata => {
            return typeof document === "object" && document !== null;
          });
        const directory = path.dirname(file);
        const sourcePath = repoPath(file);

        for (const metadata of documents) {
          // Interconnect metadata is shared by boards and shields but does not
          // get a catalog page of its own in this first pass.
          const type = metadata.type;

          if (type !== "board" && type !== "shield") {
            continue;
          }

          if (typeof metadata.id !== "string") {
            throw new Error(
              `Hardware metadata in ${sourcePath} has no string id.`
            );
          }

          if (typeof metadata.name !== "string") {
            throw new Error(
              `Hardware metadata in ${sourcePath} has no string name.`
            );
          }

          const target: HardwareTarget = {
            ...metadata,
            id: metadata.id,
            name: metadata.name,
            type,
          };
          const targetId = `${target.type}/${target.id}`;

          if (targetIds.has(targetId)) {
            throw new Error(
              `Duplicate hardware target "${target.id}" in ${sourcePath}.`
            );
          }

          targetIds.add(targetId);

          const readme = await findReadme(directory, target);
          let readmeContents: string | undefined;

          if (readme) {
            readmeContents = await fs.readFile(readme, "utf8");
          }

          sources.push({
            metadata: target,
            file,
            contents,
            readme,
            readmeContents,
            sourcePath,
            ...(readme ? { readmePath: repoPath(readme) } : {}),
          });
        }
      }

      const groups = new Map<string, Source[]>();

      for (const source of sources) {
        const name = hardwareSlug(source.metadata.type, source.metadata.id);
        const id = `${source.metadata.type}/${name}`;
        const group = groups.get(id);

        if (group) {
          group.push(source);
        } else {
          groups.set(id, [source]);
        }
      }

      for (const [id, group] of groups) {
        const [first] = group;
        const type = first.metadata.type;
        const name = hardwareSlug(type, first.metadata.id);
        // Use the target with the most revisions for shared board fields.
        const primary = group.reduce((best, source) => {
          const bestRevisions = Array.isArray(best.metadata.revisions)
            ? best.metadata.revisions.length
            : 0;
          const sourceRevisions = Array.isArray(source.metadata.revisions)
            ? source.metadata.revisions.length
            : 0;

          return sourceRevisions > bestRevisions ? source : best;
        });
        const readmeSource =
          group.find((source) => source.readmeContents?.trim()) ??
          group.find((source) => source.readme);
        const data = buildData(type, name, group, primary, readmeSource);

        let body: string | undefined;
        let rendered: Awaited<ReturnType<typeof renderMarkdown>> | undefined;
        const readme = readmeSource?.readme;

        if (readme && readmeSource?.readmeContents?.trim()) {
          body = stripDuplicateTitle(readmeSource.readmeContents, data.name);
          rendered = await renderMarkdown(body, {
            fileURL: pathToFileURL(readme),
          });
        }

        const parsedData = await parseData({
          id,
          data,
          filePath: primary.file,
        });

        const filePath = path
          .relative(siteRoot, readme ?? primary.file)
          .split(path.sep)
          .join("/");
        const digest = generateDigest(
          group
            .map(
              (source) => `${source.contents}\n${source.readmeContents ?? ""}`
            )
            .join("\n")
        );

        store.set({
          id,
          data: parsedData,
          ...(body ? { body } : {}),
          filePath,
          digest,
          ...(rendered
            ? {
                rendered,
                assetImports: rendered.metadata?.imagePaths ?? [],
              }
            : {}),
        });
      }

      logger.info(
        `Loaded ${groups.size} hardware entries from ${sources.length} board and shield targets in ${files.length} metadata files.`
      );
    }

    await loadAll();

    if (!watcher) {
      return;
    }

    watcher.add(boardsRoot);

    let reloadTimer: ReturnType<typeof setTimeout> | undefined;

    function scheduleReload(changedPath: string) {
      const relativePath = path.relative(boardsRoot, changedPath);

      if (relativePath.startsWith("..") || path.isAbsolute(relativePath)) {
        return;
      }

      const lowerPath = relativePath.toLowerCase();

      if (!lowerPath.endsWith(".zmk.yml") && !lowerPath.endsWith(".md")) {
        return;
      }

      if (reloadTimer) {
        clearTimeout(reloadTimer);
      }

      reloadTimer = setTimeout(() => {
        loadAll().catch((error: unknown) => {
          logger.error(
            `Failed to reload the hardware catalog: ${
              error instanceof Error ? error.message : String(error)
            }`
          );
        });
      }, 100);
    }

    watcher.on("add", scheduleReload);
    watcher.on("change", scheduleReload);
    watcher.on("unlink", scheduleReload);
  },
};
