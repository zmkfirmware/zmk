/* SPDX-License-Identifier: MIT */
#include "driver_fakes.h"
#include "module/drivers/led/is31/is31fl3737.c"
#include "module/drivers/led/is31/power.c"

static struct device bus_dev = {.ready = true}, gpio_dev = {.ready = true};
static struct is31fl3737_config config = {.bus = {.bus = &bus_dev, .addr = 0x50},
                                          .sdb = {.port = &gpio_dev, .pin = 6},
                                          .current_limit = 123};
static struct is31fl3737_data data;
static struct device led_dev = {.config = &config, .data = &data, .ready = true};
static struct zmk_is31_power_config power;
static uint8_t registers[4][256], page;
static int sdb, gpio_calls, bus_calls, fail_bus_at, fail_gpio_value;
static bool fail_bus_all;

static int gpio_pin_set_dt(const struct gpio_dt_spec *pin, int value) {
    assert(pin->port == &gpio_dev);
    gpio_calls++;
    if (value == fail_gpio_value) {
        return -EIO;
    }
    sdb = value;
    return 0;
}
static int gpio_pin_configure_dt(const struct gpio_dt_spec *pin, int flags) {
    assert(flags == GPIO_OUTPUT_ACTIVE);
    return gpio_pin_set_dt(pin, 1);
}
static int transaction(void) {
    assert(data.lock.depth == 1); /* All IS31FL3737 register access must use its driver lock. */
    bus_calls++;
    return fail_bus_all || bus_calls == fail_bus_at ? -EIO : 0;
}
static int i2c_reg_write_byte_dt(const struct i2c_dt_spec *bus, uint8_t reg, uint8_t value) {
    assert(bus->addr == 0x50);
    if (transaction()) {
        return -EIO;
    }
    if (reg == 0xFD) {
        assert(value < 4);
        page = value;
    } else if (reg != 0xFE) {
        registers[page][reg] = value;
    }
    return 0;
}
static int i2c_write_dt(const struct i2c_dt_spec *bus, const uint8_t *buf, size_t len) {
    (void)bus;
    if (transaction()) {
        return -EIO;
    }
    assert((size_t)buf[0] + len - 1 <= 256);
    memcpy(&registers[page][buf[0]], &buf[1], len - 1);
    return 0;
}
static void reset(void) {
    memset(registers, 0xA5, sizeof(registers));
    memset(&data, 0, sizeof(data));
    data.selected_page = UINT8_MAX;
    bus_dev.ready = gpio_dev.ready = led_dev.ready = true;
    config.sdb.port = &gpio_dev;
    fail_bus_at = 0;
    fail_bus_all = false;
    fail_gpio_value = -1;
    sdb = 1;
    gpio_calls = bus_calls = 0;
    page = 2;
    power = (struct zmk_is31_power_config){.chip = ZMK_IS31_CHIP_IS31FL3737,
                                           .dev = &led_dev,
                                           .bus = config.bus,
                                           .sdb = config.sdb,
                                           .current_limit = config.current_limit};
    assert(is31fl3737_init(&led_dev) == 0);
    bus_calls = gpio_calls = 0;
}
#include "composite_fakes.h"

int main(void) {
    reset();
    fail_bus_all = true;
    assert(zmk_is31_enter_hard_off(&power) == 0 && sdb == 0 && gpio_calls == 1);
    reset();
    bus_dev.ready = false;
    assert(zmk_is31_enter_hard_off(&power) == 0 && sdb == 0 && bus_calls == 0);
    reset();
    bus_dev.ready = false;
    assert(zmk_is31_resume_from_hard_off(&power) == -ENODEV && sdb == 0);
    reset();
    led_dev.ready = false;
    assert(zmk_is31_enter_hard_off(&power) == 0 && sdb == 0);
    reset();
    fail_gpio_value = 0;
    assert(zmk_is31_enter_hard_off(&power) == -EIO && sdb == 1);
    reset();
    power.sdb.port = NULL;
    fail_bus_all = true;
    assert(zmk_is31_enter_hard_off(&power) == -EIO && gpio_calls == 0);
    reset();
    gpio_dev.ready = false;
    assert(zmk_is31_enter_hard_off(&power) == -ENODEV);

    reset();
    assert(zmk_is31_resume_from_hard_off(&power) == 0);
    int resume_operations = bus_calls;
    for (int fail = 1; fail <= resume_operations; fail++) {
        reset();
        assert(zmk_is31_enter_hard_off(&power) == 0);
        bus_calls = 0;
        fail_bus_at = fail;
        assert(zmk_is31_resume_from_hard_off(&power) == -EIO);
        assert(sdb == 0); /* Every individual initialization failure must shut hardware down. */
        fail_bus_at = 0;
        assert(zmk_is31_resume_from_hard_off(&power) == 0 && sdb == 1);
        assert(registers[3][0] == 1 && registers[3][1] == config.current_limit);
    }
    reset();
    fail_gpio_value = 1;
    assert(zmk_is31_resume_from_hard_off(&power) == -EIO && sdb == 0);

    reset();
    for (int cycle = 0; cycle < 100; cycle++) {
        uint8_t rgb[] = {10, 20, 30};
        assert(is31fl3737_led_write_channels(&led_dev, 11, 3, rgb) == 0);
        assert(registers[1][11] == 10 && registers[1][16] == 20 && registers[1][17] == 30);
        assert(zmk_is31_enter_hard_off(&power) == 0 && sdb == 0);
        assert(zmk_is31_resume_from_hard_off(&power) == 0 && sdb == 1);
        assert(data.selected_page == page);
        assert(registers[1][11] == 0 && registers[1][16] == 0);
        assert(is31fl3737_led_write_channels(&led_dev, 11, 3, rgb) == 0);
        assert(registers[1][11] == 10 && registers[3][0] == 1);
    }
    reset();
    fail_bus_all = true;
    assert(is31fl3737_init(&led_dev) == -EIO && sdb == 0);
    printf("PASS driver: shutdown fallbacks, %d initialization failure points, 100 power "
           "cycles/page cache\n",
           resume_operations);
    test_composite();
    return 0;
}
