
#define KT_ROW(item) (item >> 8)
#define KT_COL(item) (item & 0xFF)

#define RC(row, col) (((row) << 8) + (col))