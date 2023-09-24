/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REG_VCELL 0x02
#define REG_STATE_OF_CHARGE 0x04
#define REG_MODE 0x06
#define REG_VERSION 0x08
#define REG_HIBERNATE 0x0A
#define REG_CONFIG 0x0C
#define REG_VALERT 0x14
#define REG_CHARGE_RATE 0x16
#define REG_VRESET 0x18
#define REG_STATUS 0x1A

struct max17048_config {
    struct i2c_dt_spec i2c_bus;
};

struct max17048_drv_data {
    struct k_sem lock;

    uint16_t raw_state_of_charge;
    uint16_t raw_charge_rate;
    uint16_t raw_vcell;
};

#ifdef __cplusplus
}
#endif
