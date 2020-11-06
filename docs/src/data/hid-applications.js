/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: CC-BY-NC-SA-4.0
 */

import usage from "../hid-usage";
import * as pages from "./hid-usage-pages";

export const keyboard = usage(pages.genericDesktop, 0x06);
export const consumer = usage(pages.consumer, 0x01);
