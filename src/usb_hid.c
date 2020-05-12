
#include <device.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <dt-bindings/zmk/keys.h>

#include "keymap.h"

LOG_MODULE_REGISTER(zmk_usb_hid, CONFIG_ZMK_USB_HID_LOG_LEVEL);

#define MAX_KEYCODE KC_APP

static const u8_t hid_report_desc[] = {
	/* USAGE_PAGE (Generic Desktop) */
	HID_GI_USAGE_PAGE,
	USAGE_GEN_DESKTOP,
	/* USAGE (Keyboard) */
	HID_LI_USAGE,
	USAGE_GEN_DESKTOP_KEYBOARD,
	/* COLLECTION (Application) */
	HID_MI_COLLECTION,
	COLLECTION_APPLICATION,
	/* USAGE_PAGE (Keypad) */
	HID_GI_USAGE_PAGE,
	USAGE_GEN_DESKTOP_KEYPAD,
	/* USAGE_MINIMUM (Keyboard LeftControl) */
	HID_LI_USAGE_MIN(1),
	0xE0,
	/* USAGE_MAXIMUM (Keyboard Right GUI) */
	HID_LI_USAGE_MAX(1),
	0xE7,
	/* LOGICAL_MINIMUM (0) */
	HID_GI_LOGICAL_MIN(1),
	0x00,
	/* LOGICAL_MAXIMUM (1) */
	HID_GI_LOGICAL_MAX(1),
	0x01,
	/* REPORT_SIZE (1) */
	HID_GI_REPORT_SIZE,
	0x01,
	/* REPORT_COUNT (8) */
	HID_GI_REPORT_COUNT,
	0x08,
	/* INPUT (Data,Var,Abs) */
	HID_MI_INPUT,
	0x02,
	/* USAGE_PAGE (Keypad) */
	HID_GI_USAGE_PAGE,
	USAGE_GEN_DESKTOP_KEYPAD,
	/* REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE,
	0x08,
	/* REPORT_COUNT (1) */
	HID_GI_REPORT_COUNT,
	0x07,
	/* INPUT (Cnst,Var,Abs) */
	HID_MI_INPUT,
	0x03,

	/* USAGE_PAGE (Keypad) */
	HID_GI_USAGE_PAGE,
	USAGE_GEN_DESKTOP_KEYPAD,
	/* LOGICAL_MINIMUM (0) */
	HID_GI_LOGICAL_MIN(1),
	0x00,
	/* LOGICAL_MAXIMUM (101) */
	HID_GI_LOGICAL_MAX(1),
	0x01,
	/* USAGE_MINIMUM (Reserved) */
	HID_LI_USAGE_MIN(1),
	0x00,
	/* USAGE_MAXIMUM (Keyboard Application) */
	HID_LI_USAGE_MAX(1),
	MAX_KEYCODE,
	/* REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE,
	0x01,
	/* REPORT_COUNT (6) */
	HID_GI_REPORT_COUNT,
	MAX_KEYCODE + 1,
	/* INPUT (Data,Ary,Abs) */
	HID_MI_INPUT,
	0x02,
	/* USAGE_PAGE (Keypad) */
	HID_GI_USAGE_PAGE,
	USAGE_GEN_DESKTOP_KEYPAD,
	/* REPORT_SIZE (8) */
	HID_GI_REPORT_SIZE,
	0x02,
	/* REPORT_COUNT (6) */
	HID_GI_REPORT_COUNT,
	0x01,
	/* INPUT (Cnst,Var,Abs) */
	HID_MI_INPUT,
	0x03,
	/* END_COLLECTION */
	HID_MI_COLLECTION_END,
};

static enum usb_dc_status_code usb_status;

static struct device *hid_dev;
struct boot_report
{
	u8_t modifiers;
	u8_t _unused;
	u8_t keys[6];
} __packed;

struct extended_report
{
	u8_t keys[13];
} __packed;

struct hid_report
{
	struct boot_report boot;
	struct extended_report extended;
} __packed report;

#define KEY_OFFSET 0x02
#define MAX_KEYS 6

#define TOGGLE_BOOT_KEY(match, val)                      \
	for (int idx = 0; idx < MAX_KEYS; idx++)             \
	{                                                    \
		if (report.boot.keys[idx + KEY_OFFSET] != match) \
		{                                                \
			continue;                                    \
		}                                                \
		report.boot.keys[idx + KEY_OFFSET] = val;        \
		break;                                           \
	}

#define TOGGLE_EXT_KEY(code, val) WRITE_BIT(report.extended.keys[code / 8], code % 8, val)

int zmk_usb_hid_press_key(zmk_key code)
{
	if (usb_status == USB_DC_SUSPEND)
	{
		return usb_wakeup_request();
	}

	if (code > MAX_KEYCODE)
	{
		return -EINVAL;
	}

	TOGGLE_BOOT_KEY(0U, code);

	TOGGLE_EXT_KEY(code, true);

	return hid_int_ep_write(hid_dev, (u8_t *)&report, sizeof(report), NULL);
}

int zmk_usb_hid_release_key(zmk_key code)
{
	if (code > MAX_KEYCODE)
	{
		return -EINVAL;
	}

	TOGGLE_BOOT_KEY(code, 0U);

	TOGGLE_EXT_KEY(code, false);

	return hid_int_ep_write(hid_dev, (u8_t *)&report, sizeof(report), NULL);
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
							hid_report_desc, sizeof(hid_report_desc),
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
