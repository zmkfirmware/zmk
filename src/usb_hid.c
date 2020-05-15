
#include <device.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <dt-bindings/zmk/keys.h>

#include "hid.h"
#include "keymap.h"

LOG_MODULE_REGISTER(zmk_usb_hid, CONFIG_ZMK_USB_HID_LOG_LEVEL);

static enum usb_dc_status_code usb_status;

static struct device *hid_dev;

int zmk_usb_hid_send_report(const struct zmk_hid_report *report)
{
	if (usb_status == USB_DC_SUSPEND)
	{
		return usb_wakeup_request();
	}

	return hid_int_ep_write(hid_dev, (u8_t *)report, sizeof(struct zmk_hid_report), NULL);
}

void usb_hid_status_cb(enum usb_dc_status_code status, const u8_t *params)
{
	usb_status = status;
};

int zmk_usb_hid_init()
{
	int usb_enable_ret;

	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL)
	{
		LOG_ERR("Unable to locate HID device");
		return -EINVAL;
	}

	usb_hid_register_device(hid_dev,
							zmk_hid_report_desc, sizeof(zmk_hid_report_desc),
							NULL);

	usb_hid_init(hid_dev);

	usb_enable_ret = usb_enable(usb_hid_status_cb);

	if (usb_enable_ret != 0)
	{
		LOG_ERR("Unable to enable USB");
		return -EINVAL;
	}

	return 0;
}
