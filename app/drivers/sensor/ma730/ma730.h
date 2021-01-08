#ifndef ZEPHYR_DRIVERS_SENSOR_MA730_MA730_H_
#define ZEPHYR_DRIVERS_SENSOR_MA730_MA730_H_

#include <drivers/sensor.h>
#include <zephyr/types.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <drivers/spi.h>




#define MA730_REG_ZERO_OFF_1 0x0

#define MA730_REG_ZERO_OFF_2 0x1

#define MA730_REG_BCT 0x2

#define MA730_REG_ETX_ETY 0x3

#define MA730_REG_PPT_ILIP 0x4
#define MA730_MASK_PPT_ILIP_PPT1 BIT(7)
#define MA730_SHIFT_PPT_ILIP_PPT1 7
#define MA730_MASK_PPT_ILIP_PPT0 BIT(7)
#define MA730_SHIFT_PPT_ILIP_PPT0 6

#define MA730_REG_PPT 0x5
#define MA730_MASK_PPT_PPT9 BIT(7)
#define MA730_SHIFT_PPT_PPT9 7
#define MA730_MASK_PPT_PPT8 BIT(6)
#define MA730_SHIFT_PPT_PPT8 6
#define MA730_MASK_PPT_PPT7 BIT(5)
#define MA730_SHIFT_PPT_PPT7 5
#define MA730_MASK_PPT_PPT6 BIT(4)
#define MA730_SHIFT_PPT_PPT6 4
#define MA730_MASK_PPT_PPT5 BIT(3)
#define MA730_SHIFT_PPT_PPT5 3
#define MA730_MASK_PPT_PPT4 BIT(2)
#define MA730_SHIFT_PPT_PPT4 2
#define MA730_MASK_PPT_PPT3 BIT(1)
#define MA730_SHIFT_PPT_PPT3 1
#define MA730_MASK_PPT_PPT2 BIT(0)
#define MA730_SHIFT_PPT_PPT2 0

#define MA730_REG_MGT 0x6
#define MA730_MASK_MGT_MGLT2 BIT(7)
#define MA730_SHIFT_MGT_MGLT2 7
#define MA730_MASK_MGT_MGLT1 BIT(6)
#define MA730_SHIFT_MGT_MGLT1 6
#define MA730_MASK_MGT_MGLT0 BIT(5)
#define MA730_SHIFT_MGT_MGLT0 5
#define MA730_MASK_MGT_MGHT2 BIT(4)
#define MA730_SHIFT_MGT_MGHT2 4
#define MA730_MASK_MGT_MGHT1 BIT(3)
#define MA730_SHIFT_MGT_MGHT1 3
#define MA730_MASK_MGT_MGHT0 BIT(2)
#define MA730_SHIFT_MGT_MGHT0 2

#define MA730_REG_RD 0x9
#define MA730_MASK_RD_RD BIT(7)
#define MA730_SHIFT_RD_RD 7

#define MA730_REG_MG 0x1B




struct ma730_spi_cfg {
	struct spi_config spi_conf;
	const char *cs_gpios_label;
};


struct ma730_config {
	char *bus_name;
	int (*bus_init)(const struct device *dev);
	const union ma730_spi_cfg spi_cfg;
	int resolution;
};

struct ma730_data;

struct ma730_transfer_function {
	int (*read_data)(const struct device *dev,
			 uint16_t *value);

};

struct ma730_data {
	const struct device *bus;
	struct spi_cs_control cs_ctrl;

	uint16_t angle;
	int8_t velocity;
	bool direction;

	const struct ma730_transfer_function *hw_tf;


};

int ma730_spi_init(const struct device *dev);


#endif /* ZEPHYR_DRIVERS_SENSOR_MA730_MA730_H_ */
