/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

int zmk_pm_suspend_devices(void);
void zmk_pm_resume_devices(void);

int zmk_pm_soft_off(void);