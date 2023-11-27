#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/i2c.h>

#define PINNACLE_READ 0xA0
#define PINNACLE_WRITE 0x80

#define PINNACLE_AUTOINC 0xFC
#define PINNACLE_FILLER 0xFB

// Registers
#define PINNACLE_FW_ID 0x00   // ASIC ID.
#define PINNACLE_FW_VER 0x01  // Firmware Version Firmware revision number.
#define PINNACLE_STATUS1 0x02 // Contains status flags about the state of Pinnacle.
#define PINNACLE_STATUS1_SW_DR BIT(2)
#define PINNACLE_STATUS1_SW_CC BIT(3)
#define PINNACLE_SYS_CFG 0x03 // Contains system operation and configuration bits.
#define PINNACLE_SYS_CFG_EN_SLEEP BIT(2)
#define PINNACLE_SYS_CFG_SHUTDOWN BIT(1)
#define PINNACLE_SYS_CFG_RESET BIT(0)

#define PINNACLE_FEED_CFG1 0x04 // Contains feed operation and configuration bits.
#define PINNACLE_FEED_CFG1_EN_FEED BIT(0)
#define PINNACLE_FEED_CFG1_ABS_MODE BIT(1)
#define PINNACLE_FEED_CFG1_DIS_FILT BIT(2)
#define PINNACLE_FEED_CFG1_DIS_X BIT(3)
#define PINNACLE_FEED_CFG1_DIS_Y BIT(4)
#define PINNACLE_FEED_CFG1_INV_X BIT(6)
#define PINNACLE_FEED_CFG1_INV_Y BIT(7)
#define PINNACLE_FEED_CFG2 0x05             // Contains feed operation and configuration bits.
#define PINNACLE_FEED_CFG2_EN_IM BIT(0)     // Intellimouse
#define PINNACLE_FEED_CFG2_DIS_TAP BIT(1)   // Disable all taps
#define PINNACLE_FEED_CFG2_DIS_SEC BIT(2)   // Disable secondary tap
#define PINNACLE_FEED_CFG2_DIS_SCRL BIT(3)  // Disable scroll
#define PINNACLE_FEED_CFG2_DIS_GE BIT(4)    // Disable GlideExtend
#define PINNACLE_FEED_CFG2_ROTATE_90 BIT(7) // Swap X & Y
#define PINNACLE_CAL_CFG 0x07               // Contains calibration configuration bits.
#define PINNACLE_PS2_AUX 0x08               // Contains Data register for PS/2 Aux Control.
#define PINNACLE_SAMPLE 0x09                // Sample Rate Number of samples generated per second.
#define PINNACLE_Z_IDLE 0x0A                // Number of Z=0 packets sent when Z goes from >0 to 0.
#define PINNACLE_Z_SCALER 0x0B              // Contains the pen Z_On threshold.
#define PINNACLE_SLEEP_INTERVAL 0x0C        // Sleep Interval
#define PINNACLE_SLEEP_TIMER 0x0D           // Sleep Timer
#define PINNACLE_AG_PACKET0 0x10            // trackpad Data (Pinnacle AG)
#define PINNACLE_2_2_PACKET0 0x12           // trackpad Data
#define PINNACLE_REG_COUNT 0x18

#define PINNACLE_REG_ERA_VALUE 0x1B
#define PINNACLE_REG_ERA_HIGH_BYTE 0x1C
#define PINNACLE_REG_ERA_LOW_BYTE 0x1D
#define PINNACLE_REG_ERA_CONTROL 0x1E

#define PINNACLE_ERA_CONTROL_READ 0x01
#define PINNACLE_ERA_CONTROL_WRITE 0x02

#define PINNACLE_ERA_REG_TRACKING_ADC_CONFIG 0x0187

#define PINNACLE_TRACKING_ADC_CONFIG_1X 0x00
#define PINNACLE_TRACKING_ADC_CONFIG_2X 0x40
#define PINNACLE_TRACKING_ADC_CONFIG_3X 0x80
#define PINNACLE_TRACKING_ADC_CONFIG_4X 0xC0

#define PINNACLE_PACKET0_BTN_PRIM BIT(0) // Primary button
#define PINNACLE_PACKET0_BTN_SEC BIT(1)  // Secondary button
#define PINNACLE_PACKET0_BTN_AUX BIT(2)  // Auxiliary (middle?) button
#define PINNACLE_PACKET0_X_SIGN BIT(4)   // X delta sign
#define PINNACLE_PACKET0_Y_SIGN BIT(5)   // Y delta sign

struct pinnacle_data {
    uint8_t btn_cache;
    bool in_int;
    const struct device *dev;
    struct gpio_callback gpio_cb;
    struct k_work work;
};

enum pinnacle_sensitivity {
    PINNACLE_SENSITIVITY_1X,
    PINNACLE_SENSITIVITY_2X,
    PINNACLE_SENSITIVITY_3X,
    PINNACLE_SENSITIVITY_4X,
};

struct pinnacle_config {
#if DT_INST_ON_BUS(0, i2c)
    const struct i2c_dt_spec bus;
#elif DT_INST_ON_BUS(0, spi)
    const struct spi_dt_spec bus;
#endif
    bool rotate_90, sleep_en, no_taps;
    enum pinnacle_sensitivity sensitivity;
    const struct gpio_dt_spec dr;
};
