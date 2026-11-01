import type { CollectionEntry } from "astro:content";

/** Shared helpers for the hardware catalog built from `app/boards`. */

export type HardwareKind = "board" | "shield";

export type HardwareCollectionEntry = CollectionEntry<"hardware">;

/** A hardware collection entry that is a board or shield, not an interconnect. */
export type HardwareDeviceEntry = Omit<HardwareCollectionEntry, "data"> & {
  data: Extract<HardwareCollectionEntry["data"], { type: HardwareKind }>;
};

export function isHardwareDeviceEntry(
  entry: HardwareCollectionEntry
): entry is HardwareDeviceEntry {
  return entry.data.type === "board" || entry.data.type === "shield";
}

export const BOARD_CATEGORIES = ["controller", "keyboard"] as const;

export const HARDWARE_CATEGORIES = [...BOARD_CATEGORIES, "shield"] as const;

export type HardwareCategory = (typeof HARDWARE_CATEGORIES)[number];

export const HARDWARE_CATEGORY_LABELS: Record<HardwareCategory, string> = {
  controller: "Controller Board",
  keyboard: "Keyboard Board",
  shield: "Shield",
};

export const HARDWARE_FEATURES = [
  "keys",
  "display",
  "encoder",
  "underglow",
  "backlight",
  "pointer",
  "studio",
] as const;

export type HardwareFeature = (typeof HARDWARE_FEATURES)[number];

/**
 * Return the Zephyr board name from a target id, dropping revision and
 * qualifiers (`nrf5340dk/nrf5340/cpuapp` -> `nrf5340dk`).
 */
export function hardwareBoardName(id: string): string {
  return id.split("@")[0].split("/")[0];
}

/** Convert a hardware metadata id into a URL-safe catalog slug. */
export function hardwareSlug(type: HardwareKind, id: string): string {
  const name = type === "board" ? hardwareBoardName(id) : id;

  return name.replace(/[@/]+/g, "-");
}

/** Build a GitHub link to a file in the ZMK repository. */
export function hardwareSourceUrl(path: string): string {
  return `https://github.com/zmkfirmware/zmk/blob/main/${path}`;
}

export function hardwareVariantIds(
  variants: readonly (string | { id: string })[] | undefined
): string[] {
  return (
    variants?.map((variant) =>
      typeof variant === "string" ? variant : variant.id
    ) ?? []
  );
}

/**
 * Find other entries from the same metadata directory, such as board revisions.
 * Split siblings are parts of one device and are not included here.
 */
export function hardwareRelatedEntries(
  entry: HardwareDeviceEntry,
  entries: HardwareDeviceEntry[]
): HardwareDeviceEntry[] {
  const directory = entry.data.sourcePath.substring(
    0,
    entry.data.sourcePath.lastIndexOf("/")
  );

  return entries
    .filter(
      (candidate) =>
        candidate.data.type === entry.data.type &&
        candidate.data.slug !== entry.data.slug &&
        candidate.data.sourcePath.substring(
          0,
          candidate.data.sourcePath.lastIndexOf("/")
        ) === directory
    )
    .sort((a, b) => a.data.name.localeCompare(b.data.name));
}
