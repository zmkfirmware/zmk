#define DT_DRV_COMPAT cirque_pinnacle

#include <init.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "cirque_trackpad.h"

LOG_MODULE_REGISTER(pinnacle, CONFIG_SENSOR_LOG_LEVEL);

static int pinnacle_seq_read(const struct device *dev, const uint8_t addr, uint8_t *buf, const uint8_t len) {
    const struct pinnacle_config *config = dev->config;
#if DT_INST_ON_BUS(0, spi)
    uint8_t tx_buffer[len + 3], rx_dummy[3];
    tx_buffer[0] = PINNACLE_READ | addr;
    memset(&tx_buffer[1], PINNACLE_AUTOINC, len + 1);
    tx_buffer[len + 2] = PINNACLE_DUMMY;

    const struct spi_buf tx_buf = {
        .buf = tx_buffer,
        .len = len + 3,
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };
	struct spi_buf rx_buf[2] = {
		{
			.buf = rx_dummy,
			.len = 3,
		},
		{
			.buf = buf,
			.len = len,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2,
	};
    return spi_transceive_dt(&config->bus, &tx, &rx);
#elif DT_INST_ON_BUS(0, i2c)
    return i2c_burst_read_dt(&config->bus, PINNACLE_READ | addr, buf, len);
#endif
}

static int pinnacle_write(const struct device *dev, const uint8_t addr, const uint8_t val) {
    const struct pinnacle_config *config = dev->config;
#if DT_INST_ON_BUS(0, spi)
    uint8_t tx_buffer[2] = { PINNACLE_WRITE | addr, val };
    uint8_t rx_buffer[2];

    const struct spi_buf tx_buf = {
        .buf = tx_buffer,
        .len = 2,
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };
	const struct spi_buf rx_buf[1] = {
		{
			.buf = rx_buffer,
			.len = sizeof(rx_buffer),
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 1,
	};
    const int ret = spi_transceive_dt(&config->bus, &tx, &rx);
    if (rx_buffer[1] != 0xFB) {
        LOG_ERR("bad ret val");
        return -EIO;
    }
    if (ret < 0) {
        LOG_ERR("spi ret: %d", ret);
    }
    return ret;
#elif DT_INST_ON_BUS(0, i2c)
    return i2c_reg_write_byte_dt(&config->bus, PINNACLE_WRITE | addr, val);
#endif
}

static int pinnacle_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val) {
    const struct pinnacle_data *data = dev->data;
    switch (chan) {
    case SENSOR_CHAN_POS_DX: val->val1 = data->dx; break;
    case SENSOR_CHAN_POS_DY: val->val1 = data->dy; break;
    case SENSOR_CHAN_PRESS: val->val1 = data->btn; break;
    default: return -ENOTSUP;
    }
    return 0;
}

static int pinnacle_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    uint8_t packet[3];
    int res = pinnacle_seq_read(dev, PINNACLE_2_2_PACKET0, packet, 3);
    if (res < 0) {
        LOG_ERR("res: %d", res);
        return res;
    }
    struct pinnacle_data *data = dev->data;
    data->btn = packet[0] & PINNACLE_PACKET0_BTN_PRIM;
    data->dx = (int16_t) (int8_t) packet[1];
    data->dy = (int16_t) (int8_t) packet[2];
    return 0;
}

#ifdef CONFIG_PINNACLE_TRIGGER
static void set_int(const struct device *dev, const bool en) {
    const struct pinnacle_config *config = dev->config;
    int ret = gpio_pin_interrupt_configure_dt(&config->dr, en ? GPIO_INT_LEVEL_ACTIVE : GPIO_INT_DISABLE);
    if (ret < 0) {
        LOG_ERR("can't set interrupt");
    }
}
    
static int pinnacle_trigger_set(const struct device *dev, const struct sensor_trigger *trig, sensor_trigger_handler_t handler) {
    struct pinnacle_data *data = dev->data;

    set_int(dev, false);
    if (trig->type != SENSOR_TRIG_DATA_READY) {
        return -ENOTSUP;
    }
    data->data_ready_trigger = trig;
    data->data_ready_handler = handler;
    set_int(dev, true);
    return 0;
}

static void pinnacle_int_cb(const struct device *dev) {
    struct pinnacle_data *data = dev->data;
    data->data_ready_handler(dev, data->data_ready_trigger);
    set_int(dev, true);
}

#ifdef CONFIG_PINNACLE_TRIGGER_OWN_THREAD
static void pinnacle_thread(void *arg) {
    const struct device *dev = arg;
    struct pinnacle_data *data = dev->data;

    while (1) {
        k_sem_take(&data->gpio_sem, K_FOREVER);
        pinnacle_int_cb(dev);
        pinnacle_write(dev, PINNACLE_STATUS1, 0);   // Clear SW_DR
    }
}
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
static void pinnacle_work_cb(struct k_work *work) {
    struct pinnacle_data *data = CONTAINER_OF(work, struct pinnacle_data, work);
    pinnacle_int_cb(data->dev);
    pinnacle_write(dev, PINNACLE_STATUS1, 0);   // Clear SW_DR
}
#endif

static void pinnacle_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    struct pinnacle_data *data = CONTAINER_OF(cb, struct pinnacle_data, gpio_cb);
    const struct device *dev = data->dev;
#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
    k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
    k_work_submit(&data->work);
#endif
}
#endif

#define SPI_BUS DT_BUS(DT_DRV_INST(0))
#define SPI_REG DT_REG_ADDR(DT_DRV_INST(0))

static int pinnacle_init(const struct device *dev) {
    struct pinnacle_data *data = dev->data;
    const struct pinnacle_config *config = dev->config;

    pinnacle_write(dev, PINNACLE_STATUS1, 0);   // Clear CC
    pinnacle_write(dev, PINNACLE_Z_IDLE, 0);    // No Z-Idle packets
    if (config->sleep_en) {
        pinnacle_write(dev, PINNACLE_SYS_CFG, PINNACLE_SYS_CFG_EN_SLEEP);
    }
    if (config->no_taps) {
        pinnacle_write(dev, PINNACLE_FEED_CFG2, PINNACLE_FEED_CFG2_DIS_TAP);
    }
    uint8_t feed_cfg1 = PINNACLE_FEED_CFG1_EN_FEED;
    if (config->invert_x) {
        feed_cfg1 |= PINNACLE_FEED_CFG1_INV_X;
    }
    if (config->invert_y) {
        feed_cfg1 |= PINNACLE_FEED_CFG1_INV_Y;
    }
    if (feed_cfg1) {
        pinnacle_write(dev, PINNACLE_FEED_CFG1, feed_cfg1);
    }

#ifdef CONFIG_PINNACLE_TRIGGER
    data->dev = dev;
    gpio_pin_configure_dt(&config->dr, GPIO_INPUT);
    gpio_init_callback(&data->gpio_cb, pinnacle_gpio_cb, BIT(config->dr.pin));
    int ret = gpio_add_callback(config->dr.port, &data->gpio_cb);
    if (ret < 0) {
        LOG_ERR("Failed to set DR callback: %d", ret);
        return -EIO;
    }

#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
    k_sem_init(&data->gpio_sem, 0, UINT_MAX);

    k_thread_create(&data->thread, data->thread_stack, CONFIG_PINNACLE_THREAD_STACK_SIZE,
                    (k_thread_entry_t) pinnacle_thread, (void *) dev, 0, NULL,
                    K_PRIO_COOP(CONFIG_PINNACLE_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
    k_work_init(&data->work, pinnacle_work_cb);
#endif
    pinnacle_write(dev, PINNACLE_FEED_CFG1, feed_cfg1);
#endif
    LOG_WRN("inited");
    return 0;
}

static const struct sensor_driver_api pinnacle_driver_api = {
#if CONFIG_PINNACLE_TRIGGER
	.trigger_set = pinnacle_trigger_set,
#endif
	.sample_fetch = pinnacle_sample_fetch,
	.channel_get = pinnacle_channel_get,
};

#define CIRQUE_INST(n) \
    static struct pinnacle_data pinnacle_data_##n; \
    static const struct pinnacle_config pinnacle_config_##n = { \
        .bus = COND_CODE_1(DT_INST_ON_BUS(0, i2c), (I2C_DT_SPEC_INST_GET(0)), (SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB, 0))), \
        .invert_x = DT_INST_PROP(0, invert_x), \
        .invert_y = DT_INST_PROP(0, invert_y), \
        .sleep_en = DT_INST_PROP(0, sleep), \
        .no_taps = DT_INST_PROP(0, no_taps), \
        COND_CODE_1(CONFIG_PINNACLE_TRIGGER, (.dr = GPIO_DT_SPEC_GET(DT_DRV_INST(0), dr_gpios),), ) \
    }; \
    DEVICE_DT_INST_DEFINE(n, pinnacle_init, device_pm_control_nop, &pinnacle_data_##n, &pinnacle_config_##n, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &pinnacle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CIRQUE_INST)
