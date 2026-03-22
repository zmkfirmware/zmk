#pragma once

void set_peripheral_layers_state(uint32_t new_layers);
bool peripheral_layer_active(uint8_t layer);
uint8_t peripheral_highest_layer_active(void);