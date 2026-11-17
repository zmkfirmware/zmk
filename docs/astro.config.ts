import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";

import react from "@astrojs/react";
import sitemap from "@astrojs/sitemap";
import starlight from "@astrojs/starlight";
import { defineConfig } from "astro/config";
import starlightBlog from "starlight-blog";
import mermaid from "astro-mermaid";
import starlightLinksValidator from "starlight-links-validator";

import { blogAuthors } from "./src/data/blog-authors";

const readGrammar = (name: string) =>
  JSON.parse(
    readFileSync(new URL(`./src/grammars/${name}`, import.meta.url), "utf8")
  );

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
        starlightLinksValidator({
          exclude: [
            // Validator doesn't know about the custom pages
            "/power-profiler",
            "/keymap-upgrader",
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
