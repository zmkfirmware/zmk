import { fileURLToPath } from "node:url";

import react from "@astrojs/react";
import sitemap from "@astrojs/sitemap";
import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import starlightBlog from "starlight-blog";

import { blogAuthors } from "./blog-authors.mjs";
import {
  copyHardwareMetadataToDist,
  generateHardwareMetadata,
} from "./scripts/generate-hardware-metadata.mjs";
import { sidebar } from "./src/lib/sidebar.mjs";

// Generate the hardware metadata before Astro loads content collections.
await generateHardwareMetadata();

const hardwareMetadata = {
  name: "zmk-hardware-metadata",
  hooks: {
    "astro:build:done": async ({ dir }) => {
      await copyHardwareMetadataToDist(fileURLToPath(dir));
    },
  },
};

export default defineConfig({
  site: "https://zmk.dev",
  trailingSlash: "never",
  build: {
    format: "directory",
  },
  vite: {
    resolve: {
      alias: {
        "@": fileURLToPath(new URL("./src", import.meta.url)),
        "@data": fileURLToPath(new URL("./src/data", import.meta.url)),
        "@components": fileURLToPath(
          new URL("./src/components", import.meta.url)
        ),
      },
    },
  },
  integrations: [
    react(),
    starlight({
      title: "ZMK Firmware",
      description: "Modern, open source keyboard firmware",
      logo: {
        src: "./src/assets/zmk_logo.svg",
        alt: "ZMK Logo",
      },
      favicon: "/img/favicon.ico",
      social: [
        {
          icon: "github",
          label: "ZMK GitHub Repository",
          href: "https://github.com/zmkfirmware/zmk",
        },
      ],
      editLink: {
        baseUrl: "https://github.com/zmkfirmware/zmk/blob/main/docs/",
      },
      routeMiddleware: "./src/routeData.ts",
      sidebar,
      expressiveCode: {
        themes: ["github-light", "github-dark-dimmed"],
        useStarlightUiThemeColors: true,
      },
      components: {
        Header: "./src/components/Header.astro",
        Footer: "./src/components/Footer.astro",
      },
      head: [
        {
          tag: "script",
          attrs: {
            async: true,
            src: "https://www.googletagmanager.com/gtag/js?id=UA-145201102-2",
          },
        },
        {
          tag: "script",
          content:
            "window.dataLayer=window.dataLayer||[];function gtag(){dataLayer.push(arguments);}gtag('js',new Date());gtag('config','UA-145201102-2',{anonymize_ip:true});",
        },
      ],
      plugins: [
        starlightBlog({
          title: "Blog",
          prefix: "blog",
          postCount: 10,
          recentPostCount: 100,
          navigation: "none",
          authors: blogAuthors,
          metrics: {
            readingTime: true,
          },
          rss: true,
        }),
      ],
    }),
    sitemap(),
    hardwareMetadata,
  ],
});
