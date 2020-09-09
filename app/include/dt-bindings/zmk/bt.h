/*
 * Copyright (c) 2020 Peter Johanson <peter@peterjohanson.com>
 *
 * SPDX-License-Identifier: MIT
 */

#define BT_CLEAR_BONDS_CMD  0
#define BT_PROF_NEXT_CMD    1
#define BT_PROF_PREV_CMD    2
#define BT_PROF_SEL_CMD     3
// #define BT_FULL_RESET_CMD   4

/*
Note: Some future commands will include additional parameters, so we
defines these aliases up front.
*/

#define BT_CLEAR_BONDS  BT_CLEAR_BONDS_CMD  0
#define BT_PROF_NEXT    BT_PROF_NEXT_CMD    0
#define BT_PROF_PREV    BT_PROF_PREV_CMD    0
#define BT_PROF_SEL     BT_PROF_SEL_CMD