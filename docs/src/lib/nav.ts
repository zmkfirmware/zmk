import { isDevelopmentVersion } from "virtual:zmk/version-info";

/**
 * Single source of truth for the primary navigation.
 *
 * Docs, Blog, and Tools are the top-level destinations on this branch. The
 * Hardware destination is added by the hardware branch; the layout and
 * breakpoints already reserve room for it. Tools is a group: it has no page of
 * its own, only children.
 *
 * The header renders this model inline at >=72rem, in the tablet dropdown
 * between 50rem and 72rem, and inside the mobile drawer below 50rem. If a
 * fifth destination makes the inline row tight, raise the nav-inline
 * breakpoint (72rem -> 80rem) in Header.astro and NavLinks.astro; the tablet
 * dropdown already covers that range.
 */

export interface NavChild {
  label: string;
  href: string;
}

export interface NavItem {
  label: string;
  href?: string;
  children?: NavChild[];
  /** Whether this destination is the current section for the given pathname. */
  isActive: (pathname: string) => boolean;
}

const blogHref = isDevelopmentVersion ? "/blog" : "https://zmk.dev/blog";

const toolChildren: NavChild[] = [
  { label: "Power Profiler", href: "/power-profiler" },
  { label: "Keymap Upgrader", href: "/keymap-upgrader" },
];

/**
 * Normalise an Astro pathname for navigation matching. Production builds use
 * `.html` file names (see `build.format` in astro.config.mjs), so both the
 * extension and any trailing slash are stripped.
 */
export const normalizePathname = (pathname: string) =>
  pathname
    .replace(/\.html$/, "")
    .replace(/\/index$/, "/")
    .replace(/\/+$/, "") || "/";

/** Match a pathname against a site-absolute href, ignoring trailing slashes. */
export const isPath = (pathname: string, href: string) =>
  href.startsWith("/") &&
  (pathname === href || pathname.startsWith(href + "/"));

export const navItems: NavItem[] = [
  {
    label: "Docs",
    href: "/docs",
    isActive: (pathname) => isPath(pathname, "/docs"),
  },
  {
    label: "Blog",
    href: blogHref,
    isActive: (pathname) => isDevelopmentVersion && isPath(pathname, "/blog"),
  },
  {
    label: "Tools",
    children: toolChildren,
    isActive: (pathname) =>
      toolChildren.some((child) => isPath(pathname, child.href)),
  },
];
