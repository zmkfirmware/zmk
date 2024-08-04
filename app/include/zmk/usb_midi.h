/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/usb/usb_device.h>
#include <zmk/midi.h>
#include <usb_descriptor.h>

// TODO can add this back as a config option, but for now hardcode it
#define USB_MIDI_NUM_INPUTS 1
#define USB_MIDI_NUM_OUTPUTS 1

#define USB_MIDI_DEFAULT_CABLE_NUM 0
#define USB_MIDI_MAX_NUM_BYTES 3

// TODO: these are hardcoded here for usb_write, but bEndpointAddress actually get assigned
// automatically in the usb configs hard coding them in the usb configs doesn't seem to help, so we
// don't have a good way of ensuring that the endpoint addresses defined here actually match what
// zephyr gives our endpoints you can see what the endpoint addresses are by doing "cat
// /sys/kernel/debug/usb/devices" when the device is plugged in eventually we should find a way to
// get this information back out of the usb device configuration
#define ZMK_USB_MIDI_EP_IN 0x81
#define ZMK_USB_MIDI_EP_OUT 0x01

/* Require at least one jack */
BUILD_ASSERT((USB_MIDI_NUM_INPUTS + USB_MIDI_NUM_OUTPUTS > 0),
             "USB MIDI device must have more than 0 jacks");

/**
 * MS (MIDI streaming) Class-Specific Interface Descriptor Subtypes.
 * See table A.1 in the spec.
 */
enum usb_midi_if_desc_subtype {
    USB_MIDI_IF_DESC_UNDEFINED = 0x00,
    USB_MIDI_IF_DESC_MS_HEADER = 0x01,
    USB_MIDI_IF_DESC_MIDI_IN_JACK = 0x02,
    USB_MIDI_IF_DESC_MIDI_OUT_JACK = 0x03,
    USB_MIDI_IF_DESC_ELEMENT = 0x04
};

/**
 * MS Class-Specific Endpoint Descriptor Subtypes.
 * See table A.2 in the spec.
 */
enum usb_midi_ep_desc_subtype {
    USB_MIDI_EP_DESC_UNDEFINED = 0x00,
    USB_MIDI_EP_DESC_MS_GENERAL = 0x01
};

/**
 * MS MIDI IN and OUT Jack types.
 * See table A.3 in the spec.
 */
enum usb_midi_jack_type {
    USB_MIDI_JACK_TYPE_UNDEFINED = 0x00,
    USB_MIDI_JACK_TYPE_EMBEDDED = 0x01,
    USB_MIDI_JACK_TYPE_EXTERNAL = 0x02
};

#define USB_MIDI_AUDIO_INTERFACE_CLASS 0x01
#define USB_MIDI_MIDISTREAMING_INTERFACE_SUBCLASS 0x03
#define USB_MIDI_AUDIOCONTROL_INTERFACE_SUBCLASS 0x01

/**
 * USB MIDI input pin.
 */
struct usb_midi_input_pin {
    uint8_t baSourceID;
    uint8_t baSourcePin;
} __packed;

/**
 * Class-specific AC (audio control) Interface Descriptor.
 */
struct usb_midi_ac_if_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint16_t bcdADC;
    uint16_t wTotalLength;
    uint8_t bInCollection;
    uint8_t baInterfaceNr;
} __packed;

/**
 * Class-Specific MS Interface Header Descriptor.
 * See table 6.2 in the spec.
 */
struct usb_midi_ms_if_descriptor {
    /** Size of this descriptor, in bytes */
    uint8_t bLength;
    /** CS_INTERFACE descriptor type */
    uint8_t bDescriptorType;
    /** MS_HEADER descriptor subtype. */
    uint8_t bDescriptorSubtype;
    /**
     * MIDIStreaming SubClass Specification Release Number in
     * Binary-Coded Decimal. Currently 01.00.
     */
    uint16_t BcdADC;
    /**
     * Total number of bytes returned for the class-specific
     * MIDIStreaming interface descriptor. Includes the combined
     * length of this descriptor header and all Jack and Element descriptors.
     */
    uint16_t wTotalLength;
} __packed;

/**
 * MIDI IN Jack Descriptor. See table 6.3 in the spec.
 */
struct usb_midi_in_jack_descriptor {
    /** Size of this descriptor, in bytes. */
    uint8_t bLength;
    /** CS_INTERFACE descriptor type. */
    uint8_t bDescriptorType;
    /** MIDI_IN_JACK descriptor subtype. */
    uint8_t bDescriptorSubtype;
    /** EMBEDDED or EXTERNAL */
    uint8_t bJackType;
    /**
     * Constant uniquely identifying the MIDI IN Jack within
     * the USB-MIDI function.
     */
    uint8_t bJackID;
    /** Index of a string descriptor, describing the MIDI IN Jack. */
    uint8_t iJack;
} __packed;

/**
 * MIDI OUT Jack Descriptor. See table 6.4 in the spec.
 */
struct usb_midi_out_jack_descriptor {
    /** Size of this descriptor, in bytes: */
    uint8_t bLength;
    /** CS_INTERFACE descriptor type. */
    uint8_t bDescriptorType;
    /** MIDI_OUT_JACK descriptor subtype. */
    uint8_t bDescriptorSubtype;
    /** EMBEDDED or EXTERNAL */
    uint8_t bJackType;
    /**
     * Constant uniquely identifying the MIDI OUT Jack
     * within the USB-MIDI function.
     */
    uint8_t bJackID;
    /**
     * Number of Input Pins of this MIDI OUT Jack
     * (assumed to be 1 in this implementation).
     */
    uint8_t bNrInputPins;
    /** ID and source pin of the entity to which this jack is connected. */
    struct usb_midi_input_pin input_pin;
    /** Index of a string descriptor, describing the MIDI OUT Jack. */
    uint8_t iJack;
} __packed;

/**
 * The same as Zephyr's usb_ep_descriptor but with two additional fields
 * to match the USB MIDI spec.
 */
struct usb_ep_descriptor_padded {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
    /* The following two attributes were added to match the USB MIDI spec. */
    uint8_t bRefresh;
    uint8_t bSynchAddress;
} __packed;

/**
 * Class-Specific MS Bulk Data Endpoint Descriptor
 * corresponding to a MIDI output. See table 6-7 in the spec.
 */
struct usb_midi_bulk_out_ep_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bNumEmbMIDIJack;
    uint8_t BaAssocJackID[USB_MIDI_NUM_INPUTS];
} __packed;

/**
 * Class-Specific MS Bulk Data Endpoint Descriptor
 * corresponding to a MIDI input. See table 6-7 in the spec.
 */
struct usb_midi_bulk_in_ep_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bNumEmbMIDIJack;
    uint8_t BaAssocJackID[USB_MIDI_NUM_OUTPUTS];
} __packed;

#define USB_MIDI_ELEMENT_CAPS_COUNT 1

/**
 * Element descriptor. See table 6-5 in the spec.
 */
struct usb_midi_element_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bElementID;
    uint8_t bNrInputPins;

    struct usb_midi_input_pin input_pins[USB_MIDI_NUM_INPUTS];
    uint8_t bNrOutputPins;
    uint8_t bInTerminalLink;
    uint8_t bOutTerminalLink;
    uint8_t bElCapsSize;
    uint8_t bmElementCaps[USB_MIDI_ELEMENT_CAPS_COUNT];
    uint8_t iElement;
} __packed;

/**
 * A complete set of descriptors for a USB MIDI device without physical jacks.
 */
struct usb_midi_config {
    struct usb_if_descriptor ac_if;
    struct usb_midi_ac_if_descriptor ac_cs_if;
    struct usb_if_descriptor ms_if;
    struct usb_midi_ms_if_descriptor ms_cs_if;
    struct usb_midi_in_jack_descriptor in_jacks_emb[USB_MIDI_NUM_INPUTS];
    struct usb_midi_out_jack_descriptor out_jacks_emb[USB_MIDI_NUM_OUTPUTS];
    struct usb_midi_element_descriptor element;
    struct usb_ep_descriptor_padded out_ep;
    struct usb_midi_bulk_out_ep_descriptor out_cs_ep;
    struct usb_ep_descriptor_padded in_ep;
    struct usb_midi_bulk_in_ep_descriptor in_cs_ep;
} __packed;

/* No jack string descriptors by default  */
#define INPUT_JACK_STRING_DESCR_IDX(jack_idx) 0
#define OUTPUT_JACK_STRING_DESCR_IDX(jack_idx) 0

/* Audio control interface descriptor */
#define INIT_AC_IF                                                                                 \
    {                                                                                              \
        .bLength = sizeof(struct usb_if_descriptor), .bDescriptorType = USB_DESC_INTERFACE,        \
        .bInterfaceNumber = 0, .bAlternateSetting = 0, .bNumEndpoints = 0,                         \
        .bInterfaceClass = USB_MIDI_AUDIO_INTERFACE_CLASS,                                         \
        .bInterfaceSubClass = USB_MIDI_AUDIOCONTROL_INTERFACE_SUBCLASS,                            \
        .bInterfaceProtocol = 0x00, .iInterface = 0x00                                             \
    }

/* Class specific audio control interface descriptor */
#define INIT_AC_CS_IF                                                                              \
    {                                                                                              \
        .bLength = sizeof(struct usb_midi_ac_if_descriptor),                                       \
        .bDescriptorType = USB_DESC_CS_INTERFACE, .bDescriptorSubtype = 0x01, .bcdADC = 0x0100,    \
        .wTotalLength = sizeof(struct usb_midi_ac_if_descriptor), .bInCollection = 0x01,           \
        .baInterfaceNr = 0x01                                                                      \
    }

/* MIDI streaming interface descriptor */
#define INIT_MS_IF                                                                                 \
    {                                                                                              \
        .bLength = sizeof(struct usb_if_descriptor), .bDescriptorType = USB_DESC_INTERFACE,        \
        .bInterfaceNumber = 0x01, .bAlternateSetting = 0x00, .bNumEndpoints = 2,                   \
        .bInterfaceClass = USB_MIDI_AUDIO_INTERFACE_CLASS,                                         \
        .bInterfaceSubClass = USB_MIDI_MIDISTREAMING_INTERFACE_SUBCLASS,                           \
        .bInterfaceProtocol = 0x00, .iInterface = 0x00                                             \
    }

/* Class specific MIDI streaming interface descriptor */
#define INIT_MS_CS_IF                                                                              \
    {                                                                                              \
        .bLength = sizeof(struct usb_midi_ms_if_descriptor),                                       \
        .bDescriptorType = USB_DESC_CS_INTERFACE, .bDescriptorSubtype = 0x01, .BcdADC = 0x0100,    \
        .wTotalLength = MIDI_MS_IF_DESC_TOTAL_SIZE                                                 \
    }

/* Embedded MIDI input jack */
#define INIT_IN_JACK(idx, idx_offset)                                                              \
    {                                                                                              \
        .bLength = sizeof(struct usb_midi_in_jack_descriptor),                                     \
        .bDescriptorType = USB_DESC_CS_INTERFACE,                                                  \
        .bDescriptorSubtype = USB_MIDI_IF_DESC_MIDI_IN_JACK,                                       \
        .bJackType = USB_MIDI_JACK_TYPE_EMBEDDED, .bJackID = 1 + idx + idx_offset,                 \
        .iJack = INPUT_JACK_STRING_DESCR_IDX(idx),                                                 \
    }

/* Embedded MIDI output jack */
#define INIT_OUT_JACK(idx, jack_id_idx_offset)                                                     \
    {                                                                                              \
        .bLength = sizeof(struct usb_midi_out_jack_descriptor),                                    \
        .bDescriptorType = USB_DESC_CS_INTERFACE,                                                  \
        .bDescriptorSubtype = USB_MIDI_IF_DESC_MIDI_OUT_JACK,                                      \
        .bJackType = USB_MIDI_JACK_TYPE_EMBEDDED, .bJackID = 1 + idx + jack_id_idx_offset,         \
        .bNrInputPins = 0x01,                                                                      \
        .input_pin =                                                                               \
            {                                                                                      \
                .baSourceID = ELEMENT_ID,                                                          \
                .baSourcePin = 1 + idx,                                                            \
            },                                                                                     \
        .iJack = OUTPUT_JACK_STRING_DESCR_IDX(idx)                                                 \
    }

/* Out endpoint */
#define INIT_OUT_EP                                                                                \
    {                                                                                              \
        .bLength = sizeof(struct usb_ep_descriptor_padded), .bDescriptorType = USB_DESC_ENDPOINT,  \
        .bEndpointAddress = ZMK_USB_MIDI_EP_OUT, .bmAttributes = 0x02, .wMaxPacketSize = 0x0040,   \
        .bInterval = 0x00, .bRefresh = 0x00, .bSynchAddress = 0x00,                                \
    }

/* In endpoint */
#define INIT_IN_EP                                                                                 \
    {                                                                                              \
        .bLength = sizeof(struct usb_ep_descriptor_padded), .bDescriptorType = USB_DESC_ENDPOINT,  \
        .bEndpointAddress = ZMK_USB_MIDI_EP_IN, .bmAttributes = 0x02, .wMaxPacketSize = 0x0040,    \
        .bInterval = 0x00, .bRefresh = 0x00, .bSynchAddress = 0x00,                                \
    }

#define ELEMENT_ID 0xf0
#define IDX_WITH_OFFSET(index, offset) (index + offset)
#define INIT_INPUT_PIN(index, offset)                                                              \
    { .baSourceID = (index + offset), .baSourcePin = 1 }

#define INIT_ELEMENT                                                                               \
    {                                                                                              \
        .bLength = sizeof(struct usb_midi_element_descriptor),                                     \
        .bDescriptorType = USB_DESC_CS_INTERFACE, .bDescriptorSubtype = USB_MIDI_IF_DESC_ELEMENT,  \
        .bElementID = ELEMENT_ID, .bNrInputPins = USB_MIDI_NUM_INPUTS,                             \
        .input_pins = {LISTIFY(USB_MIDI_NUM_INPUTS, INIT_INPUT_PIN, (, ), 1)},                     \
        .bNrOutputPins = USB_MIDI_NUM_OUTPUTS, .bInTerminalLink = 0, .bOutTerminalLink = 0,        \
        .bElCapsSize = 1, .bmElementCaps = 1, .iElement = 0                                        \
    }

/* Value for the wTotalLength field of the class-specific MS Interface Descriptor,
     i.e the total number of bytes following that descriptor. */
#define MIDI_MS_IF_DESC_TOTAL_SIZE                                                                 \
    (sizeof(struct usb_midi_in_jack_descriptor) * USB_MIDI_NUM_INPUTS +                            \
     sizeof(struct usb_midi_out_jack_descriptor) * USB_MIDI_NUM_OUTPUTS +                          \
     sizeof(struct usb_midi_element_descriptor) + sizeof(struct usb_ep_descriptor_padded) +        \
     sizeof(struct usb_midi_bulk_out_ep_descriptor) + sizeof(struct usb_ep_descriptor_padded) +    \
     sizeof(struct usb_midi_bulk_in_ep_descriptor))

int zmk_usb_send_midi_report(struct zmk_midi_key_report_body *body);
