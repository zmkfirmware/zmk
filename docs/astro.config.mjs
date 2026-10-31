import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

import react from "@astrojs/react";
import sitemap from "@astrojs/sitemap";
import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import starlightBlog from "starlight-blog";
import mermaid from 'astro-mermaid';

import { blogAuthors } from "./blog-authors.mjs";
import {
  copyHardwareMetadataToDist,
  generateHardwareMetadata,
} from "./scripts/generate-hardware-metadata.mjs";
import { sidebar } from "./src/lib/sidebar.mjs";

const readGrammar = (name) =>
  JSON.parse(
    readFileSync(new URL(`./src/grammars/${name}`, import.meta.url), "utf8")
  );

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

const blogPrefix = "blog";

// starlight-blog adds an RSS link to Starlight's header social icons when the
// blog feed is enabled. Keep the feed itself and its <link rel="alternate"> in
// the page head, but drop the social icon the plugin injects. This runs after
// `starlightBlog` below, so it sees the social link added by that plugin.
const removeBlogRssSocial = {
  name: "zmk-remove-blog-rss-social",
  hooks: {
    "config:setup"({ astroConfig, config, updateConfig }) {
      if (!astroConfig.site) return;

      const rssHref = new URL(`/${blogPrefix}/rss.xml`, astroConfig.site).href;
      const social = config.social ?? [];
      const withoutBlogRss = social.filter(
        (link) => !(link.icon === "rss" && link.href === rssHref)
      );

      if (withoutBlogRss.length !== social.length) {
        updateConfig({ social: withoutBlogRss });
      }
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
    mermaid(),
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
        shiki: {
          langs: [
            readGrammar("dts.tmLanguage.json"),
            readGrammar("kconfig.tmLanguage.json"),
          ],
        },
      },
      components: {
        Header: "./src/components/starlight/Header.astro",
        Footer: "./src/components/starlight/Footer.astro",
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
          prefix: blogPrefix,
          postCount: 10,
          recentPostCount: 100,
          navigation: "none",
          authors: blogAuthors,
          metrics: {
            readingTime: true,
          },
          rss: true,
        }),
        removeBlogRssSocial,
      ],
    }),
    sitemap(),
    hardwareMetadata,
  ],
});
