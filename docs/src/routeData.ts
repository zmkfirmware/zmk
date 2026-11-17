/**
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */
import {
  defineRouteMiddleware,
  type StarlightRouteData,
} from "@astrojs/starlight/route-data";

import { groupedMetadata } from "./lib/hardware";
import Metadata from "./data/hardware-metadata.json";
import type { HardwareMetadata } from "./hardware-metadata";
import {
  isDevelopmentVersion,
  releaseVersions,
} from "virtual:zmk/version-info";

// The generated JSON has widened literal fields such as `file_format`.
const hardwareMetadata = Metadata as HardwareMetadata[];

/**
 * Set the table of contents for the hardware page.
 */
function addHardwareToc(route: StarlightRouteData) {
  if (route.id !== "docs/hardware" || !route.toc) return;

  const [overview] = route.toc.items;
  if (!overview) return;

  const interconnectItems = Object.values(
    groupedMetadata(hardwareMetadata).interconnects
  ).flatMap(({ interconnect }) =>
    interconnect
      ? [
          {
            depth: 3,
            slug: interconnect.id,
            text: `${interconnect.name} Interconnect`,
            children: [],
          },
        ]
      : []
  );

  route.toc.items = [
    overview,
    {
      depth: 2,
      slug: "composite",
      text: "Composite Keyboards",
      children: interconnectItems,
    },
    {
      depth: 2,
      slug: "onboard",
      text: "Onboard Controller Keyboards",
      children: [],
    },
    {
      depth: 2,
      slug: "other-hardware",
      text: "Other Hardware",
      children: [],
    },
  ];
}

function isDocsPage(id: string) {
  return id === "docs" || id.startsWith("docs/");
}

const developmentBannerText =
  "You're viewing the documentation for the development version of ZMK.";

/**
 * Add the development version note to Starlight's built-in banner data.
 */
function addDevelopmentBanner(route: StarlightRouteData) {
  if (!isDevelopmentVersion || !isDocsPage(route.id)) return;

  const latestVersion = releaseVersions[0];
  if (!latestVersion) return;

  const existingBanner = route.entry.data.banner?.content;
  if (existingBanner?.includes(developmentBannerText)) return;

  const releaseUrl = `https://v${latestVersion.replaceAll(".", "-")}-branch.zmk.dev/docs`;
  const devBanner = `${developmentBannerText} You may want the latest release <a href="${releaseUrl}">v${latestVersion}</a>.`;

  route.entry.data.banner = {
    content: existingBanner ? `${devBanner} ${existingBanner}` : devBanner,
  };
}

/**
 * Apply the site's Starlight route-data customizations.
 */
export const onRequest = defineRouteMiddleware(({ locals }) => {
  const route = locals.starlightRoute;
  addHardwareToc(route);
  addDevelopmentBanner(route);
});
