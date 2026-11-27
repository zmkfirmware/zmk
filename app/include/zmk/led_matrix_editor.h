/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdint.h>
#include <zephyr/toolchain.h>

/* Editor exchange, little endian. The whole packet fits the minimum ATT payload.
 * Side 0 is local. Studio supplies a transfer ticket and the split central supplies a
 * per-packet sequence. READ offsets refer to one immutable DIRECTORY or GROUP snapshot.
 * DIRECTORY: version:u8, sources:u8, board:string, then {key:u32, label:string,
 * optional zone preset name:string} entries. Strings have a one-byte length.
 * Directory v2 inserts first_custom:u8, custom_slots:u8, LED count:u16 and
 * (layout position:u16, independent owner:u16) per LED after board. 0xffff
 * denotes no mapping/owner. DIRECTORY's key selects the physical layout.
 * Directory v3 inserts first_custom_main:u8, custom_main_slots:u8 immediately
 * before LED count. All layer slots are readable by stable keymap layer ID.
 * DIRECTORY requests may append a u32 maximum version after layout. Version 4 adds
 * grid rows:u16, cols:u16 and default speed:u8 after LED count, then row:u16 and
 * col:u16 after each owner (0xffff for LEDs outside the grid).
 * Animation fields 22 and 23 are u32 renderer-relative style and direction (0/1).
 * Main and layer groups include ordered members; TLV 21 is a u32 disabled flag
 * for layers and custom mains. Inactive main slots are omitted from DIRECTORY.
 * FACTORY_GROUP snapshots factory values with the same format as GROUP.
 * Group TLVs 19 and 20 carry ordered u32 members and the zone's UTF-8 name.
 * Custom slot profile 0 can be read before creation; saving creates it and
 * restoring its empty factory configuration deletes it.
 * GROUP: revision:u32, factory_revision:u32, renderer_mask:u16, led_count:u16,
 * logical LEDs:u16[], then TLVs (field:u8, byte_length:u16, value).
 * BEGIN includes key and expected revision, with total byte length in offset.
 * APPEND is sequential. COMMIT installs a complete validated set of TLVs after saving.
 * RESTORE also checks the revision. LIVE and SHOW leave transfers untouched.
 */
struct zmk_lighting_packet {
    uint16_t sequence;
    uint8_t op;
    uint8_t length;
    uint16_t offset;
    uint16_t ticket;
    uint8_t data[12];
} __packed;

enum zmk_lighting_op {
    LM_DIRECTORY = 1,
    LM_GROUP,
    LM_READ,
    LM_BEGIN,
    LM_APPEND,
    LM_COMMIT,
    LM_RESTORE,
    LM_LIVE,
    LM_SHOW,
    /* Internal split transport only. Carries highest active layer and selected main preset. */
    LM_RUNTIME,
    LM_FACTORY_GROUP,
    LM_ERROR = 255
};
enum zmk_lighting_error {
    LM_INVALID = 1,
    LM_FULL,
    LM_CONFLICT,
    LM_OFFLINE,
    LM_UNSUPPORTED,
    LM_IO,
    LM_EXPIRED,
    LM_NOT_READY
};

void zmk_lighting_exchange(const struct zmk_lighting_packet *request,
                           struct zmk_lighting_packet *reply);
int zmk_lighting_forward(uint8_t source, struct zmk_lighting_packet *packet);
void zmk_lighting_receive_reply(uint8_t source, const struct zmk_lighting_packet *packet);

void zmk_lighting_sync_runtime(void);
void zmk_lighting_apply_runtime(uint8_t layer, uint8_t profile);
