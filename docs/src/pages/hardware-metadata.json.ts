import { getCollection } from "astro:content";
import type { APIRoute } from "astro";

export const prerender = true;

/**
 * Remove fields that are added to the content collection entries by the
 * hardware loader, so the response keeps the shape of the old generated file.
 */
function withoutKeys(value: object, keys: readonly string[]) {
  return Object.fromEntries(
    Object.entries(value).filter(([key]) => !keys.includes(key))
  );
}

/**
 * Serve the aggregated hardware metadata from the content collection.
 *
 * The site used to generate `src/data/hardware-metadata.json` before the Astro
 * build and copy it to `/hardware-metadata.json`. The content collection is now
 * the source of truth, so the endpoint flattens its entries back into the same
 * per-target records.
 */
export const GET: APIRoute = async () => {
  const entries = await getCollection("hardware");

  const metadata = entries.flatMap((entry) => {
    if (entry.data.type === "interconnect") {
      return [withoutKeys(entry.data, ["slug", "sourcePath"])];
    }

    return entry.data.targets.map((target) =>
      withoutKeys(target, ["sourcePath", "readmePath"])
    );
  });

  return new Response(JSON.stringify(metadata), {
    headers: { "Content-Type": "application/json" },
  });
};
