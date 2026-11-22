/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/split/transport/types.h>

int zmk_split_peripheral_report_event(const struct zmk_split_transport_peripheral_event *event);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC)

/**
 * @brief Store central state received from the central and raise the matching events for values
 * that changed.
 *
 * @retval -ENOTSUP if this peripheral was not built with the command's domain.
 */
int zmk_split_peripheral_state_sync_handle(const struct zmk_split_transport_central_command *cmd);

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC)
