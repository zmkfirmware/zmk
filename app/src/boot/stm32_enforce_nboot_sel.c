#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/stm32_flash_api_extensions.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

const struct device *flash_dev = FIXED_PARTITION_DEVICE(storage_partition);

static int stm32_enforce_nboot_sel_init(void) {
    uint32_t opts = 0;
    int ret = 0;

    if (!device_is_ready(flash_dev)) {
        LOG_ERR("flash dev not ready");
        return 0;
    }

    ret = flash_ex_op(flash_dev, FLASH_STM32_EX_OP_OPTB_READ, (uintptr_t)NULL, &opts);

    if (ret < 0) {
        LOG_ERR("Failed to read option bytes with flash ext op (%d)\n", ret);
        return ret;
    }

    LOG_DBG("Current option bytes: %02X\n", opts);

    if (opts & FLASH_OPTR_nBOOT_SEL) {
        WRITE_BIT(opts, FLASH_OPTR_nBOOT_SEL_Pos, false);

        LOG_DBG("Writing new option bytes %02X\n", opts);
        ret = flash_ex_op(flash_dev, FLASH_STM32_EX_OP_OPTB_WRITE, opts, NULL);
        if (ret < 0) {
            LOG_ERR("Failed to write new option bytes (%d)", ret);
            return ret;
        }
    }

    return 0;
}

SYS_INIT(stm32_enforce_nboot_sel_init, APPLICATION, 10);
