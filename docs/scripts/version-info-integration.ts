/**
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */
import type { AstroIntegration } from "astro";

import { resolveVersionInfo } from "./version-info.ts";

const virtualVersionInfoId = "virtual:zmk/version-info";
const versionInfo = resolveVersionInfo();

const versionInfoModule = [
  `export const branch = ${JSON.stringify(versionInfo.branch)};`,
  `export const isDevelopmentVersion = ${JSON.stringify(versionInfo.isDevelopmentVersion)};`,
  `export const releaseVersions = ${JSON.stringify(versionInfo.releaseVersions)};`,
  `export const currentVersion = ${JSON.stringify(versionInfo.currentVersion)};`,
  `export const versionNavbarItems = ${JSON.stringify(versionInfo.versionNavbarItems)};`,
  `export const versionDropdownLabel = ${JSON.stringify(versionInfo.versionDropdownLabel)};`,
].join("\n");

const versionInfoPlugin = {
  name: "vite-plugin-zmk-version-info",
  resolveId(id: string) {
    if (id === virtualVersionInfoId) return virtualVersionInfoId;
  },
  load(id: string) {
    if (id === virtualVersionInfoId) {
      return versionInfoModule;
    }
  },
};

const virtualVersionInfoTypes = `declare module "virtual:zmk/version-info" {
  export interface VersionItem {
    label: string;
    href: string;
    version?: string;
    current: boolean;
  }

  export const branch: string;
  export const isDevelopmentVersion: boolean;
  export const releaseVersions: string[];
  export const currentVersion: VersionItem;
  export const versionNavbarItems: VersionItem[];
  export const versionDropdownLabel: string;
}`;

export const versionInfoIntegration: AstroIntegration = {
  name: "zmk-version-info",
  hooks: {
    "astro:config:setup"({ updateConfig }) {
      updateConfig({ vite: { plugins: [versionInfoPlugin] } });
    },
    "astro:config:done"({ injectTypes }) {
      injectTypes({
        filename: "zmk-version-info.d.ts",
        content: virtualVersionInfoTypes,
      });
    },
  },
};
