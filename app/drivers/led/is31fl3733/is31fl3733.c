/*
 * Copyright (c) 2020 Cameron Banna.
 */

#define DT_DRV_COMPAT issi_is31fl3733

/*
 *  LED driver for the issi is31fl3733 IC
 * 	led_on sets the leds state to on
 * 	led_off sets the leds state to off
 * 	set_brightness sets thepwm value for the specific led
 * 	set_color will set leds based upon a led number and 3
 * 	values representing RGB this is applicable for uses
 * 	where the leds are wired as shown in the datasheet
 *	in "Figure 2 Typical Application Circuit (RGB)".
 *
 *	struct device *dev = device_get_binding("IS31FL3733A");
 *	#define red 255
 *	#define green 255
 *	#define blue 255
 *	#define led 1
 *  uint8_t RGB[3] = (red, green, blue);
 *	set_color(dev, led, 3, RGB);
 *
 *	This would set the led in position (SW1,CS1) to the value
 *	of red, the led in position (SW2,CS1) to the value of
 *	green, and the led in position (SW3,CS1) to the value of
 *	blue. Before this will show any result the 3 corresponding
 *	leds needto be turned on with the led_on call.
 */

#include <drivers/i2c.h>
#include <drivers/led.h>
#include <sys/util.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(is31fl3733);
/** Number of CS lines. */
#define IS31FL3733_CS (16)
/** Number of SW lines. */
#define IS31FL3733_SW (12)
/** IS31FL3733 common registers. */
#define IS31FL3733_PSR (0xFD)          ///< Page select register. Write only.
#define IS31FL3733_PSWL (0xFE)         ///< Page select register write lock. Read/Write.
                                       /** Registers in Page 0. */
#define IS31FL3733_LEDONOFF (0x0000)   /// ON or OFF state control for each LED. Write only.
#define IS31FL3733_LEDOPEN (0x0018)    /// Open state for each LED. Read only.
#define IS31FL3733_LEDSHORT (0x0030)   /// Short state for each LED. Read only.
                                       /** Registers in Page 1. */
#define IS31FL3733_LEDPWM (0x0100)     /// PWM duty for each LED. Write only.
                                       /** Registers in Page 3. */
#define IS31FL3733_CR (0x0300)         /// Configuration Register. Write only.
#define IS31FL3733_RESET (0x0311)      /// Reset register. Read only.
                                       /** PSWL register bits. */
#define IS31FL3733_PSWL_DISABLE (0x00) /// Disable write to Page Select register.
#define IS31FL3733_PSWL_ENABLE (0xC5)  /// Enable write to Page select register.

struct is31fl3733_config {
    int reg;
    int inst;
    char *bus_name;
};
struct is31fl3733_data {
    const struct device *i2c;
};
uint8_t leds[IS31FL3733_SW * IS31FL3733_CS / 8];

static int is31fl3733_set_page(const struct device *dev, uint16_t addr) {
    const struct is31fl3733_data *dev_data = dev->data;
    const struct is31fl3733_config *dev_cfg = dev->config;
    if (i2c_reg_write_byte(dev_data->i2c, dev_cfg->reg, IS31FL3733_PSWL, IS31FL3733_PSWL_ENABLE)) {
        LOG_ERR("Enable write to Page select register failed");
        return -EIO;
    }
    if (i2c_reg_write_byte(dev_data->i2c, dev_cfg->reg, IS31FL3733_PSR, addr)) {
        LOG_ERR("Writing to Page select register failed");
        return -EIO;
    }
    return 0;
}
static int is31fl3733_write_page_reg(const struct device *dev, uint8_t reg, uint8_t buffer) {
    const struct is31fl3733_data *dev_data = dev->data;
    const struct is31fl3733_config *dev_cfg = dev->config;
    if (i2c_reg_write_byte(dev_data->i2c, dev_cfg->reg, reg, buffer)) {
        LOG_ERR("Writing Page Failed");
        return -EIO;
    }
    return 0;
}
static int is31fl3733_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value) {
    uint8_t cs = led - ((led / 16) * 16);
    uint8_t sw = led / 16;
    uint8_t offset = sw * IS31FL3733_CS + cs;
    // Set page to 0x01 PWM Page
    is31fl3733_set_page(dev, 0x01);
    is31fl3733_write_page_reg(dev, offset, value);
    return 0;
}
static inline int is31fl3733_led_on(const struct device *dev, uint32_t led) {
    uint8_t cs = led - ((led / 16) * 16);
    uint8_t sw = led / 16;
    uint8_t offset = (sw << 1) + (cs / 8);
    leds[offset] |= 0x01 << (cs % 8);
    // Set page to 0x01 Led Control Page
    is31fl3733_set_page(dev, 0x00);
    is31fl3733_write_page_reg(dev, offset, leds[offset]);
    return 0;
}
static inline int is31fl3733_led_off(const struct device *dev, uint32_t led) {
    uint8_t cs = led - ((led / 16) * 16);
    uint8_t sw = led / 16;
    uint8_t offset = (sw << 1) + (cs / 8);
    leds[offset] &= ~(0x01 << (cs % 8));
    // Set page to 0x01 Led Control Page
    is31fl3733_set_page(dev, 0x00);
    is31fl3733_write_page_reg(dev, offset, leds[offset]);
    return 0;
}
static int is31fl3733_led_set_color(const struct device *dev, uint32_t led, uint8_t num_of_colors,
                                    const uint8_t *colors) {
    uint8_t cs = led - ((led / 16) * 16);
    uint8_t sw = (led / 16) * 3;
    uint8_t offset_red = sw * IS31FL3733_CS + cs;
    uint8_t offset_green = (sw + 1) * IS31FL3733_CS + cs;
    uint8_t offset_blue = (sw + 2) * IS31FL3733_CS + cs;
    // Set page to 0x01 PWM Page
    is31fl3733_set_page(dev, 0x01);
    is31fl3733_write_page_reg(dev, offset_red, colors[0]);
    is31fl3733_write_page_reg(dev, offset_green, colors[1]);
    is31fl3733_write_page_reg(dev, offset_blue, colors[2]);
    return 0;
}
static int is31fl3733_led_reset(const struct device *dev) {
    const struct is31fl3733_data *data = dev->data;
    const struct is31fl3733_config *dev_cfg = dev->config;
    if (i2c_reg_write_byte(data->i2c, dev_cfg->reg, IS31FL3733_PSWL, IS31FL3733_PSWL_ENABLE)) {
        LOG_ERR("Enable write to Page select register failed");
        return -EIO;
    }
    uint8_t partA = (uint8_t)((IS31FL3733_RESET & 0xFF00) >> 8);
    uint8_t partB = (uint8_t)(IS31FL3733_RESET & 0x00FF);
    uint8_t tx_buf[2] = {IS31FL3733_PSR, partA};
    // Sequence to reset the IC
    if (i2c_write(data->i2c, tx_buf, 2, dev_cfg->reg)) {
        LOG_ERR("Reseting Device Failed");
        return -EIO;
    }
    uint8_t tx_buf2[1] = {partB};
    if (i2c_write(data->i2c, tx_buf2, 1, dev_cfg->reg)) {
        LOG_ERR("Reseting Device Failed");
        return -EIO;
    }
    if (i2c_read(data->i2c, NULL, 1, dev_cfg->reg)) {
        LOG_ERR("Reseting Device Failed");
        return -EIO;
    }
    // Set Page to 0x03 Function page
    is31fl3733_set_page(dev, 0x03);
    //	use the write page reg function to set the config
    //	register to 0x00 and set the value to 0x01 for normal
    //	operation, this can also be set to 0x01 for software
    //	shutdown mode. see page 17 & 18 of datasheet
    is31fl3733_write_page_reg(dev, 0x00, 0x01);
    return 0;
}

static int is31fl3733_led_init(const struct device *dev) {
    const struct is31fl3733_config *dev_cfg = dev->config;
    struct is31fl3733_data *dev_data = dev->data;
    dev_data->i2c = device_get_binding(dev_cfg->bus_name);
    if (dev_data->i2c == NULL) {
        LOG_DBG("Failed to get I2C device");
        return -EINVAL;
    }
    is31fl3733_led_reset(dev);
    //	Set the Global Current Control, would like to pull this
    //	value from the device tree eventually
    is31fl3733_write_page_reg(dev, 0x03, 0x01);
    uint8_t GCC = 255;
    is31fl3733_write_page_reg(dev, 0x01, GCC);
    return 0;
}

static const struct led_driver_api is31fl3733_led_api = {
    .on = is31fl3733_led_on,
    .off = is31fl3733_led_off,
    .set_brightness = is31fl3733_led_set_brightness,
    .set_color = is31fl3733_led_set_color,
};

#define IS31FL3733_INIT(inst)                                                                      \
    static struct is31fl3733_data is31fl3733_led_data_##inst;                                      \
    static const struct is31fl3733_config is31fl3733_config_##inst = {                             \
        .bus_name = DT_INST_BUS_LABEL(inst),                                                       \
        .reg = DT_INST_REG_ADDR(inst),                                                             \
    };                                                                                             \
    DEVICE_AND_API_INIT(is31fl3733_led##inst, DT_INST_LABEL(inst), &is31fl3733_led_init,           \
                        &is31fl3733_led_data_##inst, &is31fl3733_config_##inst, POST_KERNEL,       \
                        CONFIG_LED_INIT_PRIORITY, &is31fl3733_led_api);
DT_INST_FOREACH_STATUS_OKAY(IS31FL3733_INIT)
