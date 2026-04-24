/*                                      52 KEY MATRIX / LAYOUT MAPPING

  ╭────────────────────────╮               ╭────────────────────────╮
  │  0   1   2   3   4   5 │               │  6   7   8   9  10  11 │
  │ 12  13  14  15  16  17 ╰──╮         ╭──╯ 18  19  20  21  22  23 │
  │ 24  25  26  27  28  29  30╰────┬────╯31  32  33  34  35  36  37 │
  │ 38  39  40      41  42  43     │     44  45  46      47  48  49 │
  ╰────────────────────────────────┴────────────────────────────────╯
 ╭────────────────────────────╮         ╭────────────────────────────╮
 │ LT5 LT4 LT3 LT2 LT1 LT0 LH5│         │RH5 RT0 RT1 RT2 RT3 RT4 RT5 │
 │ LM5 LM4 LM3 LM2 LM1 LM0 LH4│         │RH4 RM0 RM1 RM2 RM3 RM4 RM5 │
 │ LB5 LB4 LB3 LB2 LB1 LB0 LH3╰────┬────╯RH3 RB0 RB1 RB2 RB3 RB4 RB5 │
 │ LP2 LP1 LP0╭───╮LH2 LH1 LH0     │     RH0 RH1 RH2╭───╮RP0 RP1 RP2 │
 ╰────────────╯   ╰────────────────┴────────────────╯   ╰────────────╯
 T : Top
 M : Middle
 B : Bottom
 H : Hand
 P : Palm
*/

#pragma once

#define LT0  5  // left-top row
#define LT1  4
#define LT2  3
#define LT3  2
#define LT4  1
#define LT5  0

#define RT0  6  // right-top row
#define RT1  7
#define RT2  8
#define RT3  9
#define RT4 10
#define RT5 11

#define LM0 17  // left-middle row
#define LM1 16
#define LM2 15
#define LM3 14
#define LM4 13
#define LM5 12

#define RM0 18  // right-middle row
#define RM1 19
#define RM2 20
#define RM3 21
#define RM4 22
#define RM5 23

#define LB0 29  // left-bottom row
#define LB1 28
#define LB2 27
#define LB3 26
#define LB4 25
#define LB5 24

#define RB0 32  // right-bottom row
#define RB1 33
#define RB2 34
#define RB3 35
#define RB4 36
#define RB5 37

#define LH0 43  // left thumb keys
#define LH1 42
#define LH2 41
#define LH3 30

#define RH0 44  // right thumb keys
#define RH1 45
#define RH2 46
#define RH3 31

#define LP0 40  // left-palm keys
#define LP1 41
#define LP2 42

#define RP0 49  // right-paml keys
#define RP1 50
#define RP2 51
