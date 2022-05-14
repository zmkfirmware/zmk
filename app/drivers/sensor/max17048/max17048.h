/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <device.h>
#include <sys/util.h>

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
    const char *i2c_device_name;
    uint16_t device_addr;
};

struct max17048_drv_data {
    const struct device *i2c;

    uint16_t raw_state_of_charge;
    uint16_t raw_charge_rate;
    uint16_t raw_vcell;
};

#ifdef __cplusplus
}
#endif
