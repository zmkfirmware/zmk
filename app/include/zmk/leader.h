/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

void zmk_leader_activate(int32_t timeout, bool timeout_on_activation, uint32_t position);
void zmk_leader_deactivate();
bool zmk_leader_get_status();
