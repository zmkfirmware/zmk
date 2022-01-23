#pragma once

#include <device.h>

#define PINNACLE_READ       0xA0
#define PINNACLE_WRITE      0x80

#define PINNACLE_AUTOINC    0xFC
#define PINNACLE_DUMMY      0xFB

// Registers
#define PINNACLE_FW_ID              0x00 // ASIC ID.
#define PINNACLE_FW_VER             0x01 // Firmware Version Firmware revision number.
#define PINNACLE_STATUS1            0x02 // Contains status flags about the state of Pinnacle.
#define PINNACLE_SYS_CFG            0x03 // Contains system operation and configuration bits.
#define PINNACLE_SYS_CFG_EN_SLEEP   BIT(2)
#define PINNACLE_SYS_CFG_SHUTDOWN   BIT(1)
#define PINNACLE_SYS_CFG_RESET      BIT(0)

#define PINNACLE_FEED_CFG1          0x04 // Contains feed operation and configuration bits.
#define PINNACLE_FEED_CFG1_EN_FEED  BIT(0)
#define PINNACLE_FEED_CFG1_ABS_MODE BIT(1)
#define PINNACLE_FEED_CFG1_DIS_FILT BIT(2)
#define PINNACLE_FEED_CFG1_DIS_X    BIT(3)
#define PINNACLE_FEED_CFG1_DIS_Y    BIT(4)
#define PINNACLE_FEED_CFG1_INV_X    BIT(6)
#define PINNACLE_FEED_CFG1_INV_Y    BIT(7)
#define PINNACLE_FEED_CFG2          0x05 // Contains feed operation and configuration bits.
#define PINNACLE_FEED_CFG2_EN_IM    BIT(0)  // Intellimouse
#define PINNACLE_FEED_CFG2_DIS_TAP  BIT(1)  // Disable all taps
#define PINNACLE_FEED_CFG2_DIS_SEC  BIT(2)  // Disable secondary tap
#define PINNACLE_FEED_CFG2_DIS_SCRL BIT(3)  // Disable scroll
#define PINNACLE_FEED_CFG2_DIS_GE   BIT(4)  // Disable GlideExtend
#define PINNACLE_FEED_CFG2_SWAP_XY  BIT(7)  // Swap X & Y
#define PINNACLE_CAL_CFG            0x07 // Contains calibration configuration bits.
#define PINNACLE_PS2_AUX            0x08 // Contains Data register for PS/2 Aux Control.
#define PINNACLE_SAMPLE             0x09 // Sample Rate Number of samples generated per second.
#define PINNACLE_Z_IDLE             0x0A // Number of Z=0 packets sent when Z goes from >0 to 0.
#define PINNACLE_Z_SCALER           0x0B // Contains the pen Z_On threshold.
#define PINNACLE_SLEEP_INTERVAL     0x0C // Sleep Interval
#define PINNACLE_SLEEP_TIMER        0x0D // Sleep Timer
#define PINNACLE_AG_PACKET0         0x10 // trackpad Data (Pinnacle AG)
#define PINNACLE_2_2_PACKET0        0x12 // trackpad Data
#define PINNACLE_REG_COUNT          0x18

#define PINNACLE_PACKET0_BTN_PRIM   BIT(0)  // Primary button
#define PINNACLE_PACKET0_BTN_SEC    BIT(1)  // Secondary button
#define PINNACLE_PACKET0_BTN_AUX    BIT(2)  // Auxiliary (middle?) button
#define PINNACLE_PACKET0_X_SIGN     BIT(4)  // X delta sign
#define PINNACLE_PACKET0_Y_SIGN     BIT(5)  // Y delta sign

struct pinnacle_data {
    const struct device *spi;
    int16_t dx, dy;
    int8_t wheel;
    uint8_t btn;
#ifdef CONFIG_PINNACLE_TRIGGER
    const struct device *dev;
    const struct sensor_trigger *data_ready_trigger;
	struct gpio_callback gpio_cb;
    sensor_trigger_handler_t data_ready_handler;
#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
    K_THREAD_STACK_MEMBER(thread_stack, CONFIG_PINNACLE_THREAD_STACK_SIZE);
    struct k_sem gpio_sem;
    struct k_thread thread;
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
    struct k_work work;
#endif
#endif
};

struct pinnacle_config {
	struct spi_cs_control spi_cs;
	struct spi_config spi_config;
    bool invert_x, invert_y, sleep_en, no_taps;
#ifdef CONFIG_PINNACLE_TRIGGER
    const struct device *dr_port;
    uint8_t dr_pin, dr_flags;
#endif
};
