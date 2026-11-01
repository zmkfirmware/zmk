import { promises as fs, type Dirent } from "node:fs";
import path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

import type { Loader } from "astro/loaders";
import { glob } from "glob";
import yaml from "js-yaml";

import {
  hardwareBoardName,
  hardwareSlug,
  type HardwareCategory,
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

/** Use the words shared by all target names as the catalog title. */
function commonNamePrefix(names: string[]): string {
  const words = names.map((name) => name.trim().split(/\s+/));
  const first = words[0];

  if (!first) {
    return "";
  }

  let length = 0;

  while (
    length < first.length &&
    words.every((other) => other[length] === first[length])
  ) {
    length += 1;
  }

  if (length === 0) {
    return first.join(" ");
  }

  return first.slice(0, length).join(" ");
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

function readDesignGuideline(metadata: Metadata) {
  const value = metadata.design_guideline;

  if (typeof value !== "string" || !value.trim()) {
    return undefined;
  }

  return value;
}

function normalizeHeadingText(text: string) {
  return text
    .replace(/\[([^\]]+)\]\([^)]+\)/g, "$1")
    .replace(/[*_`]/g, "")
    .trim()
    .toLocaleLowerCase();
}

/** Remove a leading README heading that repeats the catalog entry name. */
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

/** Check whether an error means the path does not exist. */
function isMissingPathError(error: unknown): boolean {
  return (
    typeof error === "object" &&
    error !== null &&
    "code" in error &&
    (error.code === "ENOENT" || error.code === "ENOTDIR")
  );
}

/**
 * Find the documentation file for a hardware target.
 * Candidates are checked in order, then matched case-insensitively.
 */
async function findReadme(directory: string, target: HardwareTarget) {
  const docDirectory = path.join(directory, "doc");
  const boardName =
    target.type === "board" ? hardwareBoardName(target.id) : target.id;

  const candidates = [
    path.join(docDirectory, `${target.id}.zmk.md`),
    path.join(docDirectory, `${boardName}.zmk.md`),
    path.join(docDirectory, `${target.id}.md`),
    path.join(docDirectory, `${boardName}.md`),
    path.join(docDirectory, "readme.zmk.md"),
    path.join(docDirectory, "readme.md"),
    path.join(directory, "readme.zmk.md"),
    path.join(directory, "readme.md"),
  ];

  const listings = new Map<string, Dirent[] | undefined>();

  for (const candidate of new Set(candidates)) {
    try {
      if ((await fs.stat(candidate)).isFile()) {
        return candidate;
      }
    } catch (error) {
      if (!isMissingPathError(error)) {
        throw error;
      }
    }

    const parent = path.dirname(candidate);

    if (!listings.has(parent)) {
      try {
        listings.set(parent, await fs.readdir(parent, { withFileTypes: true }));
      } catch (error) {
        if (!isMissingPathError(error)) {
          throw error;
        }

        listings.set(parent, undefined);
      }
    }

    const entries = listings.get(parent);

    if (!entries) {
      continue;
    }

    const filename = path.basename(candidate).toLowerCase();
    const match = entries.find(
      (entry) => entry.isFile() && entry.name.toLowerCase() === filename
    );

    if (match) {
      return path.join(parent, match.name);
    }
  }

  return undefined;
}

/**
 * Merge the targets of one board or shield into a catalog entry.
 * `targets` keeps the full metadata; `buildTargets` holds the values accepted
 * by `-b` and `-DSHIELD=`.
 */
function buildData(
  type: HardwareKind,
  slug: string,
  sources: Source[],
  primary: Source,
  readmeSource: Source | undefined
) {
  const names = sources.map((source) => source.metadata.name);
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

  const targets = sources.map((source) => {
    const target: Record<string, unknown> = {
      ...source.metadata,
      sourcePath: source.sourcePath,
    };

    if (source.readmePath) {
      target.readmePath = source.readmePath;
    }

    return target;
  });

  let buildTargets = sources.map((source) => source.metadata.id);

  if (siblings.length > 0) {
    buildTargets = siblings;
  }

  let category: HardwareCategory;

  if (type === "shield") {
    category = "shield";
  } else if (features.includes("keys")) {
    category = "keyboard";
  } else {
    category = "controller";
  }

  const data: Record<string, unknown> & { name: string } = {
    ...primary.metadata,
    id: slug,
    name: commonNamePrefix(names),
    type,
    slug,
    sourcePath: primary.sourcePath,
    targets,
    buildTargets,
    category,
  };

  if (readmeSource?.readmePath) {
    data.readmePath = readmeSource.readmePath;
  }

  if (features.length > 0) {
    data.features = features;
  }

  if (exposes.length > 0) {
    data.exposes = exposes;
  }

  if (siblings.length > 0) {
    data.siblings = siblings;
  }

  if (revisions.length > 0) {
    data.revisions = revisions;
  }

  if (targetVariants.length > 0) {
    data.variants = targetVariants;
  }

  if (type === "board") {
    data.outputs = unique(
      sources.flatMap((source) => stringArray(source.metadata.outputs))
    );
  } else {
    data.requires = unique(
      sources.flatMap((source) => stringArray(source.metadata.requires))
    );
  }

  if (defaultRevision !== undefined) {
    data.default_revision = defaultRevision;
  }

  return data;
}

/**
 * Load boards and shields into the `hardware` collection.
 * Targets that share a board name become one entry; READMEs are rendered as the
 * entry body relative to their source file.
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

    async function storeInterconnect(
      file: string,
      metadata: Metadata,
      metadataId: string,
      sourcePath: string,
      contents: string
    ) {
      const data = await parseData({
        id: metadataId,
        data: {
          ...metadata,
          slug: metadata.id,
          sourcePath,
        },
        filePath: file,
      });
      const entry = {
        id: metadataId,
        data,
        filePath: sourcePath,
        digest: generateDigest(contents),
      };
      const guideline = readDesignGuideline(metadata);

      if (!guideline) {
        store.set(entry);
        return;
      }

      const rendered = await renderMarkdown(guideline, {
        fileURL: pathToFileURL(file),
      });

      store.set({
        ...entry,
        body: guideline,
        rendered,
        assetImports: rendered.metadata?.imagePaths ?? [],
      });
    }

    async function loadAll() {
      store.clear();

      const files = (
        await glob(METADATA_GLOB, { cwd: boardsRoot, absolute: true })
      ).sort();
      const sources: Source[] = [];
      const metadataIds = new Set<string>();
      let interconnectCount = 0;

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
          const type = metadata.type;

          if (
            type !== "board" &&
            type !== "shield" &&
            type !== "interconnect"
          ) {
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

          const metadataId = `${type}/${metadata.id}`;

          if (metadataIds.has(metadataId)) {
            throw new Error(
              `Duplicate ${type} "${metadata.id}" in ${sourcePath}.`
            );
          }

          metadataIds.add(metadataId);

          // Interconnects share the collection but are not part of the catalog.
          if (type === "interconnect") {
            await storeInterconnect(
              file,
              metadata,
              metadataId,
              sourcePath,
              contents
            );
            interconnectCount += 1;
            continue;
          }

          const target: HardwareTarget = {
            ...metadata,
            id: metadata.id,
            name: metadata.name,
            type,
          };

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
        const slug = hardwareSlug(type, first.metadata.id);
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
        const data = buildData(type, slug, group, primary, readmeSource);

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
        `Loaded ${groups.size} board/shield entries and ${interconnectCount} interconnects from ${files.length} metadata files.`
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
