import { useEffect, useMemo, useState } from "react";

import type { HardwareCategory } from "@/lib/hardware-catalog";
import {
  ALL_INTERCONNECTS,
  HARDWARE_CATALOG_CATEGORY_ORDER,
  filterHardwareCatalog,
  type HardwareCatalogItem,
  type HardwareInterconnectOption,
} from "@/lib/hardware-catalog-search";

import styles from "./HardwareCatalog.module.css";

interface Props {
  /** Entries rendered as cards in the results grid; also the total in the "Showing X of Y" count. */
  items: HardwareCatalogItem[];
  /** Options for the interconnect filter dropdown; also validates the URL's interconnect state. */
  interconnects: HardwareInterconnectOption[];
}

const FILTER_LABELS: Record<HardwareCategory, string> = {
  controller: "Board (Controller)",
  keyboard: "Board (Keyboard)",
  shield: "Shields",
};

const TYPE_LABELS: Record<HardwareCategory, string> = {
  controller: "Controller Board",
  keyboard: "Keyboard Board",
  shield: "Shield",
};

// ?q=nice&type=controller&type=keyboard&interconnect=pro_micro
const QUERY_PARAM = "q";
const TYPE_PARAM = "type";
const INTERCONNECT_PARAM = "interconnect";

function parseCategories(values: string[]): HardwareCategory[] {
  const requested = new Set(
    values.flatMap((value) => value.split(",")).map((value) => value.trim())
  );

  return HARDWARE_CATALOG_CATEGORY_ORDER.filter((category) =>
    requested.has(category)
  );
}

function readFiltersFromUrl(
  search: string,
  interconnects: HardwareInterconnectOption[]
) {
  const params = new URLSearchParams(search);
  const requestedInterconnect = params.get(INTERCONNECT_PARAM)?.trim();
  const validInterconnects = new Set(interconnects.map((option) => option.id));

  return {
    query: params.get(QUERY_PARAM) ?? "",
    categories: parseCategories(params.getAll(TYPE_PARAM)),
    interconnect:
      requestedInterconnect && validInterconnects.has(requestedInterconnect)
        ? requestedInterconnect
        : ALL_INTERCONNECTS,
  };
}

function itemSummary(
  item: HardwareCatalogItem,
  names: Record<string, string>
): string | undefined {
  const parts: string[] = [];

  if (item.manufacturer) {
    parts.push(item.manufacturer);
  }

  if (item.requires.length > 0) {
    parts.push(
      `Requires ${item.requires.map((id) => names[id] ?? id).join(", ")}`
    );
  }

  if (item.exposes.length > 0) {
    parts.push(
      `Exposes ${item.exposes.map((id) => names[id] ?? id).join(", ")}`
    );
  }

  return parts.length > 0 ? parts.join(" · ") : undefined;
}

export default function HardwareCatalog({ items, interconnects }: Props) {
  const [query, setQuery] = useState("");
  const [selectedCategories, setSelectedCategories] = useState<
    HardwareCategory[]
  >([]);
  const [interconnect, setInterconnect] = useState(ALL_INTERCONNECTS);
  const [hydrated, setHydrated] = useState(false);

  const interconnectNames = useMemo(
    () =>
      Object.fromEntries(
        interconnects.map(({ id, name }) => [id, name])
      ) as Record<string, string>,
    [interconnects]
  );

  const filteredItems = useMemo(
    () =>
      filterHardwareCatalog(
        items,
        query,
        selectedCategories,
        interconnect,
        interconnectNames
      ),
    [items, query, selectedCategories, interconnect, interconnectNames]
  );

  const hasActiveFilters =
    query.trim() !== "" ||
    interconnect !== ALL_INTERCONNECTS ||
    selectedCategories.length > 0;

  function toggleCategory(category: HardwareCategory) {
    setSelectedCategories((current) =>
      current.includes(category)
        ? current.filter((selected) => selected !== category)
        : [...current, category]
    );
  }

  function resetFilters() {
    setQuery("");
    setSelectedCategories([]);
    setInterconnect(ALL_INTERCONNECTS);
  }

  useEffect(() => {
    function applyFiltersFromUrl() {
      const filters = readFiltersFromUrl(window.location.search, interconnects);

      setQuery(filters.query);
      setSelectedCategories(filters.categories);
      setInterconnect(filters.interconnect);
    }

    applyFiltersFromUrl();
    setHydrated(true);
    window.addEventListener("popstate", applyFiltersFromUrl);

    return () => window.removeEventListener("popstate", applyFiltersFromUrl);
  }, [interconnects]);

  useEffect(() => {
    if (!hydrated) return;

    const params = new URLSearchParams(window.location.search);
    const trimmedQuery = query.trim();

    if (trimmedQuery) {
      params.set(QUERY_PARAM, trimmedQuery);
    } else {
      params.delete(QUERY_PARAM);
    }

    params.delete(TYPE_PARAM);

    for (const category of HARDWARE_CATALOG_CATEGORY_ORDER) {
      if (selectedCategories.includes(category)) {
        params.append(TYPE_PARAM, category);
      }
    }

    if (interconnect !== ALL_INTERCONNECTS) {
      params.set(INTERCONNECT_PARAM, interconnect);
    } else {
      params.delete(INTERCONNECT_PARAM);
    }

    const nextSearch = params.toString();
    const nextUrl = `${window.location.pathname}${nextSearch ? `?${nextSearch}` : ""
      }${window.location.hash}`;
    const currentUrl = `${window.location.pathname}${window.location.search}${window.location.hash}`;

    if (nextUrl !== currentUrl) {
      window.history.replaceState(window.history.state, "", nextUrl);
    }
  }, [hydrated, query, selectedCategories, interconnect]);

  return (
    <div className={`not-content ${styles.catalog}`}>
      <div className={styles.controls}>
        <div className={styles.searchField}>
          <svg
            className={styles.searchIcon}
            viewBox="0 0 24 24"
            aria-hidden="true"
          >
            <circle
              cx="11"
              cy="11"
              r="7"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
            />
            <path
              d="m16.5 16.5 4 4"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
            />
          </svg>
          <input
            id="hardware-catalog-search"
            className={styles.search}
            type="search"
            aria-label="Search hardware"
            placeholder="Search by name..."
            value={query}
            onChange={(event) => setQuery(event.target.value)}
            autoComplete="off"
            spellCheck={false}
          />
        </div>

        <div className={styles.filterRow}>
          <div className={styles.filterGroup}>
            <span id="hardware-catalog-type" className={styles.filterLabel}>
              Type
            </span>
            <div
              className={styles.toggles}
              role="group"
              aria-labelledby="hardware-catalog-type"
            >
              {HARDWARE_CATALOG_CATEGORY_ORDER.map((category) => (
                <button
                  key={category}
                  type="button"
                  className={styles.toggle}
                  aria-pressed={selectedCategories.includes(category)}
                  onClick={() => toggleCategory(category)}
                >
                  {FILTER_LABELS[category]}
                </button>
              ))}
            </div>
          </div>

          <div className={styles.filterGroup}>
            <label
              className={styles.filterLabel}
              htmlFor="hardware-catalog-interconnect"
            >
              Interconnect
            </label>
            <div className={styles.selectField}>
              <select
                id="hardware-catalog-interconnect"
                className={styles.select}
                value={interconnect}
                onChange={(event) => setInterconnect(event.target.value)}
              >
                <option value={ALL_INTERCONNECTS}>All interconnects</option>
                {interconnects.map((option) => (
                  <option key={option.id} value={option.id}>
                    {option.name}
                  </option>
                ))}
              </select>
              <svg
                className={styles.selectIcon}
                viewBox="0 0 20 20"
                aria-hidden="true"
              >
                <path
                  d="m5.5 8 4.5 4.5L14.5 8"
                  fill="none"
                  stroke="currentColor"
                  strokeWidth="1.75"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                />
              </svg>
            </div>
          </div>
        </div>
      </div>

      <div className={styles.resultBar}>
        <p className={styles.resultCount} role="status" aria-live="polite">
          {filteredItems.length === 0 ? (
            "No matching hardware."
          ) : (
            <>
              Showing <strong>{filteredItems.length}</strong> of {items.length}{" "}
              entries
            </>
          )}
        </p>
        {hasActiveFilters && (
          <button type="button" className={styles.reset} onClick={resetFilters}>
            Clear filters
          </button>
        )}
      </div>

      {filteredItems.length === 0 ? (
        <div className={styles.empty}>
          <p>Try a different search term or adjust the filters above.</p>
        </div>
      ) : (
        <ul className={styles.list}>
          {filteredItems.map((item) => {
            const summary = itemSummary(item, interconnectNames);

            return (
              <li key={`${item.type}/${item.slug}`}>
                <a className={styles.card} href={item.href}>
                  <span className={styles.cardTop}>
                    <span
                      className={`${styles.typeBadge} ${styles[item.category]}`}
                    >
                      {TYPE_LABELS[item.category]}
                    </span>
                    {item.type === "board" &&
                      item.outputs &&
                      item.outputs.length > 0 && (
                        <span className={styles.outputs}>
                          {item.outputs.map((output) => (
                            <span key={output} className={styles.output}>
                              {output.toUpperCase()}
                            </span>
                          ))}
                        </span>
                      )}
                  </span>

                  <span className={styles.cardMain}>
                    <span className={styles.name}>{item.name}</span>
                    <code className={styles.id}>{item.id}</code>
                  </span>

                  {summary && <span className={styles.meta}>{summary}</span>}
                </a>
              </li>
            );
          })}
        </ul>
      )}
    </div>
  );
}
