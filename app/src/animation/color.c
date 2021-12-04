/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>

#include <zmk/animation.h>

static float fmod(float a, float b) {
	float mod = a < 0 ? -a : a;
	float x = b < 0 ? -b : b;

	while (mod >= x) {
		mod = mod - x;
	}

	return a < 0 ? -mod : mod;
}

static float fabs(float a) {
	return a < 0 ? -a : a;
}

/**
 * HSL chosen over HSV/HSB as it shares the same parameters with LCh or HSLuv.
 * The latter color spaces could be interesting to experiment with because of their
 * perceptual uniformity, but it would come at the cost of some performance.
 * Using the same parameters would make it easy to toggle any such behavior using a single config flag.
 *
 * Algorithm source: https://www.tlbx.app/color-converter
 */
void zmk_hsl_to_rgb(const struct zmk_color_hsl *hsl, struct zmk_color_rgb *rgb) {
	float s = (float) hsl->s / 100;
	float l = (float) hsl->l / 100;

	float a = (float) hsl->h / 60;
	float chroma = s * (1 - fabs(2 * l - 1));
	float x = chroma * (1 - fabs(fmod(a, 2) - 1));
	float m = l - chroma / 2;

	switch ((uint8_t) a % 6) {
	case 0:
		rgb->r = m + chroma;
		rgb->g = m + x;
		rgb->b = m;
		break;
	case 1:
		rgb->r = m + x;
		rgb->g = m + chroma;
		rgb->b = m;
		break;
	case 2:
		rgb->r = m;
		rgb->g = m + chroma;
		rgb->b = m + x;
		break;
	case 3:
		rgb->r = m;
		rgb->g = m + x;
		rgb->b = m + chroma;
		break;
	case 4:
		rgb->r = m + x;
		rgb->g = m;
		rgb->b = m + chroma;
		break;
	case 5:
		rgb->r = m + chroma;
		rgb->g = m;
		rgb->b = m + x;
		break;
	}
}

/**
 * Converts ZMKs RGB (float) to Zephyr's led_rgb (uint8_t) format.
 */
void zmk_rgb_to_led_rgb(const struct zmk_color_rgb *rgb, struct led_rgb *led) {
	led->r = (uint8_t) (rgb->r * 255);
	led->g = (uint8_t) (rgb->g * 255);
	led->b = (uint8_t) (rgb->b * 255);
}

/**
 * Compares two HSL colors.
 */
bool zmk_cmp_hsl(const struct zmk_color_hsl *a, const struct zmk_color_hsl *b) {
	return a->h == b->h && a->s == b->s && a->l == b->l;
}

/**
 * Interpolate between two colors using the cylindrical model (HSL).
 */
void zmk_interpolate_hsl(const struct zmk_color_hsl *from, const struct zmk_color_hsl *to,
							struct zmk_color_hsl *result, float step) {
    int16_t hue_delta;

    hue_delta = from->h - to->h;
    hue_delta = hue_delta + (180 < abs(hue_delta) ? (hue_delta < 0 ? 360 : -360) : 0);

    result->h = (uint16_t) (360 + from->h - (hue_delta * step)) % 360;
    result->s = from->s - (from->s - to->s) * step;
    result->l = from->l - (from->l - to->l) * step;
}
