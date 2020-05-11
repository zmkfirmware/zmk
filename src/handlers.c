
#include "handlers.h"

#include "usb_hid.h"

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

	if (key_event.pressed)
	{
		zmk_usb_hid_press_key(key_event.key);
	}
	else
	{
		zmk_usb_hid_release_key(key_event.key);
	}
};
