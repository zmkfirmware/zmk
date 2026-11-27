// SPDX-License-Identifier: MIT
#pragma once

#include <zmk/studio/rpc.h>

// A session changes whenever the selected endpoint changes, even between BLE profiles.
uint32_t studio_tx_session(void);
void studio_tx_select(const struct zmk_rpc_transport *transport);
int studio_tx_send(const zmk_studio_Response *response, uint32_t session);

// Peek copies bytes into transport-owned memory. Complete only acknowledges the
// matching chunk, so late callbacks cannot consume a new session's data.
size_t studio_tx_peek(enum zmk_transport transport, uint8_t *buf, size_t size, uint32_t *ticket);
void studio_tx_complete(uint32_t ticket, int sent);
