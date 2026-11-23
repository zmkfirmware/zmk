/**
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */
import { isDevelopmentVersion } from "virtual:zmk/version-info";

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

/** Normalise an Astro pathname for navigation matching. */
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
