const path = require("path");

module.exports = {
  title: "ZMK Firmware",
  tagline: "Modern, open source keyboard firmware",
  url: "https://zmk.dev",
  baseUrl: "/",
  favicon: "img/favicon.ico",
  trailingSlash: "false",
  organizationName: "zmkfirmware", // Usually your GitHub org/user name.
  projectName: "zmk", // Usually your repo name.
  plugins: [
    path.resolve(__dirname, "src/docusaurus-tree-sitter-plugin"),
    path.resolve(__dirname, "src/hardware-metadata-collection-plugin"),
    path.resolve(__dirname, "src/hardware-metadata-static-plugin"),
    path.resolve(__dirname, "src/hardware-schema-typescript-plugin"),
    path.resolve(__dirname, "src/setup-script-generation-plugin"),
  ],
  themeConfig: {
    colorMode: {
      respectPrefersColorScheme: true,
    },
    // sidebarCollapsible: false,
    navbar: {
      title: "ZMK Firmware",
      logo: {
        alt: "ZMK Logo",
        src: "img/zmk_logo.svg",
      },
      items: [
        {
          to: "docs",
          activeBasePath: "docs",
          label: "Docs",
          position: "left",
        },
        { to: "blog", label: "Blog", position: "left" },
        {
          to: "power-profiler",
          label: "Power Profiler",
          position: "left",
        },
        {
          href: "https://github.com/zmkfirmware/zmk",
          label: "GitHub",
          position: "right",
        },
      ],
    },
    footer: {
      style: "dark",
      links: [
        {
          title: "Docs",
          items: [
            {
              label: "Getting Started",
              to: "docs/",
            },
            {
              label: "Development",
              to: "docs/development/setup/",
            },
          ],
        },
        {
          title: "Community",
          items: [
            //   {
            //     label: "Stack Overflow",
            //     href: "https://stackoverflow.com/questions/tagged/docusaurus",
            //   },
            {
              label: "Discord",
              href:
                (process.env.URL || "https://zmk.dev") +
                "/community/discord/invite",
            },
            {
              label: "Twitter",
              href: "https://twitter.com/ZMKFirmware",
            },
          ],
        },
        {
          title: "More",
          items: [
            {
              label: "Blog",
              to: "blog",
            },
            {
              label: "GitHub",
              href: "https://github.com/zmkfirmware/zmk",
            },
            {
              html: `
                <a href="https://www.netlify.com" target="_blank" rel="noreferrer noopener" aria-label="Deploys by Netlify">
                  <img src="https://www.netlify.com/img/global/badges/netlify-color-accent.svg" alt="Deploys by Netlify" />
                </a>
              `,
            },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} ZMK Project Contributors. <a rel="license" href="http://creativecommons.org/licenses/by-nc-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc-sa/4.0/80x15.png" /></a>`,
    },
    algolia: {
      appId: "USXLDJ14JE",
      apiKey: "384a3bd2d50136c9dc8c8ddfe1b3a4b2",
      indexName: "zmkfirmware",
    },
  },
  presets: [
    [
      "@docusaurus/preset-classic",
      {
        googleAnalytics: {
          trackingID: "UA-145201102-2",
          anonymizeIP: true,
        },
        docs: {
          // Removed (for now) until we have content for each level of the generated breadcrumbs
          breadcrumbs: false,
          // It is recommended to set document id as docs home page (`docs/` path).
          sidebarPath: require.resolve("./sidebars.js"),
          // Please change this to your repo.
          editUrl: "https://github.com/zmkfirmware/zmk/edit/main/docs/",
        },
        blog: {
          showReadingTime: true,
          // Please change this to your repo.
          editUrl: "https://github.com/zmkfirmware/zmk/edit/main/docs/",
        },
        theme: {
          customCss: [
            require.resolve("./src/css/custom.css"),
            require.resolve("./src/css/codes.css"),
          ],
        },
      },
    ],
  ],
};
