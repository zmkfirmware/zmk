
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
			zmk_keymap_layer_activate(LOWER);
		}
		else
		{
			zmk_keymap_layer_deactivate(LOWER);
		}

		return false;
	case CC_RAIS:
		if (key_event->pressed)
		{
			zmk_keymap_layer_activate(RAISE);
		}
		else
		{
			zmk_keymap_layer_deactivate(RAISE);
		}
		return false;
	}

	return true;
};
