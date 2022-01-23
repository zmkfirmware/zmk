#define DT_DRV_COMPAT cirque_pinnacle

#include <drivers/spi.h>
#include <init.h>
#include <drivers/sensor.h>
#include <zmk/sensors.h>
#include <logging/log.h>

#include "cirque_trackpad.h"

LOG_MODULE_REGISTER(pinnacle, CONFIG_SENSOR_LOG_LEVEL);

static int pinnacle_seq_read(const struct device *dev, const uint8_t start, uint8_t *buf, const uint8_t len) {
    uint8_t tx_buffer[len + 3], rx_dummy[3];
    tx_buffer[0] = PINNACLE_READ | start;
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
    const struct pinnacle_data *data = dev->data;
    const struct pinnacle_config *config = dev->config;
    return spi_transceive(data->spi, &config->spi_config, &tx, &rx);
}

static int pinnacle_write(const struct device *dev, const uint8_t addr, const uint8_t val) {
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
    const struct pinnacle_data *data = dev->data;
    const struct pinnacle_config *config = dev->config;
    const int ret = spi_transceive(data->spi, &config->spi_config, &tx, &rx);
    if (rx_buffer[1] != 0xFB) {
        LOG_ERR("bad ret val");
        return -EIO;
    }
    if (ret < 0) {
        LOG_ERR("spi ret: %d", ret);
    }
    return ret;
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

static int pinnacle_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr, const struct sensor_value *val) {
    const struct pinnacle_config *config = dev->config;
    if (attr == SENSOR_ATTR_PINNACLE_GE) {
        const uint8_t ge_set = val->val1 ? 0 : PINNACLE_FEED_CFG2_DIS_GE;
        const uint8_t taps_set = config->no_taps ? PINNACLE_FEED_CFG2_DIS_TAP : 0;
        pinnacle_write(dev, PINNACLE_FEED_CFG2, ge_set | taps_set);
        return 0;
    }
    return -ENOTSUP;
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
    int ret = gpio_pin_interrupt_configure(config->dr_port, config->dr_pin, en ? GPIO_INT_LEVEL_ACTIVE : GPIO_INT_DISABLE);
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
    data->spi = DEVICE_DT_GET(SPI_BUS);

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
    gpio_pin_configure(config->dr_port, config->dr_pin, GPIO_INPUT | config->dr_flags);
    gpio_init_callback(&data->gpio_cb, pinnacle_gpio_cb, BIT(config->dr_pin));
    int ret = gpio_add_callback(config->dr_port, &data->gpio_cb);
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
    return 0;
}

static const struct sensor_driver_api pinnacle_driver_api = {
#if CONFIG_PINNACLE_TRIGGER
	.trigger_set = pinnacle_trigger_set,
#endif
	.sample_fetch = pinnacle_sample_fetch,
	.channel_get = pinnacle_channel_get,
    .attr_set = pinnacle_attr_set,
};

static struct pinnacle_data pinnacle_data;
static const struct pinnacle_config pinnacle_config = {
    .spi_cs = {
        .gpio_dev = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(SPI_BUS, cs_gpios, SPI_REG)),
        .gpio_pin = DT_GPIO_PIN_BY_IDX(SPI_BUS, cs_gpios, SPI_REG),
        .delay = 0,
        .gpio_dt_flags = DT_GPIO_FLAGS_BY_IDX(SPI_BUS, cs_gpios, SPI_REG),
    },
    .spi_config = {
        .cs = &pinnacle_config.spi_cs,
        .frequency = DT_INST_PROP(0, spi_max_frequency),
        .slave = DT_INST_REG_ADDR(0),
        .operation = (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB),
    },
    .invert_x = DT_INST_PROP(0, invert_x),
    .invert_y = DT_INST_PROP(0, invert_y),
    .sleep_en = DT_INST_PROP(0, sleep),
    .no_taps = DT_INST_PROP(0, no_taps),
#ifdef CONFIG_PINNACLE_TRIGGER
    .dr_port = DEVICE_DT_GET(DT_GPIO_CTLR(DT_DRV_INST(0), dr_gpios)),
    .dr_pin = DT_INST_GPIO_PIN(0, dr_gpios),
    .dr_flags = DT_INST_GPIO_FLAGS(0, dr_gpios),
#endif
};

DEVICE_DT_INST_DEFINE(0, pinnacle_init, device_pm_control_nop, &pinnacle_data, &pinnacle_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &pinnacle_driver_api);
