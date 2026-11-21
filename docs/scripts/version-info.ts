import { execFileSync } from "node:child_process";
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

const repoRoot = fileURLToPath(new URL("../..", import.meta.url));
const defaultReleaseVersionsFile = fileURLToPath(
  new URL("../src/data/release-versions.json", import.meta.url)
);

const releaseVersionPattern = /^(0|[1-9]\d*)\.(0|[1-9]\d*)$/;
const releaseBranchPattern = /^v((?:0|[1-9]\d*)\.(?:0|[1-9]\d*))-branch$/;

const branchEnvKeys = [
  // Netlify: read-only build variable with the ref to check out
  "BRANCH",
  // Netlify: head branch name reported by the Git provider
  "HEAD",
  // GitHub Actions
  "GITHUB_REF_NAME",
  // Cloudflare Pages
  "CF_PAGES_BRANCH",
  // Vercel
  "VERCEL_GIT_COMMIT_REF",
  // GitLab CI/CD
  "CI_COMMIT_REF_NAME",
] as const;

export interface VersionItem {
  label: string;
  href: string;
  version?: string;
  current: boolean;
}

export interface VersionInfo {
  branch: string;
  isDevelopmentVersion: boolean;
  releaseVersions: string[];
  currentVersion: VersionItem;
  versionNavbarItems: VersionItem[];
  versionDropdownLabel: string;
}

export interface CreateVersionInfoInput {
  branch: string;
  releaseVersions: unknown;
}

export interface ResolveVersionInfoOptions {
  env?: Record<string, string | undefined>;
  cwd?: string;
  releaseVersionsFile?: string;
}

/** Strip the ref forms down to a plain branch name. */
export function normalizeBranchName(value: unknown): string {
  if (typeof value !== "string") return "";

  const branch = value
    .trim()
    .replace(/^refs\/heads\//, "")
    .replace(/^refs\/remotes\/[^/]+\//, "")
    .replace(/^remotes\/[^/]+\//, "")
    .replace(/^origin\//, "");

  return branch === "HEAD" ? "" : branch;
}

/** `X.Y` from a `vX.Y-branch` name. */
export function parseReleaseBranch(branch: string): string | null {
  const match = releaseBranchPattern.exec(branch);
  return match ? match[1] : null;
}

/** Newest first, compared numerically so `0.10` sorts above `0.9`. */
export function compareReleaseVersions(a: string, b: string): number {
  const [aMajor = 0, aMinor = 0] = a.split(".").map(Number);
  const [bMajor = 0, bMinor = 0] = b.split(".").map(Number);
  return bMajor - aMajor || bMinor - aMinor;
}

export function normalizeReleaseVersions(value: unknown): string[] {
  if (!Array.isArray(value)) {
    throw new TypeError(
      '"releaseVersions" in src/data/release-versions.json must be an array.'
    );
  }

  const versions: string[] = [];
  const entries: unknown[] = value;

  for (const entry of entries) {
    if (typeof entry !== "string") {
      throw new TypeError(
        `Invalid release version ${JSON.stringify(entry)} in src/data/release-versions.json: expected an "X.Y" string.`
      );
    }

    const version = entry.trim();
    if (!releaseVersionPattern.test(version)) {
      throw new TypeError(
        `Invalid release version ${JSON.stringify(entry)} in src/data/release-versions.json: expected an "X.Y" string.`
      );
    }

    if (!versions.includes(version)) versions.push(version);
  }

  return versions.sort(compareReleaseVersions);
}

/** Build the version selector model. */
export function createVersionInfo({
  branch,
  releaseVersions,
}: CreateVersionInfoInput): VersionInfo {
  const normalizedBranch = normalizeBranchName(branch) || "main";
  const versions = normalizeReleaseVersions(releaseVersions);
  const releaseVersion = parseReleaseBranch(normalizedBranch);
  const isDevelopmentVersion = releaseVersion === null;

  if (releaseVersion && !versions.includes(releaseVersion)) {
    versions.push(releaseVersion);
    versions.sort(compareReleaseVersions);
  }

  const preReleaseItem: VersionItem = {
    label: "Pre-Release",
    href: "https://zmk.dev/docs",
    current: false,
  };

  const releaseItems: VersionItem[] = versions.map((version) => ({
    label: `v${version}`,
    href: `https://v${version.replaceAll(".", "-")}-branch.zmk.dev/docs`,
    version,
    current: false,
  }));

  const currentItem = isDevelopmentVersion
    ? preReleaseItem
    : (releaseItems.find((item) => item.version === releaseVersion) ??
      preReleaseItem);

  const versionNavbarItems = [preReleaseItem, ...releaseItems].map((item) => ({
    ...item,
    current: item === currentItem,
  }));

  const currentVersion = versionNavbarItems.find(
    (item) => item.label === currentItem.label
  ) ?? {
    ...currentItem,
    current: true,
  };

  return {
    branch: normalizedBranch,
    isDevelopmentVersion,
    releaseVersions: versions,
    currentVersion,
    versionNavbarItems,
    versionDropdownLabel: currentVersion.label,
  };
}

export function readReleaseVersions(
  file: string = defaultReleaseVersionsFile
): string[] {
  const data = JSON.parse(readFileSync(file, "utf8")) as {
    releaseVersions?: unknown;
  } | null;
  return normalizeReleaseVersions(data?.releaseVersions);
}

/** Current branch from git, or `""` when detached or unavailable. */
export function resolveGitBranch(cwd: string = repoRoot): string {
  const commands: string[][] = [
    ["branch", "--show-current"],
    ["rev-parse", "--abbrev-ref", "HEAD"],
  ];

  for (const args of commands) {
    try {
      const branch = normalizeBranchName(
        execFileSync("git", args, {
          cwd,
          encoding: "utf8",
          stdio: ["ignore", "pipe", "ignore"],
        })
      );
      if (branch) return branch;
    } catch {
      // No git, or a detached HEAD.
    }
  }

  return "";
}

/** Resolve version data for the current build. */
export function resolveVersionInfo({
  env = process.env,
  cwd = repoRoot,
  releaseVersionsFile = defaultReleaseVersionsFile,
}: ResolveVersionInfoOptions = {}): VersionInfo {
  const envBranch = branchEnvKeys
    .map((key) => normalizeBranchName(env[key]))
    .find(Boolean);
  const branch = envBranch || resolveGitBranch(cwd) || "main";

  return createVersionInfo({
    branch,
    releaseVersions: readReleaseVersions(releaseVersionsFile),
  });
}
