module.exports = {
  title: "ZMK Keyboard Firmware",
  tagline: "Modern, open source keyboard firmware ",
  url: "https://your-docusaurus-test-site.com",
  baseUrl: "/",
  favicon: "img/favicon.ico",
  organizationName: "zmkproject", // Usually your GitHub org/user name.
  projectName: "zmk", // Usually your repo name.
  themeConfig: {
    navbar: {
      title: "ZMK Firmware",
      logo: {
        alt: "ZMK Logo",
        src: "img/zmk_logo.png",
      },
      links: [
        {
          to: "docs/",
          activeBasePath: "docs",
          label: "Docs",
          position: "left",
        },
        { to: "blog", label: "Blog", position: "left" },
        {
          href: "https://gitlab.com/zmkproject/zmk",
          label: "GitLab",
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
              to: "docs/dev-setup/",
            },
          ],
        },
        {
          // title: "Community",
          // items: [
          //   {
          //     label: "Stack Overflow",
          //     href: "https://stackoverflow.com/questions/tagged/docusaurus",
          //   },
          //   {
          //     label: "Discord",
          //     href: "https://discordapp.com/invite/docusaurus",
          //   },
          //   {
          //     label: "Twitter",
          //     href: "https://twitter.com/docusaurus",
          //   },
          // ],
        },
        {
          title: "More",
          items: [
            {
              label: "Blog",
              to: "blog",
            },
            {
              label: "GitLab",
              href: "https://gitlab.com/zmkproject/zmk",
            },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} ZMK Project Contributors, Built with Docusaurus.`,
    },
  },
  presets: [
    [
      "@docusaurus/preset-classic",
      {
        docs: {
          // It is recommended to set document id as docs home page (`docs/` path).
          homePageId: "intro",
          sidebarPath: require.resolve("./sidebars.js"),
          // Please change this to your repo.
          editUrl: "https://githlab.com/zmkproject/zmk/edit/master/docs/",
        },
        blog: {
          showReadingTime: true,
          // Please change this to your repo.
          editUrl: "https://gitlab.com/zmkproject/zmk/edit/master/docs/blog/",
        },
        theme: {
          customCss: require.resolve("./src/css/custom.css"),
        },
      },
    ],
  ],
};
