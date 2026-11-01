import { defineCollection } from "astro:content";
import { z } from "astro/zod";
import { docsLoader } from "@astrojs/starlight/loaders";
import { docsSchema } from "@astrojs/starlight/schema";
import { blogSchema } from "starlight-blog/schema";

import { BOARD_CATEGORIES } from "./lib/hardware-catalog";
import { hardwareLoader } from "./loaders/hardware-catalog";

const featureSchema = z.enum([
  "keys",
  "display",
  "encoder",
  "underglow",
  "backlight",
  "pointer",
  "studio",
]);

const variantSchema = z.union([
  z.string(),
  z.looseObject({
    id: z.string(),
    features: z.array(featureSchema),
  }),
]);

const interconnectSchema = z.array(z.string());

const targetSchema = z.looseObject({
  id: z.string(),
  name: z.string(),
  sourcePath: z.string(),
  readmePath: z.string().optional(),
});

const hardwareBaseSchema = {
  file_format: z.literal("1"),
  id: z.string(),
  name: z.string(),
  url: z.string(),
  description: z.string().optional(),
  manufacturer: z.string().optional(),
  version: z.string().optional(),
  features: z.array(featureSchema).optional(),
  variants: z.array(variantSchema).optional(),
  siblings: z.array(z.string()).optional(),
  slug: z.string(),
  sourcePath: z.string(),
  readmePath: z.string().optional(),
  targets: z.array(targetSchema),
  buildTargets: z.array(z.string()),
};

const boardSchema = z.looseObject({
  ...hardwareBaseSchema,
  type: z.literal("board"),
  category: z.enum(BOARD_CATEGORIES),
  arch: z.string(),
  outputs: z.array(z.enum(["usb", "ble"])),
  exposes: interconnectSchema.optional(),
  revisions: z.array(z.string()).optional(),
  default_revision: z.string().optional(),
});

const shieldSchema = z.looseObject({
  ...hardwareBaseSchema,
  type: z.literal("shield"),
  category: z.literal("shield"),
  requires: interconnectSchema,
  exposes: interconnectSchema.optional(),
});

export const collections = {
  docs: defineCollection({
    loader: docsLoader(),
    schema: docsSchema({
      extend: (context) => blogSchema(context),
    }),
  }),
  hardware: defineCollection({
    loader: hardwareLoader,
    schema: z.discriminatedUnion("type", [boardSchema, shieldSchema]),
  }),
};
