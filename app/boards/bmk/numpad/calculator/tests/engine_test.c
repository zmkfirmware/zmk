/* SPDX-License-Identifier: MIT */
#include "engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static struct calc_engine calc;
static void type(const char *text) {
    while (*text) {
        calc_input(&calc, *text++);
    }
}
static void expect(const char *text, const char *result) {
    calc_clear(&calc);
    type(text);
    assert(calc_evaluate(&calc));
    if (strcmp(calc.result, result)) {
        fprintf(stderr, "%s: got %s expected %s\n", text, calc.result, result);
    }
    assert(strcmp(calc.result, result) == 0);
}
int main(void) {
    expect("2+3*4", "14");
    expect("8/2*3", "12");
    expect("0.1+0.2", "0.3");
    expect("-2*-3+4/-2", "4");
    expect("2--3", "5");
    expect(".5+.25", "0.75");
    expect("-0", "0");
    expect("2+*3", "6");
    expect("2*-+3", "5");
    expect("1..2", "1.2");
    expect("999999999999*999999999999", "9.99999999998e23");
    assert(calc_evaluate(&calc));
    calc_input(&calc, '7');
    assert(!strcmp(calc.expression, "7"));
    expect("1/3", "0.333333333333");
    type("*3-1");
    assert(calc_evaluate(&calc));
    assert(calc.value == 0); /* Continue from full precision, not displayed digits. */
    expect("12", "12");
    type("+");
    calc_backspace(&calc);
    type("3");
    assert(calc_evaluate(&calc) && !strcmp(calc.result, "123"));
    expect("1/10000000000000", "1e-13");
    type("*10000000000000");
    assert(calc_evaluate(&calc) && !strcmp(calc.result, "1"));
    expect("12+3", "15");
    calc_backspace(&calc);
    assert(!strcmp(calc.expression, "12+"));
    assert(!calc_evaluate(&calc) && calc.error == CALC_INCOMPLETE);
    type("4");
    assert(calc_evaluate(&calc) && !strcmp(calc.result, "16"));
    calc_clear(&calc);
    type("1/0");
    assert(!calc_evaluate(&calc) && calc.error == CALC_DIV_ZERO);
    calc_backspace(&calc);
    type("2");
    assert(calc_evaluate(&calc) && !strcmp(calc.result, "0.5"));
    calc_clear(&calc);
    for (int i = 0; i < 64; i++) {
        calc_input(&calc, '9');
    }
    assert(strlen(calc.expression) == 63 && calc.error == CALC_FULL);
    calc_backspace(&calc);
    assert(strlen(calc.expression) == 62 && calc.error == CALC_OK);
    calc_clear(&calc);
    type("-");
    assert(!calc_evaluate(&calc) && calc.error == CALC_INCOMPLETE);
    calc_clear(&calc);
    assert(calc_evaluate(&calc) && !strcmp(calc.result, "0"));
    expect("999999999999999999999999999999999999999999999999999999999999999", "1e63");
    for (int i = 0; i < 5; i++) {
        type("*99999999999999999999999999999999999999999999999999999999999");
        if (!calc_evaluate(&calc)) {
            break;
        }
    }
    assert(calc.error == CALC_OVERFLOW);
    calc_clear(&calc);
    assert(!strcmp(calc.result, "0") && !calc.expression[0] && !calc.error);
    puts("calculator engine tests passed");
}
