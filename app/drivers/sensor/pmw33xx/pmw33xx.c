

#define DT_DRV_COMPAT pixart_pmw33xx

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
#ifdef CONFIG_PMW33XX_3389
#include <pmw3389_srom.h>
#elif CONFIG_PMW33XX_3360
#include <pmw3360_srom.h>
#endif

#include "pmw33xx.h"

LOG_MODULE_REGISTER(PMW33XX, CONFIG_SENSOR_LOG_LEVEL);
#define PMW33XX_PID COND_CODE_1(CONFIG_PMW33XX_3389, (PMW33XX_3389_PID), (PMW33XX_3360_PID))

struct pmw33xx_motion_burst {
	uint8_t motion;
	uint8_t observation;
	int16_t dx;
	int16_t dy;
} __attribute__((packed));

static inline int pmw33xx_cs_select(const struct pmw33xx_gpio_dt_spec *cs_gpio_cfg,
				    const uint8_t value)
{
	return gpio_pin_set(cs_gpio_cfg->port, cs_gpio_cfg->pin, value);
}

static int pmw33xx_access(const struct device *dev, const uint8_t reg, uint8_t *value)
{
	struct pmw33xx_data *data = dev->data;
	const struct pmw33xx_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
	const struct pmw33xx_gpio_dt_spec *cs_gpio_cfg = &cfg->bus_cfg.spi_cfg->cs_spec;

	uint8_t access[1] = { reg };
	struct spi_buf_set tx = {
		.buffers = { &(struct spi_buf){
			.buf = access,
			.len = 1,
		} },
		.count = 1,
	};
	uint8_t result[1] = { *value };
	struct spi_buf_set rx = {
		.buffers = { &(struct spi_buf){
			.buf = result,
			.len = 1,
		} },
		.count = 1,
	};

	pmw33xx_cs_select(cs_gpio_cfg, 0);

	int err = spi_write(data->bus, spi_cfg, &tx);
	k_sleep(K_USEC(120)); //Tsrad
	if (err) {
		pmw33xx_cs_select(cs_gpio_cfg, 1);
		return err;
	}

	if ((reg & PMW33XX_WR_MASK))
		err = spi_write(data->bus, spi_cfg, &rx);
	else
		err = spi_read(data->bus, spi_cfg, &rx);
	pmw33xx_cs_select(cs_gpio_cfg, 1);
	k_sleep(K_USEC(160));
	if ((reg & PMW33XX_WR_MASK) == 0)
		*value = result[0];
	return err;
}
static int pmw33xx_read_reg(const struct device *dev, const uint8_t reg, uint8_t *value)
{
	return pmw33xx_access(dev, reg & PMW33XX_RD_MASK, value);
}
static int pmw33xx_write_reg(const struct device *dev, const uint8_t reg, const uint8_t value)
{
	uint8_t v = value;
	return pmw33xx_access(dev, reg | PMW33XX_WR_MASK, &v);
}
static int pmw33xx_write_srom(const struct device *dev)
{
	struct pmw33xx_data *data = dev->data;
	const struct pmw33xx_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
	const struct pmw33xx_gpio_dt_spec *cs_gpio_cfg = &cfg->bus_cfg.spi_cfg->cs_spec;
	uint8_t access[1] = { PMW33XX_REG_SROM_BURST | PMW33XX_WR_MASK };
	struct spi_buf_set tx = {
		.buffers = { &(struct spi_buf){
			.buf = access,
			.len = 1,
		} },
		.count = 1,
	};

	pmw33xx_write_reg(dev, PMW33XX_REG_SROM_EN, PMW33XX_SROM_DWNLD_CMD);
	k_sleep(K_USEC(15));
	pmw33xx_write_reg(dev, PMW33XX_REG_SROM_EN, PMW33XX_SROM_DWNLD_START_CMD);

	pmw33xx_cs_select(cs_gpio_cfg, 0);

	int err = spi_write(data->bus, spi_cfg, &tx);

	k_sleep(K_USEC(15));
	if (err) {
		pmw33xx_cs_select(cs_gpio_cfg, 1);
		return err;
	}

	for (uint16_t i = 0; i < sizeof(SROM); i++) {
		access[0] = SROM[i];
		err = spi_write(data->bus, spi_cfg, &tx);
		k_sleep(K_USEC(15));
		if (err) {
			pmw33xx_cs_select(cs_gpio_cfg, 1);
			return err;
		}
	}

	pmw33xx_cs_select(cs_gpio_cfg, 1);
	k_sleep(K_MSEC(2)); //Tbexit
	return err;
}

static int pmw33xx_read_motion_burst(const struct device *dev, struct pmw33xx_motion_burst *burst)
{
	struct pmw33xx_data *data = dev->data;
	const struct pmw33xx_config *cfg = dev->config;
	const struct spi_config *spi_cfg = &cfg->bus_cfg.spi_cfg->spi_conf;
	const struct pmw33xx_gpio_dt_spec *cs_gpio_cfg = &cfg->bus_cfg.spi_cfg->cs_spec;

	uint8_t access[1] = { PMW33XX_REG_BURST };
	struct spi_buf_set tx = {
		.buffers = { &(struct spi_buf){
			.buf = access,
			.len = 1,
		} },
		.count = 1,
	};
	struct spi_buf_set rx = {
		.buffers = { &(struct spi_buf){
			.buf = (uint8_t *)burst,
			.len = sizeof(struct pmw33xx_motion_burst),
		} },
		.count = 1,
	};

	pmw33xx_cs_select(cs_gpio_cfg, 0);

	int err = spi_write(data->bus, spi_cfg, &tx);
	k_sleep(K_USEC(35)); // tsrad motbr
	if (err) {
		pmw33xx_cs_select(cs_gpio_cfg, 1);
		return err;
	}
	err = spi_read(data->bus, spi_cfg, &rx);
	pmw33xx_cs_select(cs_gpio_cfg, 1);
	return err;
}

// converts twos complement data to an int16
static int16_t pmw33xx_raw_to_int16(const uint8_t src[2])
{
	int16_t res = sys_get_be16(src);
	if (res > BIT_MASK(15))
		res -= BIT(16);
	return res;
}

static int pmw33xx_read_raw(const struct device *dev, const uint8_t reg_high, const uint8_t reg_low,
			    int16_t *value)
{
	uint8_t raw[2] = { 0x0, 0x0 };
	int err;
	err = pmw33xx_read_reg(dev, reg_high, &raw[0]);
	if (err) {
		LOG_ERR("could not read high byte at %x", reg_high);
		return err;
	}
	k_sleep(K_USEC(100));
	err = pmw33xx_read_reg(dev, reg_low, &raw[1]);
	if (err) {
		LOG_ERR("could not read low byte at %x", reg_low);
		return err;
	}
	k_sleep(K_USEC(100));
	*value = pmw33xx_raw_to_int16(raw);
	return 0;
}

int pmw33xx_spi_init(const struct device *dev)
{
	struct pmw33xx_data *data = dev->data;
	const struct pmw33xx_config *cfg = dev->config;
	const struct pmw33xx_spi_cfg *spi_cfg = cfg->bus_cfg.spi_cfg;
	const struct pmw33xx_gpio_dt_spec *cs_gpio_cfg = &cfg->bus_cfg.spi_cfg->cs_spec;

	int err;
	err = gpio_pin_configure(cs_gpio_cfg->port, cs_gpio_cfg->pin, GPIO_OUTPUT_ACTIVE);
	if (err) {
		LOG_ERR("could configure cs pin %d", err);
		return -EIO;
	}
	return 0;
}

static int pmw33xx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct pmw33xx_data *data = dev->data;
	const struct pmw33xx_config *cfg = dev->config;
	struct pmw33xx_motion_burst burst;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_POS_DX && chan != SENSOR_CHAN_POS_DY)
		return -ENOTSUP;

	int err = pmw33xx_read_motion_burst(dev, &burst);
	if (err) {
		return err;
	}
	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DX)
		data->dx = burst.dx;
	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_POS_DY)
		data->dy = burst.dy;
	return 0;
}

static int pmw33xx_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct pmw33xx_data *data = dev->data;
	const struct pmw33xx_config *cfg = dev->config;

	switch (chan) {
	case SENSOR_CHAN_POS_DX:
		val->val1 = data->dx;
		data->dx = 0;
		break;
	case SENSOR_CHAN_POS_DY:
		val->val1 = data->dy;
		data->dy = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api pmw33xx_driver_api = {
#ifdef CONFIG_PMW33XX_TRIGGER
	.trigger_set = pmw33xx_trigger_set,
#endif
	// .attr_set = pmw33xx_attr_set,
	.sample_fetch = pmw33xx_sample_fetch,
	.channel_get = pmw33xx_channel_get,
};

static int pmw33xx_init_chip(const struct device *dev)
{
	struct pmw33xx_data *data = dev->data;
	return 0;
}

static int pmw33xx_init(const struct device *dev)
{
	const struct pmw33xx_config *const config = dev->config;
	struct pmw33xx_data *data = dev->data;

	data->bus = device_get_binding(config->bus_name);
	if (!data->bus) {
		LOG_DBG("master not found: %s", log_strdup(config->bus_name));
		return -EINVAL;
	}

	config->bus_init(dev);

	if (pmw33xx_init_chip(dev) < 0) {
		LOG_DBG("failed to initialize chip");
		return -EIO;
	}

	const struct pmw33xx_gpio_dt_spec *cs_gpio_cfg = &config->bus_cfg.spi_cfg->cs_spec;

#ifdef CONFIG_PMW33XX_TRIGGER
	if (pmw33xx_init_interrupt(dev) < 0) {
		LOG_DBG("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	pmw33xx_cs_select(cs_gpio_cfg, 1);
	k_sleep(K_MSEC(1));

	int err = pmw33xx_write_reg(dev, PMW33XX_REG_PWR_UP_RST, PMW33XX_RESET_CMD);
	if (err) {
		LOG_ERR("could not reset %d", err);
		return -EIO;
	}
	uint8_t pid = 0x0;
	err = pmw33xx_read_reg(dev, PMW33XX_REG_PID, &pid);
	if (err) {
		LOG_ERR("could not reset %d", err);
		return -EIO;
	}
	if (pid != PMW33XX_PID) {
		LOG_ERR("pid does not match expected: got (%x), expected(%x)", pid, PMW33XX_PID);
		return -EIO;
	}
	pmw33xx_write_reg(dev, PMW33XX_REG_CONFIG2, 0x00); // clear rest enable

	err = pmw33xx_write_srom(dev);
	if (err) {
		LOG_ERR("could not upload srom %d", err);
		return -EIO;
	}
	uint8_t srom_run = 0x0;
	err = pmw33xx_read_reg(dev, PMW33XX_REG_OBSERVATION, &srom_run);
	if (err) {
		LOG_ERR("could not check srom status %d", err);
		return -EIO;
	}
	if (!(srom_run & PMW33XX_SROM_RUN)) {
		LOG_ERR("srom status invalid %d", srom_run);
		return -EIO;
	}

	uint8_t srom_id = 0x0;
	err = pmw33xx_read_reg(dev, PMW33XX_REG_SROM_ID, &srom_id);
	if (err) {
		LOG_ERR("could not check srom id %d", err);
		return -EIO;
	}
	if (!srom_id) {
		LOG_ERR("srom id invalid %d", srom_id);
		return -EIO;
	}

	pmw33xx_write_reg(dev, PMW33XX_REG_CONFIG2, 0x00); // clear rest enable
	pmw33xx_write_reg(dev, PMW33XX_REG_BURST, 0x01); // clear rest enable
	struct pmw33xx_motion_burst val;
	pmw33xx_read_motion_burst(dev, &val); // read and throwout initial motion data

	return 0;
}

#define PMW33XX_DATA_SPI(n)                                                                        \
	{                                                                                          \
		.cs_ctrl = {},                                                                     \
	}

#define PMW33XX_SPI_CFG(n)                                                                         \
	(&(struct pmw33xx_spi_cfg){                                                                    \
        .spi_conf =                                                                                \
            {                                                                                      \
                .frequency = DT_INST_PROP(n, spi_max_frequency),                                   \
                .operation =                                                                       \
                    (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA),        \
                .slave = DT_INST_REG_ADDR(n),                                                      \
            },                                                                                     \
	   .cs_spec = PMW33XX_GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), cs_gpios, 0), \
    })

#define PMW33XX_GPIO_DT_SPEC_GET_BY_IDX(node_id, prop, idx)                                        \
	{                                                                                          \
		.port = DEVICE_DT_GET(DT_GPIO_CTLR_BY_IDX(node_id, prop, idx)),                    \
		.pin = DT_GPIO_PIN_BY_IDX(node_id, prop, idx),                                     \
		.dt_flags = DT_GPIO_FLAGS_BY_IDX(node_id, prop, idx),                              \
	}

#define PMW33XX_CONFIG_SPI(n)                                                                      \
	{                                                                                          \
		.bus_name = DT_INST_BUS_LABEL(n), .bus_init = pmw33xx_spi_init,                    \
		.bus_cfg = { .spi_cfg = PMW33XX_SPI_CFG(n) },                                      \
		COND_CODE_1(CONFIG_MA730_TRIGGER,                                                  \
			    (, PMW33XX_GPIO_DT_SPEC_GET_BY_IDX(DT_DRV_INST(n), motswk_gpios, 0)),  \
			    ())                                                                    \
	}

#define PMW33XX_INST(n)                                                                            \
	static struct pmw33xx_data pmw33xx_data_##n = PMW33XX_DATA_SPI(n);                         \
	static const struct pmw33xx_config pmw33xx_cfg_##n = PMW33XX_CONFIG_SPI(n);                \
	DEVICE_DT_INST_DEFINE(n, pmw33xx_init, device_pm_control_nop, &pmw33xx_data_##n,           \
			      &pmw33xx_cfg_##n, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,          \
			      &pmw33xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PMW33XX_INST)
