
#include <device.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

LOG_MODULE_REGISTER(zmk_usb_hid, CONFIG_ZMK_USB_HID_LOG_LEVEL);

static const u8_t hid_report_desc[] = HID_KEYBOARD_REPORT_DESC();
static enum usb_dc_status_code usb_status;

static struct device *hid_dev;
static u8_t report[8] = { 0x00, 0x00 };

#define KEY_OFFSET 0x02
#define MAX_KEYS 6

int zmk_usb_hid_press_key(enum hid_kbd_code code)
{
	if (usb_status == USB_DC_SUSPEND) {
		return usb_wakeup_request();
	}

	for (int idx = 0; idx < MAX_KEYS; idx++) {
		if (report[idx + KEY_OFFSET] != 0U) {
			continue;
		}

		report[idx + KEY_OFFSET] = code;

		return hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
	}

	return -EINVAL;
}

int zmk_usb_hid_release_key(enum hid_kbd_code code)
{
	for (int idx = 0; idx < MAX_KEYS; idx++) {
		if (report[idx + KEY_OFFSET] != code) {
			continue;
		}

		report[idx + KEY_OFFSET] = 0U;

		return hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
	}

	return -EINVAL;
}

void usb_hid_status_cb(enum usb_dc_status_code status, const u8_t *params)
{
	usb_status = status;
};

int zmk_usb_hid_init()
{
	int usb_enable_ret;

	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		LOG_ERR("Unable to locate HID device");
		return -EINVAL;
	}

	usb_hid_register_device(hid_dev,
	                        hid_report_desc, sizeof(hid_report_desc),
	                        NULL);

	usb_hid_init(hid_dev);

	usb_enable_ret = usb_enable(usb_hid_status_cb);

	if (usb_enable_ret != 0) {
		LOG_ERR("Unable to enable USB");
		return -EINVAL;
	}

	return 0;
}
