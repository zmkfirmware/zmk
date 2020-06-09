
#include <zmk/keys.h>
#include <zmk/keymap.h>
#include <keymap.h>

bool zmk_handle_key_user(struct zmk_key_event *key_event)
{
	switch (key_event->key)
	{
	case CC_LOWR:
		if (key_event->pressed)
		{
			zmk_keymap_layer_activate(1);
		}
		else
		{
			zmk_keymap_layer_deactivate(1);
		}

		return false;
	case CC_RAIS:
		if (key_event->pressed)
		{
			zmk_keymap_layer_activate(2);
		}
		else
		{
			zmk_keymap_layer_deactivate(2);
		}
		return false;
	}

	return true;
};
