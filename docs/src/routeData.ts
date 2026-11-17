import {
  defineRouteMiddleware,
  type StarlightRouteData,
} from "@astrojs/starlight/route-data";

import { isDevelopmentVersion, releaseVersions } from "./lib/versions.mjs";

function isDocsPage(id: string) {
  return id === "docs" || id.startsWith("docs/");
}

/**
 * Add the development-version warning to Starlight's built-in banner data.
 */
function addDevelopmentBanner(route: StarlightRouteData) {
  if (!isDevelopmentVersion || !isDocsPage(route.id)) return;

  const latestVersion = releaseVersions[0];
  const releaseUrl = `https://v${latestVersion.replaceAll(".", "-")}-branch.zmk.dev/docs/`;
  const devBanner = `You're viewing the documentation for the development version of ZMK. You may want the latest release <a href="${releaseUrl}">v${latestVersion}</a>.`;

  const existingBanner = route.entry.data.banner?.content;
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
