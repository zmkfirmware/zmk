/* SPDX-License-Identifier: MIT */
#include "ext_power_test_fakes.h"
#include "src/ext_power_generic.c"

static void reset(bool enabled, bool persistent) {
    data = (struct ext_power_generic_data){
        .status = enabled, .status_known = true, .persisted_status = persistent};
    test_device = (struct device){.name = "test-rail", .data = &data, .config = &config};
    fail_pin = -1;
    total_calls = save_schedules = save_writes = 0;
    saved_value = false;
    memset(pin_calls, 0, sizeof(pin_calls));
    for (size_t i = 0; i < 3; i++)
        pin_values[i] = enabled;
    ext_power_save_work = (struct k_work_delayable){0};
    k_work_init_delayable(&ext_power_save_work, ext_power_save_state_work);
}
static void expect_all(bool enabled) {
    for (size_t i = 0; i < 3; i++)
        assert(pin_values[i] == enabled);
    assert(data.status_known && ext_power_generic_get(&test_device) == enabled);
}
static void multi_gpio_failures(void) {
    for (int pin = 0; pin < 3; pin++) {
        reset(false, false);
        fail_pin = pin;
        assert(ext_power_generic_enable_runtime(&test_device) == -EIO);
        assert(!data.status_known && ext_power_generic_get(&test_device) == -EIO);
        assert(total_calls == 3 && !data.persisted_status && save_schedules == 0);
        for (size_t i = 0; i < 3; i++) {
            assert(pin_calls[i] == 1);
            assert(pin_values[i] == ((int)i != pin));
        }
        /* Old status is false: unknown must defeat the duplicate-OFF fast path. */
        fail_pin = -1;
        assert(ext_power_generic_disable_runtime(&test_device) == 0);
        assert(total_calls == 6);
        expect_all(false);
        assert(ext_power_generic_disable_runtime(&test_device) == 0 && total_calls == 6);

        reset(true, true);
        fail_pin = pin;
        assert(ext_power_generic_disable_runtime(&test_device) == -EIO);
        assert(!data.status_known && total_calls == 3 && data.persisted_status);
        for (size_t i = 0; i < 3; i++)
            assert(pin_calls[i] == 1);
        /* Old status is true: opposite retry must drive every control again. */
        fail_pin = -1;
        assert(ext_power_generic_enable_runtime(&test_device) == 0 && total_calls == 6);
        expect_all(true);
        assert(ext_power_generic_disable_runtime(&test_device) == 0 && total_calls == 9);
        expect_all(false);
        assert(data.persisted_status && save_schedules == 0 && save_writes == 0);
    }
    puts("PASS ext-power: first/middle/last GPIO errors attempt every pin; unknown state forces "
         "both rollback directions; stable repeats are silent");
}
static void runtime_and_persistence(void) {
    reset(false, true);
    for (int i = 0; i < 100; i++) {
        assert(ext_power_generic_enable_runtime(&test_device) == 0);
        expect_all(true);
        assert(ext_power_generic_disable_runtime(&test_device) == 0);
        expect_all(false);
    }
    assert(total_calls == 600 && data.persisted_status && save_schedules == 0 && save_writes == 0);
    reset(false, false);
    assert(ext_power_generic_enable(&test_device) == 0);
    assert(data.persisted_status && save_schedules == 1);
    assert(ext_power_generic_disable_runtime(&test_device) == 0 && save_schedules == 1);
    /* A pending save still writes user intent after a runtime power cycle. */
    ext_power_save_state_work(&ext_power_save_work.work);
    assert(save_writes == 1 && saved_value);
    assert(ext_power_generic_disable(&test_device) == 0);
    assert(!data.persisted_status && save_schedules == 2 && total_calls == 6);
    assert(ext_power_generic_disable(&test_device) == 0 && save_schedules == 2 && total_calls == 6);
    ext_power_save_state_work(&ext_power_save_work.work);
    assert(save_writes == 2 && !saved_value);
    for (int pin = 0; pin < 3; pin++) {
        reset(false, false);
        fail_pin = pin;
        assert(ext_power_generic_enable(&test_device) == -EIO);
        assert(!data.persisted_status && save_schedules == 0);
        reset(true, true);
        fail_pin = pin;
        assert(ext_power_generic_disable(&test_device) == -EIO);
        assert(data.persisted_status && save_schedules == 0);
    }
    puts("PASS ext-power: 100 runtime cycles never save or change persisted intent; deferred saves "
         "retain intent; failed persistent requests do not save");
}
static void initialization_and_pm(void) {
    reset(false, false);
    data.status_known = false;
    assert(ext_power_generic_get(&test_device) == -EIO);
    assert(ext_power_generic_disable_runtime(&test_device) == 0 && total_calls == 3);
    expect_all(false);
    assert(ext_power_generic_pm_action(&test_device, PM_DEVICE_ACTION_RESUME) == 0);
    expect_all(true);
    assert(ext_power_generic_pm_action(&test_device, PM_DEVICE_ACTION_SUSPEND) == 0);
    expect_all(false);
    assert(save_schedules == 0 && !data.persisted_status);
    reset(false, false);
    data.status_known = false;
    assert(ext_power_generic_init(&test_device) == 0);
    expect_all(true);
    assert(!data.persisted_status && save_schedules == 0);
    puts("PASS ext-power: initialization resolves unknown state; PM uses runtime semantics");
}
int main(void) {
    multi_gpio_failures();
    runtime_and_persistence();
    initialization_and_pm();
    return 0;
}
