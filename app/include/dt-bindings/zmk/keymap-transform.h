
#include <zmk/util.h>

#define KT_ROW(item) (item >> 8)
#define KT_COL(item) (item & 0xFF)

#define KT_ROW_COL(row, col) (((row) << 8) + (col))

#define _IDENTITY_ENTRY(col, row) KT_ROW_COL(row, col)
#define KT_ID_ROW(row, count) UTIL_LISTIFY(count, _IDENTITY_ENTRY, row)

#define _OFFSET_ENTRY(col, offset, row) KT_ROW_COL(row, col + offset)
#define KT_OFFSET_ROW(offset, row, count) UTIL_LISTIFY(count, _OFFSET_ENTRY, offset, row)