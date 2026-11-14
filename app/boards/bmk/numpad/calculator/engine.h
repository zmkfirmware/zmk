/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#define CALC_EXPRESSION_SIZE 64
#define CALC_RESULT_SIZE 32

enum calc_error { CALC_OK, CALC_INCOMPLETE, CALC_DIV_ZERO, CALC_OVERFLOW, CALC_FULL };

struct calc_engine {
    char expression[CALC_EXPRESSION_SIZE];
    char result[CALC_RESULT_SIZE];
    double value;
    double seed;
    size_t seed_length;
    bool evaluated;
    enum calc_error error;
};

void calc_clear(struct calc_engine *calc);
void calc_input(struct calc_engine *calc, char key);
void calc_backspace(struct calc_engine *calc);
bool calc_evaluate(struct calc_engine *calc);
const char *calc_error_text(enum calc_error error);
