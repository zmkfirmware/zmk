#define DT_DRV_COMPAT cirque_pinnacle

#include <init.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "pinnacle.h"

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
    int ret;
    ret = pinnacle_seq_read(dev, PINNACLE_STATUS1, packet, 0);
    if (ret < 0) {
        LOG_ERR("read status: %d", ret);
        return ret;
    }
    if (!(packet[0] & PINNACLE_STATUS1_SW_DR)) {
        return -EAGAIN;
    }
    ret = pinnacle_seq_read(dev, PINNACLE_2_2_PACKET0, packet, 3);
    if (ret < 0) {
        LOG_ERR("read packet: %d", ret);
        return ret;
    }
    struct pinnacle_data *data = dev->data;
    data->btn = packet[0] & PINNACLE_PACKET0_BTN_PRIM;
    data->dx = (int16_t) (int8_t) packet[1];
    data->dy = (int16_t) (int8_t) packet[2];
    if (!data->in_int) {
        ret = pinnacle_write(dev, PINNACLE_STATUS1, 0);   // Clear SW_DR
        if (ret < 0) {
            LOG_ERR("clear dr: %d", ret);
            return ret;
        }
    }
    return 0;
}

#ifdef CONFIG_PINNACLE_TRIGGER
static void set_int(const struct device *dev, const bool en) {
    const struct pinnacle_config *config = dev->config;
    int ret = gpio_pin_interrupt_configure_dt(&config->dr, en ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
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
    int ret = pinnacle_write(dev, PINNACLE_STATUS1, 0);   // Clear SW_DR
    if (ret < 0) {
        LOG_ERR("clear dr: %d", ret);
    }
    data->in_int = false;
}

#ifdef CONFIG_PINNACLE_TRIGGER_OWN_THREAD
static void pinnacle_thread(void *arg) {
    const struct device *dev = arg;
    struct pinnacle_data *data = dev->data;

    while (1) {
        k_sem_take(&data->gpio_sem, K_FOREVER);
        pinnacle_int_cb(dev);
    }
}
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
static void pinnacle_work_cb(struct k_work *work) {
    struct pinnacle_data *data = CONTAINER_OF(work, struct pinnacle_data, work);
    pinnacle_int_cb(data->dev);
}
#endif

static void pinnacle_gpio_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    struct pinnacle_data *data = CONTAINER_OF(cb, struct pinnacle_data, gpio_cb);
    data->in_int = true;
#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
    k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
    k_work_submit(&data->work);
#endif
}
#endif

static int pinnacle_init(const struct device *dev) {
    struct pinnacle_data *data = dev->data;
    const struct pinnacle_config *config = dev->config;

    LOG_WRN("pinnacle start");
    data->in_int = false;
    int ret;
    ret = pinnacle_write(dev, PINNACLE_SYS_CFG, PINNACLE_SYS_CFG_RESET);
    if (ret < 0) {
        LOG_ERR("can't reset %d", ret);
        return ret;
    }
    k_msleep(20);
    ret = pinnacle_write(dev, PINNACLE_STATUS1, 0);   // Clear CC
    if (ret < 0) {
        LOG_ERR("can't write %d", ret);
        return ret;
    }
    k_usleep(50);
    ret = pinnacle_write(dev, PINNACLE_Z_IDLE, 0);    // No Z-Idle packets
    if (ret < 0) {
        LOG_ERR("can't write %d", ret);
        return ret;
    }
    if (config->sleep_en) {
        ret = pinnacle_write(dev, PINNACLE_SYS_CFG, PINNACLE_SYS_CFG_EN_SLEEP);
        if (ret < 0) {
            LOG_ERR("can't write %d", ret);
            return ret;
        }
    }
    uint8_t feed_cfg2 = PINNACLE_FEED_CFG2_EN_IM;
    if (config->no_taps) {
        feed_cfg2 |= PINNACLE_FEED_CFG2_DIS_TAP;
    }
    if (config->rotate_90) {
        feed_cfg2 |= PINNACLE_FEED_CFG2_ROTATE_90;
    }
    ret = pinnacle_write(dev, PINNACLE_FEED_CFG2, feed_cfg2);
    if (ret < 0) {
        LOG_ERR("can't write %d", ret);
        return ret;
    }
    uint8_t feed_cfg1 = PINNACLE_FEED_CFG1_EN_FEED;
    if (feed_cfg1) {
        ret = pinnacle_write(dev, PINNACLE_FEED_CFG1, feed_cfg1);
    }
    if (ret < 0) {
        LOG_ERR("can't write %d", ret);
        return ret;
    }

#ifdef CONFIG_PINNACLE_TRIGGER
    data->dev = dev;
    gpio_pin_configure_dt(&config->dr, GPIO_INPUT);
    gpio_init_callback(&data->gpio_cb, pinnacle_gpio_cb, BIT(config->dr.pin));
    ret = gpio_add_callback(config->dr.port, &data->gpio_cb);
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

#define PINNACLE_INST(n) \
    static struct pinnacle_data pinnacle_data_##n; \
    static const struct pinnacle_config pinnacle_config_##n = { \
        .bus = COND_CODE_1(DT_INST_ON_BUS(0, i2c), (I2C_DT_SPEC_INST_GET(0)), (SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_LINES_SINGLE | SPI_TRANSFER_MSB, 0))), \
        .rotate_90 = DT_INST_PROP(0, rotate_90), \
        .sleep_en = DT_INST_PROP(0, sleep), \
        .no_taps = DT_INST_PROP(0, no_taps), \
        COND_CODE_1(CONFIG_PINNACLE_TRIGGER, (.dr = GPIO_DT_SPEC_GET(DT_DRV_INST(0), dr_gpios),), ()) \
    }; \
    DEVICE_DT_INST_DEFINE(n, pinnacle_init, NULL, &pinnacle_data_##n, &pinnacle_config_##n, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &pinnacle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PINNACLE_INST)
