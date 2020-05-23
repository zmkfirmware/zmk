
#include <zmk/ble.h>
#include <zmk/handlers.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/matrix.h>

#ifdef CONFIG_ZMK_ACTION_MOD_TAP
u16_t action_effect_pending = 0;
#endif

__attribute__((weak)) bool zmk_handle_key_user(struct zmk_key_event *key_event)
{
	return true;
};

bool zmk_handle_action(zmk_action action, struct zmk_key_event *key_event)
{
	zmk_mod mods = ZK_MODS(key_event->key);
	u8_t flattened_index = (key_event->row * ZMK_MATRIX_COLS) + key_event->column;
	switch (action)
	{
#ifdef CONFIG_ZMK_ACTION_MOD_TAP
	case ZMK_ACTION_MOD_TAP:
		if (key_event->pressed)
		{
			WRITE_BIT(action_effect_pending, flattened_index, true);
			zmk_hid_register_mods(mods);
		}
		else
		{
			zmk_hid_unregister_mods(mods);
			if (action_effect_pending & BIT(flattened_index))
			{
				// Allow baseline keycode to flow to the endpoints!
				return true;
			}
			else
			{
				// Since not sending a keycode, at least send the report w/ the mod removed
				zmk_endpoints_send_report();
			}
		}
		break;
#endif
	}
	return false;
};

void zmk_handle_key(struct zmk_key_event key_event)
{
	zmk_action action = ZK_ACTION(key_event.key);

	if (!zmk_handle_key_user(&key_event))
	{
		return;
	}

	if (action && !zmk_handle_action(action, &key_event))
	{
		return;
	}

#ifdef CONFIG_ZMK_ACTION_MOD_TAP
	action_effect_pending = 0;
#endif

#ifdef CONFIG_ZMK_BLE
	/* Used for intercepting key presses when doing passkey verification */
	if (!zmk_ble_handle_key_user(&key_event))
	{
		return;
	}
#endif /* CONFIG_ZMK_BLE */

	zmk_endpoints_send_key_event(key_event);
};
