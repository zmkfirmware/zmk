/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef ZEPHYR_DRIVERS_SENSOR_PIXART_PMW33XX_H_
#define ZEPHYR_DRIVERS_SENSOR_PIXART_PMW33XX_H_

#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/spi.h>

#define PMW33XX_WR_MASK 0x80
#define PMW33XX_RD_MASK 0x7F

#define PMW33XX_3389_PID 0x47
#define PMW33XX_3360_PID 0x42
#define PMW33XX_REV 0x01

/* General Registers */
#define PMW33XX_REG_PID 0x00
#define PMW33XX_REG_REV_ID 0x01
#define PMW33XX_REG_PWR_UP_RST 0x3A

/* Motion Registers */
#define PMW33XX_REG_MOTION 0x02
#define PMW33XX_REG_DX_L 0x03
#define PMW33XX_REG_DX_H 0x04
#define PMW33XX_REG_DY_L 0x05
#define PMW33XX_REG_DY_H 0x06
#define PMW33XX_REG_BURST 0x50

/* Motion bits */
#define PMW33XX_MOTION (1 << 8)
#define PMW33XX_OPMODE_RUN (0)
#define PMW33XX_OPMODE_REST1 (0b01 << 1)
#define PMW33XX_OPMODE_REST2 (0b10 << 1)
#define PMW33XX_OPMODE_REST3 (0b11 << 1)

/* SROM Registers */
#define PMW33XX_REG_SROM_EN 0x13
#define PMW33XX_REG_SROM_ID 0x2A
#define PMW33XX_REG_SROM_BURST 0x62

/* SROM CMDs */
#define PMW33XX_SROM_CRC_CMD 0x15
#define PMW33XX_SROM_DWNLD_CMD 0x1D
#define PMW33XX_SROM_DWNLD_START_CMD 0x18

/* CPI Registers */
#define PMW33XX_3360_REG_CPI 0x0F
#define PMW33XX_3389_REG_CPI_L 0x0E
#define PMW33XX_3389_REG_CPI_H 0x0F

/* Config Registers */
#define PMW33XX_REG_CONFIG2 0x10
#define PMW33XX_REG_OBSERVATION 0x24
#define PMW33XX_REG_DOUT_L 0x25
#define PMW33XX_REG_DOUT_H 0x26

/* Config2 Bits */
#define PMW33XX_RESTEN 0x20
#define PMW33XX_RPT_MOD 0x04

/* Observation Bits */
#define PMW33XX_SROM_RUN 0x40

/* power up reset cmd */
#define PMW33XX_RESET_CMD 0x5A

/* cpi max and min values */
#define PMW33XX_3389_CPI_MIN 50
#define PMW33XX_3389_CPI_MAX 16000
#define PMW33XX_3360_CPI_MIN 100
#define PMW33XX_3360_CPI_MAX 12000

struct pmw33xx_gpio_dt_spec {
    const struct device *port;
    gpio_pin_t pin;
    gpio_dt_flags_t dt_flags;
};

struct pmw33xx_spi_cfg {
    struct spi_config spi_conf;
    struct pmw33xx_gpio_dt_spec cs_spec;
};

union pmw33xx_bus_cfg {
    struct pmw33xx_spi_cfg *spi_cfg;
};

struct pmw33xx_config {
    char *bus_name;
    int (*bus_init)(const struct device *dev);
    const union pmw33xx_bus_cfg bus_cfg;
    bool disable_rest;
    int cpi;
#if CONFIG_PMW33XX_TRIGGER
    struct pmw33xx_gpio_dt_spec motswk_spec;
#endif // CONFIG_PMW33XX_TRIGGER
};

struct pmw33xx_data;

struct pmw33xx_transfer_function {
    int (*read_data)(const struct device *dev, int16_t *value);
};

struct pmw33xx_data {
    const struct device *bus;
    struct spi_cs_control cs_ctrl;

    int16_t dx;
    int16_t dy;

    const struct pmw33xx_transfer_function *hw_tf;

#ifdef CONFIG_PMW33XX_TRIGGER

    struct gpio_callback motswk_gpio_cb;
    const struct device *dev;

    sensor_trigger_handler_t handler;
    const struct sensor_trigger *trigger;

#if defined(CONFIG_PMW33XX_TRIGGER_OWN_THREAD)
    K_THREAD_STACK_MEMBER(thread_stack, CONFIG_PMW33XX_THREAD_STACK_SIZE);
    struct k_sem gpio_sem;
    struct k_thread thread;
#elif defined(CONFIG_PMW33XX_TRIGGER_GLOBAL_THREAD)
    struct k_work work;
#endif

#endif /* CONFIG_PMW33XX_TRIGGER */
};

int pmw33xx_spi_init(const struct device *dev);
#ifdef CONFIG_PMW33XX_TRIGGER

int pmw33xx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                        sensor_trigger_handler_t handler);

int pmw33xx_init_interrupt(const struct device *dev);

void pmw33xx_reset_motion(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_PIXART_PMW33XX_H_ */
