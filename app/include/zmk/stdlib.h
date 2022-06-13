/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdlib.h> /* for size_t */

/*
 * ANSI C version of strlcpy
 * Based on the NetBSD strlcpy man page.
 *
 * Nathan Myers <ncm-nospam@cantrip.org>, 2003/06/03
 * Placed in the public domain.
 */

size_t strlcpy(char *dst, const char *src, size_t size);