/**
 * Shared helpers for the hardware catalog.
 *
 * The catalog is built from the hardware metadata files under `app/boards`.
 * Board routes use the Zephyr board name, which is the part of the target id
 * before any revision or qualifiers.
 */

export type HardwareKind = "board" | "shield";

export const BOARD_CATEGORIES = ["controller", "keyboard"] as const;

export type HardwareCategory = (typeof BOARD_CATEGORIES)[number] | "shield";

export const HARDWARE_CATEGORY_LABELS: Record<HardwareCategory, string> = {
  controller: "Controller board",
  keyboard: "Keyboard board",
  shield: "Shield",
};

export const HARDWARE_CATEGORY_SECTION_LABELS: Record<
  HardwareCategory,
  string
> = {
  controller: "Boards (Controller)",
  keyboard: "Boards (Keyboard)",
  shield: "Shields",
};

/**
 * Extract the board name from a Zephyr board target id.
 *
 * Zephyr documents board targets as
 * `board name[@revision][/board qualifiers]`, for example
 * `nrf5340dk/nrf5340/cpuapp`. The catalog groups all qualifiers and variants
 * under the board name `nrf5340dk`.
 */
export function hardwareBoardName(id: string): string {
  return id.split("@")[0].split("/")[0];
}

/**
 * Convert a hardware metadata id into a URL-safe catalog slug.
 *
 * Boards use the Zephyr board name, so `nice_nano//zmk` and
 * `nrf5340dk/nrf5340/cpuapp` become `nice_nano` and `nrf5340dk`.
 * Shields do not use Zephyr's board target qualifiers, so their metadata id is
 * used directly.
 */
export function hardwareSlug(type: HardwareKind, id: string): string {
  const name = type === "board" ? hardwareBoardName(id) : id;

  return name.replace(/[@/]+/g, "-");
}

/**
 * Build a link to a file in the ZMK repository on GitHub.
 */
export function hardwareSourceUrl(path: string): string {
  return `https://github.com/zmkfirmware/zmk/blob/main/${path}`;
}

/**
 * The subset of a hardware catalog entry needed to find related entries.
 */
export interface HardwareCatalogEntry {
  data: {
    type: HardwareKind;
    slug: string;
    name: string;
    sourcePath: string;
  };
}

function hardwareSourceDirectory(sourcePath: string): string {
  const separator = sourcePath.lastIndexOf("/");

  return separator === -1 ? "" : sourcePath.slice(0, separator);
}

/**
 * Find catalog entries that share a metadata directory with `entry`.
 *
 * Related hardware is not the same thing as split hardware: `siblings` are the
 * parts of one logical device, while entries such as `kyria`, `kyria_rev2`,
 * and `kyria_rev3` are separate revisions that live in one directory. Linking
 * them as related hardware lets readers move between revisions without merging
 * pages whose build targets differ.
 */
export function hardwareRelatedEntries<T extends HardwareCatalogEntry>(
  entry: T,
  entries: T[]
): T[] {
  const directory = hardwareSourceDirectory(entry.data.sourcePath);

  return entries
    .filter(
      (candidate) =>
        candidate.data.type === entry.data.type &&
        candidate.data.slug !== entry.data.slug &&
        hardwareSourceDirectory(candidate.data.sourcePath) === directory
    )
    .sort((a, b) => a.data.name.localeCompare(b.data.name));
}
