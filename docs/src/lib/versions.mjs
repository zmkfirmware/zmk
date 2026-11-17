import { execSync } from "node:child_process";

import releaseVersionsData from "../data/release-versions.json";

const releaseVersions = releaseVersionsData.releaseVersions;

let gitBranch = process.env.BRANCH;
if (!gitBranch) {
  try {
    gitBranch = execSync("git branch --show-current", {
      encoding: "utf-8",
    }).trim();
  } catch {
    gitBranch = "main";
  }
}

export const branch = gitBranch;
export { releaseVersions };

export const isDevelopmentVersion =
  branch === "main" || !/^v\d+\.\d+-branch$/.test(branch);

export const versionNavbarItems = [
  {
    label: "Pre-Release",
    description: "Current active development branch (main)",
    href: "https://zmk.dev/docs/",
  },
  ...releaseVersions.map((r) => ({
    label: "v" + r,
    description: "Stable release v" + r,
    href: `https://v${r.replaceAll(".", "-")}-branch.zmk.dev/docs/`,
  })),
];

const matchingItem = versionNavbarItems.find(
  (item) => item.label === branch.replace("-branch", "")
);

export const versionDropdownLabel = (
  isDevelopmentVersion
    ? versionNavbarItems[0]
    : (matchingItem ?? versionNavbarItems[0])
).label;
