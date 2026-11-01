export interface FactItem {
  /** Value text shown in the row. */
  label: string;
  /** Renders the label in monospace (IDs, revision numbers). */
  code?: boolean;
  /** Native tooltip; falls back to a browse or default-revision hint. */
  title?: string;
  /** Marks the default revision: adds the star (inline values turn semibold, chips get a stronger border). */
  default?: boolean;
  /** Makes the item a link; chips gain hover and focus styling. */
  href?: string;
}

export interface Fact {
  /** Row label displayed above the value. */
  label: string;
  /** Single plain value; takes precedence over `items`. */
  value?: string;
  /** Renders `value` as monospace code. */
  code?: boolean;
  /** Values to list; rendered as chips unless `inline` is set. */
  items?: FactItem[];
  /** Makes the row span the full grid width. */
  wide?: boolean;
  /** Renders `items` as a compact slash-separated line instead of chips. */
  inline?: boolean;
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
