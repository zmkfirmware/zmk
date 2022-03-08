
#define DT_DRV_COMPAT pixart_pmw3389

#include <drivers/spi.h>
#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "pmw3389.h"

LOG_MODULE_REGISTER(PMW3389, CONFIG_SENSOR_LOG_LEVEL);

static int pmw3389_access(const struct device *dev,
                          uint8_t reg, uint8_t *value)
{
    struct pmw3389_data *data = dev->data;
    const struct pmw3389_config *cfg = dev->config;
    const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
    uint8_t access[1] = {reg};
    struct spi_buf bufs[] = {
        {
            .buf = access,
            .len = 1,
        },
        {
            .buf = value,
            .len = 1,
        },
    };
    struct spi_buf_set tx = {
        .buffers = bufs,
        .count = 2,
    };
    struct spi_buf_set *rx;
    if (reg & PMW3389_WR_MASK > 0)
    {
        rx = &(struct spi_buf_set){
            .buffers = bufs,
            .count = 2,
        };
    }
    return spi_transceive(data->bus, spi_cfg, &tx, rx);
}
static int pmw3389_read_reg(const struct device *dev, const uint8_t reg, uint8_t *value)
{
    return pmw3389_access(dev, reg, value);
}
static int pmw3389_write_reg(const struct device *dev, const uint8_t reg, const uint8_t value)
{
    return pmw3389_access(dev, reg & PMW3389_WR_MASK, &value);
}

// converts twos complement data to an int16
static int16_t pmw3389_raw_to_int16(const uint8_t src[2])
{
    int16_t res = sys_get_be16(src[2]);
    if (res > BIT_MASK(15))
        res -= BIT(16);
    return res;
}

static int pmw3389_read_raw(const struct device *dev, const uint8_t reg_high, const uint8_t reg_low, int16_t *value)
{
    uint8_t raw[2] = {0x0, 0x0};
    int err;
    err = pmw3389_read_reg(dev, reg_high, &raw[0]);
    if (err)
    {
        LOG_ERR("could not read high byte at %x", reg_high);
        return err;
    }
    err = pmw3389_read_reg(dev, reg_low, &raw[1]);
    if (err)
    {
        LOG_ERR("could not read low byte at %x", reg_low);
        return err;
    }
    *value = pmw3389_raw_to_int16(raw);
    return 0;
}

static bool pmw3389_spi_check_id(const struct device *dev)
{
    int err;
    uint8_t val;
    err = pmw3389_read_reg(dev, &val, PMW3389_REG_PID);
    if (err)
    {
        LOG_ERR("could not read PID");
        return false;
    }
    if (val != PMW3389_PID)
    {
        LOG_ERR("invalid PID");
        return false;
    }
    err = pmw3389_read_reg(dev, &val, PMW3389_REG_REV);
    if (err)
    {
        LOG_ERR("could not read REV");
        return false;
    }
    if (val != PMW3389_REV)
    {
        LOG_ERR("invalid REV");
        return false;
    }
    return true;
}

int pmw3389_spi_init(const struct device *dev)
{
    struct pmw3389_data *data = dev->data;
    const struct pmw3389_config *cfg = dev->config;
    const struct pmw3389_spi_cfg *spi_cfg = cfg->bus_cfg.spi_cfg;

    if (spi_cfg->cs_gpios_label != NULL)
    {

        /* handle SPI CS thru GPIO if it is the case */
        data->cs_ctrl.gpio_dev = device_get_binding(spi_cfg->cs_gpios_label);
        if (!data->cs_ctrl.gpio_dev)
        {
            LOG_ERR("Unable to get GPIO SPI CS device");
            return -ENODEV;
        }
    }

    return 0;
}

static int pmw3389_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct pmw3389_data *data = dev->data;
    const struct pmw3389_config *cfg = dev->config;
    uint16_t dx = 0, dy = 0;

    if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_POS_DX &&
        chan != SENSOR_CHAN_POS_DY)
        return -ENOTSUP;
    if (!pmw3389_spi_check_id(dev))
        return -EINVAL;
    int err;
    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DX)
    {
        err = pmw3389_read_raw(dev, PMW3389_REG_DX_H, PMW3389_REG_DX_L, &dx);
        if (err)
        {
            LOG_DBG("could not read x motion");
            return -EIO;
        }
    }
    if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DY)
    {
        err = pmw3389_read_raw(dev, PMW3389_REG_DY_H, PMW3389_REG_DY_L, &dy);
        if (err)
        {
            LOG_DBG("could not read y motion");
            return -EIO;
        }
    }
    data->dx = dx;
    data->dy = dy;
    return 0;
}

static int pmw3389_channel_get(const struct device *dev, enum sensor_channel chan,
                               struct sensor_value *val)
{
    struct pmw3389_data *data = dev->data;
    const struct pmw3389_config *cfg = dev->config;

    switch (chan)
    {
    case SENSOR_CHAN_POS_DX:
        return data->dx;
    case SENSOR_CHAN_POS_DY:
        return data->dy;
    default:
        return -ENOTSUP;
    }
    return 0;
}

static const struct sensor_driver_api pmw3389_driver_api = {
#ifdef CONFIG_PMW3389_TRIGGER
    .trigger_set = pmw3389_trigger_set,
#endif
    // .attr_set = pmw3389_attr_set,
    .sample_fetch = pmw3389_sample_fetch,
    .channel_get = pmw3389_channel_get,
};

static int pmw3389_init_chip(const struct device *dev)
{
    struct pmw3389_data *data = dev->data;
    return 0;
}

static int pmw3389_init(const struct device *dev)
{
    const struct pmw3389_config *const config = dev->config;
    struct pmw3389_data *data = dev->data;

    data->bus = device_get_binding(config->bus_name);
    if (!data->bus)
    {
        LOG_DBG("master not found: %s", log_strdup(config->bus_name));
        return -EINVAL;
    }

    config->bus_init(dev);

    if (pmw3389_init_chip(dev) < 0)
    {
        LOG_DBG("failed to initialize chip");
        return -EIO;
    }

#ifdef CONFIG_PMW3389_TRIGGER
    if (pmw3389_init_interrupt(dev) < 0)
    {
        LOG_DBG("Failed to initialize interrupt!");
        return -EIO;
    }
#endif

    return 0;
}

#define PMW3389_HAS_CS(n) DT_INST_SPI_DEV_HAS_CS_GPIOS(n)

#define PMW3389_DATA_SPI_CS(n)                                  \
    {                                                           \
        .cs_ctrl = {                                            \
            .gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(n),        \
            .gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(n), \
        },                                                      \
    }

#define PMW3389_DATA_SPI(n) COND_CODE_1(PMW3389_HAS_CS(n), (PMW3389_DATA_SPI_CS(n)), ({}))

#define PMW3389_SPI_CS_PTR(n) COND_CODE_1(PMW3389_HAS_CS(n), (&(pmw3389_data_##n.cs_ctrl)), (NULL))

#define PMW3389_SPI_CS_LABEL(n) \
    COND_CODE_1(PMW3389_HAS_CS(n), (DT_INST_SPI_DEV_CS_GPIOS_LABEL(n)), (NULL))

#define PMW3389_SPI_CFG(n)                                                                  \
    (&(struct pmw3389_spi_cfg){                                                             \
        .spi_conf =                                                                         \
            {                                                                               \
                .frequency = DT_INST_PROP(n, spi_max_frequency),                            \
                .operation =                                                                \
                    (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA), \
                .slave = DT_INST_REG_ADDR(n),                                               \
                .cs = PMW3389_SPI_CS_PTR(n),                                                \
            },                                                                              \
        .cs_gpios_label = PMW3389_SPI_CS_LABEL(n),                                          \
    })

#define PMW3389_GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)             \
    {                                                                   \
        .port = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node_id, prop, idx)), \
        .pin = DT_GPIO_PIN_BY_IDX(node_id, prop, idx),                  \
        .dt_flags = DT_GPIO_FLAGS_BY_IDX(node_id, prop, idx),           \
    }

#define PMW3389_CONFIG_SPI(n)                                                                   \
    {                                                                                           \
        .bus_name = DT_INST_BUS_LABEL(n), .bus_init = pmw3389_spi_init,                         \
        .bus_cfg = {.spi_cfg = PMW3389_SPI_CFG(n)},                                             \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, resolution), (1), (DT_INST_PROP(n, resolution)))   \
            COND_CODE_1(CONFIG_PMW3389_TRIGGER,                                                 \
                        (, PMW3389_GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), motswk_gpios, 0), ), \
                        ())                                                                     \
    }

#define PMW3389_INST(n)                                                                                \
    static struct pmw3389_data pmw3389_data_##n = PMW3389_DATA_SPI(n);                                 \
    static const struct pmw3389_config pmw3389_cfg_##n = PMW3389_CONFIG_SPI(n);                        \
    DEVICE_DT_INST_DEFINE(n, pmw3389_init, device_pm_control_nop, &pmw3389_data_##n, &pmw3389_cfg_##n, \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &pmw3389_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PMW3389_INST)