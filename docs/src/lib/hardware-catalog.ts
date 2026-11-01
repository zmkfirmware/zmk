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
