#ifndef ZEPHYR_DRIVERS_SENSOR_PIXART_PMW3389_H_
#define ZEPHYR_DRIVERS_SENSOR_PIXART_PMW3389_H_

#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/spi.h>

#define PMW3389_WR_MASK 0x80

#define PMW3389_PID 0x47
#define PMW3389_REV 0x01

#define PMW3389_REG_PID 0x00
#define PMW3389_REG_REV 0x01

#define PMW3389_REG_DX_L 0x2A
#define PMW3389_REG_DX_H 0x2B
#define PMW3389_REG_DY_L 0x2C
#define PMW3389_REG_DY_H 0x2D

struct pmw3389_gpio_dt_spec {
    const struct device *port;
    gpio_pin_t pin;
    gpio_dt_flags_t dt_flags;
};

struct pmw3389_spi_cfg {
    struct spi_config spi_conf;
    const char *cs_gpios_label;
};

union pmw3389_bus_cfg {
    struct pmw3389_spi_cfg *spi_cfg;
};

struct pmw3389_config {
    char *bus_name;
    int (*bus_init)(const struct device *dev);
    const union pmw3389_bus_cfg bus_cfg;
    int resolution;
    struct pmw3389_gpio_dt_spec reset_spec;
#if CONFIG_PMW3389_TRIGGER
    struct pmw3389_gpio_dt_spec motswk_spec;
#endif // CONFIG_PMW3389_TRIGGER
};

struct pmw3389_data;

struct pmw3389_transfer_function {
    int (*read_data)(const struct device *dev, uint16_t *value);
};

struct pmw3389_data {
    const struct device *bus;
    struct spi_cs_control cs_ctrl;

    uint16_t dx;
    uint16_t dy;

    const struct pmw3389_transfer_function *hw_tf;

#ifdef CONFIG_PMW3389_TRIGGER

    struct gpio_callback motswk_gpio_cb;
    const struct device *dev;

    sensor_trigger_handler_t handler;
    const struct sensor_trigger *trigger;

#if defined(CONFIG_PMW3389_TRIGGER_OWN_THREAD)
    K_THREAD_STACK_MEMBER(thread_stack, CONFIG_PMW3389_THREAD_STACK_SIZE);
    struct k_sem gpio_sem;
    struct k_thread thread;
#elif defined(CONFIG_PMW3389_TRIGGER_GLOBAL_THREAD)
    struct k_work work;
#endif

#endif /* CONFIG_PMW3389_TRIGGER */
};

int pmw3389_spi_init(const struct device *dev);
#ifdef CONFIG_PMW3389_TRIGGER

int pmw3389_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
                        sensor_trigger_handler_t handler);

int pmw3389_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_PIXART_PMW3389_H_ */