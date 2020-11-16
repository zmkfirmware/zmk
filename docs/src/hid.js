/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import codes from "./data/hid";

export const map = codes.reduce((map, item) => {
  item.names.forEach((name) => (map[name] = item));
  return map;
}, {});

export function getCode(id) {
  return map[id] ?? null;
}

export function getCodes(ids) {
  return ids.reduce((result, id) => [...result, map[id]], []);
}
