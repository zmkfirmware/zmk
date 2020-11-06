/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import groups from "./data/groups.js";

export function getGroup(id) {
  return groups[id] ?? null;
}
