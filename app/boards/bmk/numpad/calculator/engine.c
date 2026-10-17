/* SPDX-License-Identifier: MIT */
#include "engine.h"

/* Zephyr's LVGL adapter aliases lv_snprintf/lv_vsnprintf to libc names.
 * The built-in formatter would override libc; CLIB would recurse via vsnprintf.
 * CUSTOM leaves these aliases to the floating-point-capable C library.
 */
#if defined(__ZEPHYR__) && !defined(CONFIG_LV_USE_CUSTOM_SPRINTF)
#error "The numpad calculator requires CONFIG_LV_USE_CUSTOM_SPRINTF=y"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool digit(char c) { return c >= '0' && c <= '9'; }
static bool operator(char c) { return c == '+' || c == '-' || c == '*' || c == '/'; }

void calc_clear(struct calc_engine *c) {
    memset(c, 0, sizeof(*c));
    strcpy(c->result, "0");
}

const char *calc_error_text(enum calc_error error) {
    switch (error) {
    case CALC_INCOMPLETE:
        return "Incomplete";
    case CALC_DIV_ZERO:
        return "Divide by zero";
    case CALC_OVERFLOW:
        return "Overflow";
    case CALC_FULL:
        return "Expression full";
    default:
        return "";
    }
}

void calc_input(struct calc_engine *c, char key) {
    if (!digit(key) && key != '.' && !operator(key)) {
        return;
    }
    if (c->evaluated) {
        if (operator(key)) {
            strcpy(c->expression, c->result);
            c->seed = c->value;
            c->seed_length = strlen(c->expression);
            c->evaluated = false;
        } else {
            calc_clear(c);
        }
    }
    c->error = CALC_OK;
    size_t len = strlen(c->expression);
    if (key == '.') {
        size_t start = len;
        while (start && (digit(c->expression[start - 1]) || c->expression[start - 1] == '.')) {
            start--;
        }
        if (strchr(c->expression + start, '.') != NULL) {
            return;
        }
    }
    if (operator(key)) {
        if (!len) {
            if (key != '-') {
                return;
            }
        } else if (operator(c->expression[len - 1])) {
            bool unary =
                c->expression[len - 1] == '-' && (len == 1 || operator(c->expression[len - 2]));
            if (unary) {
                if (key == '-') {
                    return;
                }
                if (len == 1) {
                    return;
                }
                len -= 2;
            } else if (key != '-') {
                len--;
            }
        }
    }
    if (len >= sizeof(c->expression) - 1) {
        c->error = CALC_FULL;
        return;
    }
    if (c->seed_length && len <= c->seed_length && !operator(key)) {
        c->seed_length = 0;
    }
    c->expression[len] = key;
    c->expression[len + 1] = '\0';
}

void calc_backspace(struct calc_engine *c) {
    c->evaluated = false;
    c->error = CALC_OK;
    size_t len = strlen(c->expression);
    if (len) {
        c->expression[--len] = '\0';
    }
    if (len < c->seed_length) {
        c->seed_length = 0;
    }
    if (!len) {
        calc_clear(c);
    }
}

struct parser {
    const char *p;
    struct calc_engine *calc;
    enum calc_error error;
};

static double number(struct parser *p) {
    if (p->p == p->calc->expression && p->calc->seed_length) {
        p->p += p->calc->seed_length;
        return p->calc->seed;
    }
    const char *start = p->p;
    if (*start == '-') {
        start++;
    }
    if (!digit(*start) && !(*start == '.' && digit(start[1]))) {
        p->error = CALC_INCOMPLETE;
        return 0;
    }
    char *end;
    double value = strtod(p->p, &end);
    p->p = end;
    if (!isfinite(value)) {
        p->error = CALC_OVERFLOW;
    }
    return value;
}

static double term(struct parser *p) {
    double value = number(p);
    while (!p->error && (*p->p == '*' || *p->p == '/')) {
        char op = *p->p++;
        double rhs = number(p);
        if (p->error) {
            break;
        }
        if (op == '/' && rhs == 0) {
            p->error = CALC_DIV_ZERO;
            break;
        }
        value = op == '*' ? value * rhs : value / rhs;
        if (!isfinite(value)) {
            p->error = CALC_OVERFLOW;
        }
    }
    return value;
}

bool calc_evaluate(struct calc_engine *c) {
    if (c->evaluated) {
        return c->error == CALC_OK;
    }
    if (!c->expression[0]) {
        strcpy(c->expression, "0");
    }
    struct parser p = {.p = c->expression, .calc = c};
    double value = term(&p);
    while (!p.error && (*p.p == '+' || *p.p == '-')) {
        char op = *p.p++;
        double rhs = term(&p);
        value = op == '+' ? value + rhs : value - rhs;
        if (!p.error && !isfinite(value)) {
            p.error = CALC_OVERFLOW;
        }
    }
    c->error = p.error ? p.error : (*p.p ? CALC_INCOMPLETE : CALC_OK);
    if (c->error) {
        return false;
    }
    c->value = value == 0 ? 0 : value;
    snprintf(c->result, sizeof(c->result), "%.12g", c->value);
    /* No shifted '+' key is needed when sending positive exponents. */
    char *exponent = strchr(c->result, 'e');
    if (exponent && exponent[1] == '+') {
        memmove(exponent + 1, exponent + 2, strlen(exponent + 2) + 1);
    }
    c->evaluated = true;
    return true;
}
