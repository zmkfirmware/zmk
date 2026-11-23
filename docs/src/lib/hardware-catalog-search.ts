import {
  HARDWARE_CATEGORIES,
  type HardwareCategory,
  type HardwareKind,
} from "./hardware-catalog";

export const ALL_INTERCONNECTS = "all";

export interface HardwareCatalogTarget {
  id: string;
  name: string;
}

export interface HardwareCatalogItem {
  href: string;
  slug: string;
  id: string;
  name: string;
  type: HardwareKind;
  category: HardwareCategory;
  manufacturer?: string;
  description?: string;
  features: string[];
  requires: string[];
  exposes: string[];
  siblings: string[];
  variants: string[];
  arch?: string;
  outputs?: string[];
  targets: HardwareCatalogTarget[];
}

export interface HardwareInterconnectOption {
  id: string;
  name: string;
}

interface SearchField {
  text: string;
  weight: number;
}

function normalize(value: string): string {
  return value
    .normalize("NFKD")
    .replace(/\p{M}/gu, "")
    .toLowerCase()
    .replace(/[^\p{L}\p{N}]+/gu, " ")
    .trim();
}

/**
 * Find the edit distance between two strings when it is at most `max`.
 * Returns `undefined` once the distance is known to exceed the limit.
 */
function boundedEditDistance(
  a: string,
  b: string,
  max: number
): number | undefined {
  if (Math.abs(a.length - b.length) > max) return undefined;

  let previousPrevious: number[] | undefined;
  let previous = Array.from({ length: b.length + 1 }, (_, index) => index);

  for (let i = 1; i <= a.length; i++) {
    const current = [i];
    let rowMinimum = i;

    for (let j = 1; j <= b.length; j++) {
      const substitution = a[i - 1] === b[j - 1] ? 0 : 1;

      current[j] = Math.min(
        previous[j] + 1,
        current[j - 1] + 1,
        previous[j - 1] + substitution
      );

      if (
        previousPrevious &&
        j > 1 &&
        a[i - 1] === b[j - 2] &&
        a[i - 2] === b[j - 1]
      ) {
        current[j] = Math.min(current[j], previousPrevious[j - 2] + 1);
      }

      if (current[j] < rowMinimum) rowMinimum = current[j];
    }

    if (rowMinimum > max) return undefined;

    previousPrevious = previous;
    previous = current;
  }

  return previous[b.length] <= max ? previous[b.length] : undefined;
}

function allowedTypos(token: string): number {
  if (token.length >= 7) return 2;
  if (token.length >= 4) return 1;
  return 0;
}

/**
 * Score a single query token against a searchable string.
 *
 * Direct substrings score highest, with a bonus for matches at a word
 * boundary. Otherwise words are checked for prefixes, substrings, and close
 * spellings, so a typo like "ncie" still finds "nice".
 */
function scoreText(text: string, token: string): number {
  if (!text) return 0;

  const index = text.indexOf(token);

  if (index !== -1) {
    const atBoundary = index === 0 || text[index - 1] === " ";

    return 100 + (atBoundary ? 25 : 0) - Math.min(index, 40);
  }

  const maxTypos = allowedTypos(token);
  let best = 0;

  for (const word of text.split(" ")) {
    if (!word) continue;

    let score = 0;

    if (word === token) {
      score = 90;
    } else if (word.startsWith(token)) {
      score = 75;
    } else if (word.includes(token)) {
      score = 55 - Math.min(word.length - token.length, 20);
    } else if (maxTypos > 0) {
      const distance = boundedEditDistance(word, token, maxTypos);

      if (distance !== undefined) {
        score = 40 - distance * 10;
      }
    }

    if (score > best) best = score;
  }

  return best;
}

function buildSearchFields(
  item: HardwareCatalogItem,
  interconnectNames: Record<string, string>
): SearchField[] {
  const interconnectText = [...item.requires, ...item.exposes]
    .flatMap((id) => [id, interconnectNames[id] ?? ""])
    .join(" ");
  const targetText = item.targets
    .flatMap((target) => [target.id, target.name])
    .join(" ");

  return [
    { text: normalize(item.name), weight: 10 },
    { text: normalize(item.id), weight: 8 },
    { text: normalize(item.manufacturer ?? ""), weight: 6 },
    { text: normalize(interconnectText), weight: 5 },
    {
      text: normalize(
        [
          ...item.features,
          ...(item.outputs ?? []),
          item.arch ?? "",
          item.type,
          item.category,
        ].join(" ")
      ),
      weight: 3,
    },
    { text: normalize(item.description ?? ""), weight: 2 },
    {
      text: normalize(
        [...item.siblings, ...item.variants, targetText].join(" ")
      ),
      weight: 2,
    },
  ];
}

function scoreItem(
  item: HardwareCatalogItem,
  tokens: string[],
  interconnectNames: Record<string, string>
): number | null {
  if (tokens.length === 0) return 0;

  const fields = buildSearchFields(item, interconnectNames);
  let score = 0;

  for (const token of tokens) {
    let best = 0;

    for (const field of fields) {
      const fieldScore = scoreText(field.text, token) * field.weight;

      if (fieldScore > best) best = fieldScore;
    }

    if (best === 0) return null;

    score += best;
  }

  const query = tokens.join(" ");
  const name = normalize(item.name);
  const id = normalize(item.id);

  if (name === query) score += 500;
  else if (name.startsWith(query)) score += 200;
  else if (name.includes(query)) score += 100;

  if (id === query) score += 300;
  else if (id.includes(query)) score += 60;

  return score;
}

export function filterHardwareCatalog(
  items: HardwareCatalogItem[],
  query: string,
  selectedCategories: readonly HardwareCategory[],
  interconnect: string,
  interconnectNames: Record<string, string>
): HardwareCatalogItem[] {
  const tokens = normalize(query).split(" ").filter(Boolean);
  let categoryFilter: Set<HardwareCategory> | undefined;

  if (
    selectedCategories.length > 0 &&
    selectedCategories.length < HARDWARE_CATEGORIES.length
  ) {
    categoryFilter = new Set(selectedCategories);
  }

  const matches: { item: HardwareCatalogItem; score: number }[] = [];

  for (const item of items) {
    if (categoryFilter && !categoryFilter.has(item.category)) continue;

    if (
      interconnect !== ALL_INTERCONNECTS &&
      !item.requires.includes(interconnect) &&
      !item.exposes.includes(interconnect)
    ) {
      continue;
    }

    const score = scoreItem(item, tokens, interconnectNames);

    if (score === null) continue;

    matches.push({ item, score });
  }

  matches.sort(
    (a, b) => b.score - a.score || a.item.name.localeCompare(b.item.name)
  );

  return matches.map((match) => match.item);
}
