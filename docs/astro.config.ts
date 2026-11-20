import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

import react from "@astrojs/react";
import sitemap from "@astrojs/sitemap";
import starlight from "@astrojs/starlight";
import type { StarlightPlugin } from "@astrojs/starlight/types";
import { defineConfig } from "astro/config";
import starlightBlog from "starlight-blog";
import mermaid from "astro-mermaid";
import starlightLinksValidator from "starlight-links-validator";

import { versionInfoIntegration } from "./scripts/version-info-integration.ts";
import { blogAuthors } from "./src/data/blog-authors";
import { sidebar } from "./src/data/sidebar";

const readGrammar = (name: string) =>
  JSON.parse(
    readFileSync(new URL(`./src/grammars/${name}`, import.meta.url), "utf8")
  );

const blogPrefix = "blog";

// starlight-blog adds an RSS link to Starlight's social icons when the blog feed is enabled.
// Keep the feed itself and its <link rel="alternate"> in the page head,
// but remove the social icon the plugin injects.
// This runs after `starlightBlog` below.
const removeBlogRssSocial: StarlightPlugin = {
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
  // Emit flat `name.html` files so Netlify serves extensionless URLs and
  // redirects `/x/` to `/x`.
  // Starlight is patched (see patches/) to strip `.html` from canonical and
  // Pagefind result URLs under this format.
  // Also see: https://github.com/withastro/starlight/discussions/3600
  build: {
    format: "preserve",
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
    versionInfoIntegration,
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
      customCss: ["./src/styles/site.css"],
      components: {
        Header: "./src/components/starlight/Header.astro",
        Footer: "./src/components/starlight/Footer.astro",
        PageFrame: "./src/components/starlight/PageFrame.astro",
        PageTitle: "./src/components/starlight/PageTitle.astro",
        SocialIcons: "./src/components/starlight/SocialIcons.astro",
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
        starlightLinksValidator({
          exclude: [
            // Validator doesn't know about the custom pages
            "/power-profiler",
            "/keymap-upgrader",
            "/catalog/hardware",
            "/catalog/hardware/**",
            // Validator doesn't know about the hash, because it's from custom HTML, not markdown
            "/docs/hardware#composite",
            "/docs/hardware#onboard",
            // Cross-file includes the validator can't check in these files
            // docs/hardware-integration/includes/_sideband-direct.mdx
            // docs/hardware-integration/includes/_sideband-wakeup-direct.mdx
            "#gpio-key",
          ],
        }),
      ],
    }),
    sitemap(),
  ],
});
