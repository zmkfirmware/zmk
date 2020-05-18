
#include <zmk/ble.h>
#include <zmk/handlers.h>
#include <zmk/endpoints.h>

__attribute__((weak)) bool zmk_handle_key_user(struct zmk_key_event *key_event)
{
	return true;
};

void zmk_handle_key(struct zmk_key_event key_event)
{
	if (!zmk_handle_key_user(&key_event))
	{
		return;
	}

#ifdef CONFIG_ZMK_BLE
	if (!zmk_ble_handle_key_user(&key_event))
	{
		return;
	}
#endif /* CONFIG_ZMK_BLE */

	zmk_endpoints_send_key_event(key_event);
};
