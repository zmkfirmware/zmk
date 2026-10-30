import {
  defineRouteMiddleware,
  type StarlightRouteData,
} from "@astrojs/starlight/route-data";

import { groupedMetadata } from "./components/hardware-utils";
import Metadata from "./data/hardware-metadata.json";
import type { HardwareMetadata } from "./hardware-metadata";
import { isDevelopmentVersion, releaseVersions } from "./lib/versions.mjs";

// The generated JSON has widened literal fields such as `file_format`.
const hardwareMetadata = Metadata as HardwareMetadata[];

/**
 * Restore the table of contents for the hardware page.
 *
 * Its headings are rendered by the `HardwareList` component from generated
 * hardware metadata, plus one raw HTML `<h2>`, so Starlight's Markdown-based
 * heading extraction cannot see them. The pre-migration Docusaurus page used a
 * custom `export const toc` for the same reason; Starlight has no equivalent,
 * so we add the entries to the route data here.
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

/**
 * Add the development-version warning to Starlight's built-in banner data.
 *
 * The default Banner component renders `entry.data.banner.content` and owns
 * all of the banner markup and styling. Injecting the warning here means we do
 * not need to override or duplicate that component.
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
  addHardwareToc(route);
  addDevelopmentBanner(route);
});
