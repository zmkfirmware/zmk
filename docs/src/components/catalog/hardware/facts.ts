export interface FactItem {
  /** Value text shown in the row. */
  label: string;
  /** Renders the label in monospace (IDs, revision numbers). */
  code?: boolean;
  /** Native tooltip; falls back to a browse or default-revision hint. */
  title?: string;
  /** Marks the default revision: adds the star (plain values turn semibold, chips get a stronger border). */
  default?: boolean;
  /** Makes the item a link; chips gain hover and focus styling. */
  href?: string;
}

export interface Fact {
  /** Row label displayed above the value. */
  label: string;
  /** Values to list; a single value is still expressed as an item. */
  items: FactItem[];
  /** Makes the row span the full grid width. */
  wide?: boolean;
  /** Renders `items` as bordered chips instead of a compact slash-separated line. */
  chip?: boolean;
}

export function factItemTitle(item: FactItem): string | undefined {
  if (item.href) {
    return `Browse hardware using ${item.label}`;
  }

  if (item.title) {
    return item.title;
  }

  return item.default ? "Default revision" : undefined;
}
