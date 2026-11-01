import { defineCollection } from "astro:content";
import { z } from "astro/zod";
import { docsLoader } from "@astrojs/starlight/loaders";
import { docsSchema } from "@astrojs/starlight/schema";
import { blogSchema } from "starlight-blog/schema";

import { BOARD_CATEGORIES, HARDWARE_FEATURES } from "./lib/hardware-catalog";
import { hardwareLoader } from "./loaders/hardware-catalog";

// Mirrors schema/hardware-metadata.schema.json; z.fromJSONSchema would lose the
// generated TypeScript types, so keep these constraints in sync manually.
const idSchema = z
  .string()
  .regex(/^[a-z0-9_/]+(@([A-Z]|[0-9]+|([0-9]+(\.[0-9]+){1,2})))?$/);
const revisionSchema = z
  .string()
  .regex(/^([A-Z]|[0-9]+|[0-9]+(\.[0-9]+){1,2})$/);

const featureSchema = z.enum(HARDWARE_FEATURES);

const variantSchema = z.union([
  z.string(),
  z.looseObject({
    id: idSchema,
    features: z.array(featureSchema),
  }),
]);

const interconnectListSchema = z.array(idSchema).min(1);

const targetSchema = z.looseObject({
  id: idSchema,
  name: z.string(),
  sourcePath: z.string(),
  readmePath: z.string().optional(),
});

const metadataBaseSchema = {
  file_format: z.literal("1"),
  id: idSchema,
  name: z.string(),
  url: z.url(),
  description: z.string().optional(),
  manufacturer: z.string().optional(),
  version: z.string().optional(),
  features: z.array(featureSchema).optional(),
  variants: z.array(variantSchema).optional(),
  siblings: z.array(z.string()).optional(),
};

// Fields derived by the hardware loader.
const catalogSchema = {
  slug: z.string(),
  sourcePath: z.string(),
  readmePath: z.string().optional(),
  targets: z.array(targetSchema),
  buildTargets: z.array(z.string()),
};

const boardSchema = z.strictObject({
  ...metadataBaseSchema,
  ...catalogSchema,
  type: z.literal("board"),
  category: z.enum(BOARD_CATEGORIES),
  arch: z.string().regex(/^[a-z0-9_]+$/),
  outputs: z.array(z.enum(["usb", "ble"])),
  exposes: interconnectListSchema.optional(),
  revisions: z.array(revisionSchema).optional(),
  default_revision: revisionSchema.optional(),
});

const shieldSchema = z.strictObject({
  ...metadataBaseSchema,
  ...catalogSchema,
  type: z.literal("shield"),
  category: z.literal("shield"),
  requires: interconnectListSchema,
  exposes: interconnectListSchema.optional(),
});

const interconnectSchema = z.strictObject({
  file_format: z.literal("1"),
  id: idSchema,
  name: z.string(),
  version: z.string().optional(),
  url: z.url(),
  description: z.string(),
  node_labels: z
    .strictObject({
      gpio: z.string(),
      i2c: z.string().optional(),
      spi: z.string().optional(),
      uart: z.string().optional(),
      adc: z.string().optional(),
    })
    .optional(),
  design_guideline: z.string().optional(),
  manufacturer: z.string().optional(),
  type: z.literal("interconnect"),
  slug: z.string(),
  sourcePath: z.string(),
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
    schema: z.discriminatedUnion("type", [
      boardSchema,
      shieldSchema,
      interconnectSchema,
    ]),
  }),
};
