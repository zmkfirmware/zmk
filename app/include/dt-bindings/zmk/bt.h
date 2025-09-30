/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define BT_CLR_CMD 0
#define BT_NXT_CMD 1
#define BT_PRV_CMD 2
#define BT_SEL_CMD 3
#define BT_CLR_ALL_CMD 4
#define BT_DISC_CMD 5
#define BT_ADV_OFF_CMD 6
#define BT_ADV_ON_CMD 7
#define BT_ADV_TOG_CMD 8

/*
Note: Some future commands will include additional parameters, so we
defines these aliases up front.
*/

#define BT_CLR BT_CLR_CMD 0
#define BT_NXT BT_NXT_CMD 0
#define BT_PRV BT_PRV_CMD 0
#define BT_SEL BT_SEL_CMD
#define BT_CLR_ALL BT_CLR_ALL_CMD 0
#define BT_DISC BT_DISC_CMD
#define BT_ADV_OFF BT_ADV_OFF_CMD 0
#define BT_ADV_ON BT_ADV_ON_CMD 0
#define BT_ADV_TOG BT_ADV_TOG_CMD 0