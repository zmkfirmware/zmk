/**
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */
import {
  defineRouteMiddleware,
  type StarlightRouteData,
} from "@astrojs/starlight/route-data";

import {
  isDevelopmentVersion,
  releaseVersions,
} from "virtual:zmk/version-info";

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
  addDevelopmentBanner(route);
});
