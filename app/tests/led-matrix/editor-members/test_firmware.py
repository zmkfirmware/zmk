"""Exercise a numpadv2 Studio ELF with Unicorn and pyelftools.

Run: python test_firmware.py numpad.elf [numpad-with-empty-catalog-overlay.elf]
The CPU runs the firmware's actual editor, settings loader and overlay renderers.
The scheduler, hardware and flash backend are replaced at the firmware boundary.
"""
import struct
import sys
import zlib

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
                               UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC)


def words(*values):
    return struct.pack(f"<{len(values)}I", *values)


def encode(values):
    return b"".join(struct.pack("<BH", key, len(data)) + data for key, data in values.items())


class Controller:
    ram = 0x20000000
    scratch = 0x30000000
    stop = scratch + 0xf000
    regs = (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)

    def __init__(self, path):
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        for address, size in [(0, 0x100000), (self.ram, 0x80000), (self.scratch, 0x10000)]:
            self.cpu.mem_map(address, size)
        with open(path, "rb") as source:
            elf = ELFFile(source)
            self.symbols = {s.name: s.entry.st_value for s in elf.get_section_by_name(".symtab").iter_symbols()}
            for segment in elf.iter_segments():
                if segment["p_type"] == "PT_LOAD" and segment["p_filesz"]:
                    self.cpu.mem_write(segment["p_vaddr"], segment.data())
            self.offsets = self.snapshot_offsets(elf)
        self.flash = {}
        self.full = False
        self.ticket = 1
        for name in ("z_impl_k_mutex_lock", "z_impl_k_mutex_unlock", "z_impl_k_uptime_ticks",
                     "k_work_submit_to_queue", "k_work_schedule_for_queue", "k_work_reschedule_for_queue",
                     "k_work_cancel_delayable", "z_impl_k_timer_start", "z_impl_k_timer_stop",
                     "zmk_event_manager_raise", "zmk_ble_active_profile_is_open",
                     "zmk_ble_active_profile_is_connected", "zmk_usb_get_conn_state",
                     "zmk_hid_indicators_get_current_profile", "zmk_endpoint_get_selected"):
            if name in self.symbols:
                self.stub(self.symbols[name], lambda *_: 0)
        self.stub(self.symbols["z_impl_device_is_ready"], lambda *_: 1)
        self.stub(self.symbols["settings_save_one"], self.store)
        if "settings_delete" in self.symbols:
            self.stub(self.symbols["settings_delete"], lambda key, *_: self.store(key, 0, 0, 0))
        self.reader = self.stop - 16
        self.cpu.mem_write(self.reader, b"\x70\x47")
        self.stub(self.reader, self.read_flash)
        assert self.call("zmk_led_matrix_init") == 0
        self.boot = bytes(self.cpu.mem_read(self.ram, 0x80000))

    @staticmethod
    def snapshot_offsets(elf):
        for cu in elf.get_dwarf_info().iter_CUs():
            for die in cu.iter_DIEs():
                name = die.attributes.get("DW_AT_name")
                if die.tag == "DW_TAG_structure_type" and name and name.value == b"led_matrix_snapshot" and die.has_children:
                    return {d.attributes["DW_AT_name"].value.decode():
                            d.attributes["DW_AT_data_member_location"].value for d in die.iter_children()}
        raise ValueError("Build with debug information for overlay tests")

    def stub(self, address, handler):
        def hook(cpu, *_):
            value = handler(*(cpu.reg_read(reg) for reg in self.regs))
            cpu.reg_write(self.regs[0], value & 0xffffffff)
            cpu.reg_write(UC_ARM_REG_PC, cpu.reg_read(UC_ARM_REG_LR))
        self.cpu.hook_add(UC_HOOK_CODE, hook, begin=address & ~1, end=address & ~1)

    def call(self, name, *args):
        for reg, value in zip(self.regs, args):
            self.cpu.reg_write(reg, value)
        self.cpu.reg_write(UC_ARM_REG_SP, self.ram + 0x7ff00)
        self.cpu.reg_write(UC_ARM_REG_LR, self.stop | 1)
        self.cpu.emu_start(self.symbols[name] | 1, self.stop, count=2000000)
        assert self.cpu.reg_read(UC_ARM_REG_PC) == self.stop, name
        return self.cpu.reg_read(self.regs[0])

    def store(self, key, pointer, size, _):
        name = bytes(self.cpu.mem_read(key, 32)).split(b"\0")[0].decode()
        if self.full:
            return -28
        if size:
            self.flash[name] = bytes(self.cpu.mem_read(pointer, size))
        else:
            self.flash.pop(name, None)
        return 0

    def read_flash(self, pointer, out, size, _):
        self.cpu.mem_write(out, bytes(self.cpu.mem_read(pointer, size)))
        return size

    def reboot(self):
        self.cpu.mem_write(self.ram, self.boot)
        for name, record in reversed(list(self.flash.items())):
            self.cpu.mem_write(self.scratch, name.split("/")[1].encode() + b"\0")
            self.cpu.mem_write(self.scratch + 64, record)
            assert not self.call("editor_load", self.scratch, len(record), self.reader | 1, self.scratch + 64)
        assert not self.call("editor_load_commit")

    def packet(self, op, offset=0, data=b"", error=0):
        fmt = "<HBBHH12s"
        self.cpu.mem_write(self.scratch, struct.pack(fmt, 0, op, len(data), offset, self.ticket, data))
        self.call("zmk_lighting_exchange", self.scratch, self.scratch + 32)
        _, reply, length, size, ticket, data = struct.unpack(fmt, self.cpu.mem_read(self.scratch + 32, 20))
        assert ticket == self.ticket
        assert (reply == 255 and data[0] == error) if error else reply == op, (op, reply, data)
        return size, data[:length]

    def read(self, op, key, *options):
        self.ticket += 1
        size, _ = self.packet(op, data=words(key, *options))
        return b"".join(self.packet(3, i)[1] for i in range(0, size, 12))

    def group(self, key):
        raw = self.read(2, key)
        revision, factory, _, count = struct.unpack_from("<IIHH", raw)
        members = struct.unpack_from(f"<{count}H", raw, 12)
        pos = 12 + 2 * count
        values = {}
        while pos < len(raw):
            field, length = struct.unpack_from("<BH", raw, pos)
            pos += 3
            values[field] = raw[pos:pos+length]
            pos += length
        return revision, factory, members, values

    def apply(self, key, changes, error=0):
        revision, _, _, values = self.group(key)
        data = encode({**values, **changes})
        self.ticket += 1
        self.packet(4, len(data), words(key, revision))
        for i in range(0, len(data), 12):
            self.packet(5, i, data[i:i+12])
        self.packet(6, error=error)
        return self.group(key)

    def snapshot(self, **fields):
        for name, value in fields.items():
            self.cpu.mem_write(self.symbols["snapshot"] + self.offsets[name], bytes([value]))

    def color(self, led):
        return int.from_bytes(self.cpu.mem_read(self.symbols["logical_leds"] + led * 3, 3), "big")


def check(path):
    c = Controller(path)
    directory = c.read(1, 0)
    offset = 3 + directory[2]
    first, capacity, first_main, mains, count = struct.unpack_from("<BBBBH", directory, offset)
    mapping = list(struct.iter_unpack("<HH", directory[offset+6:offset+6+4*count]))
    assert directory[0] == 3 and mains == 4 and first_main == 251 and first == 1 and capacity >= 2 and count == 24
    assert mapping[1] == (65535, 0) and mapping[4] == (0, 65535)

    geometry = c.read(1, 0, 4)
    assert geometry[0] == 4 and geometry[1:offset+6] == directory[1:offset+6]
    rows, cols, speed = struct.unpack_from("<HHB", geometry, offset+6)
    assert rows > 1 and cols > 1 and speed > 0
    cells = list(struct.iter_unpack("<4H", geometry[offset+11:offset+11+8*count]))
    assert [cell[:2] for cell in cells] == mapping
    assert cells[1][2:] == (65535, 65535) and cells[4][2:] == (0, 0)
    assert all((row < rows and col < cols) or (row, col) == (65535, 65535)
               for _, _, row, col in cells)

    builtin, battery = 2 << 16, 4 << 16
    custom, other = builtin | (first << 8), builtin | ((first+1) << 8)
    ble, caps, num = battery + 2, battery + 3, battery + 4
    # Inject a real v1 override with the old per-profile topology checksum.
    old = c.group(builtin)
    signature = zlib.crc32(words(builtin) + struct.pack("<4H", *old[2]) + bytes([1, 2, 3, 4, 16]))
    main = 1 << 16
    original_main = c.group(main)
    main_crc = zlib.crc32(words(main) + struct.pack("<24H", *original_main[2]) +
                          bytes([1, 2, 3, 4, 5, 6, 7, 16]))
    c.flash["lm_editor/010000"] = bytes([1]) + words(main_crc) + encode({3: words(77)})
    c.flash["lm_editor/020000"] = bytes([1]) + words(signature) + encode({3: words(63)})
    c.reboot()
    assert c.group(main)[2] == tuple(range(24)) and c.group(main)[3][3] == words(77)
    assert c.group(builtin)[3][3] == words(63)
    # Recreate v2/v3 records with their complete old TLV sets and topology CRCs.
    for key, version in ((main, 3), (builtin, 2)):
        changes = {2: words(1), 3: words(71)}
        saved = c.apply(key, changes)
        name = f"lm_editor/{key:06x}"
        record = c.flash[name]
        assert record[0] == 4
        old_record = bytearray([version])
        offset = 1
        while offset < len(record):
            group_key, crc, length = struct.unpack_from("<IIH", record, offset)
            offset += 10
            end = offset + length
            previous = bytearray()
            while offset < end:
                field, size = struct.unpack_from("<BH", record, offset)
                if field not in (22, 23):
                    previous.extend(record[offset:offset+3+size])
                offset += 3 + size
            old_record.extend(struct.pack("<IIH", group_key, crc, len(previous)) + previous)
        c.flash[name] = bytes(old_record)
        c.reboot()
        assert c.group(key) == saved
        c.apply(key, {2: words(13), 23: words(1)})
        assert c.flash[name][0] == 4
        c.reboot()
        assert c.group(key)[3][23] == words(1)
        c.apply(key, {2: words(1), 23: words(0), 3: words(77 if key == main else 63)})
    c.apply(main, {2: words(15), 22: words(2)}, error=1)
    c.apply(builtin, {2: words(9)}, error=1)
    c.apply(builtin, {19: words(0, 2), 20: b"Underglow"})
    g = c.apply(custom, {19: words(1, 8), 16: words(0xaabbcc, 0x123456), 20: b"Mixed"})
    c.apply(other, {19: words(9), 20: b"Spare"})
    c.apply(custom, {19: words(0, 8)}, error=3)
    c.apply(custom, {19: words(8, 8)}, error=1)
    c.full = True
    assert c.apply(custom, {20: b"Lost write"}, error=2) == g
    c.full = False
    c.packet(7, data=words(custom, old[0]), error=3)
    assert g[2] == (1, 8)
    c.apply(battery, {19: words(8, 1), 16: words(0xff0000), 17: words(0), 18: b""})
    c.apply(ble, {19: words(1, 8), 17: words(0x00ff00), 18: words(0x00ff00)})
    c.apply(num, {19: words(8), 16: words(0x112233), 17: words(0x111111)})
    c.apply(caps, {19: words(1), 16: words(0xef1234), 17: words(0)})
    c.reboot()
    assert c.group(custom) == g and c.group(builtin)[3][3] == words(63)
    c.snapshot(battery_soc=50)
    c.call("render_battery_overlay", 0)
    assert (c.color(8), c.color(1)) == (0xff0000, 0)
    for profile, expected in [(0, (0, 0xff00)), (1, (0xff00, 0))]:
        c.snapshot(ble_profile=profile, ble_connected=1)
        c.call("render_ble_overlay", 0)
        assert (c.color(8), c.color(1)) == expected
    for locks, expected in [(0, (0x111111, 0)), (1, (0x112233, 0)), (2, (0x111111, 0xef1234))]:
        c.snapshot(hid=locks)
        c.call("render_hid_overlay", 0)
        assert (c.color(8), c.color(1)) == expected
    # Drive the real frame compositor, including independent rendering and the
    # status overwrite order. Hardware sinks and status-event inputs are stubbed.
    sources = 0
    c.stub(c.symbols["current_status_sources"], lambda *_: sources)
    for symbol in ("led_matrix_sync_ext_power", "flush_matrix", "led_matrix_sync_tick_state"):
        if symbol in c.symbols:
            c.stub(c.symbols[symbol], lambda *_: 0)
    c.call("zmk_led_matrix_on")
    for sources, expected in [(0, (0x123456, 0xaabbcc)), (1 | 4 | 8, (0x111111, 0xef1234))]:
        c.cpu.mem_write(c.symbols["led_matrix_service_requests"], words(1))
        c.call("led_matrix_render_work_handler", 0)
        assert (c.color(8), c.color(1)) == expected, (sources, hex(c.color(8)), hex(c.color(1)))
    c.packet(7, data=words(custom, g[0]))
    c.reboot()
    assert c.group(custom)[2] == () and c.group(other)[2] == (9,)
    c.packet(7, data=words(builtin, c.group(builtin)[0]))
    assert c.group(builtin) == old
    # Ordered main members share LEDs with independent, layer and status overlays.
    sources = 0
    preset = main | first_main
    inactive = c.group(preset)
    assert inactive[3][16] == words(0xffffff) and inactive[3][21] == words(1)
    scene = c.apply(preset, {1: b"Custom", 19: words(6, 4, 1), 16: words(0xabcdef, 0x123456, 0xff), 21: words(0)})
    c.reboot()
    assert c.group(preset) == scene and not c.call("zmk_led_matrix_set_profile", first_main)
    c.call("zmk_led_matrix_on")
    def frame():
        c.cpu.mem_write(c.symbols["led_matrix_service_requests"], words(1))
        c.call("led_matrix_render_work_handler", 0)
        return tuple(c.color(i) for i in (4, 5, 6, 1))
    assert frame() == (0x123456, 0, 0xabcdef, 0)
    assert c.call("zmk_led_matrix_calc_profile", 1) == 0
    layer = (3 << 16) | 1
    c.apply(layer, {21: words(0), 19: words(5, 1), 16: words(0x331122, 0x771122), 8: words(0)})
    c.snapshot(active_layer=1)
    c.call("led_matrix_refresh_layer_color", 1)
    assert frame() == (0x123456, 0x331122, 0xabcdef, 0x771122)
    sources = 8
    c.snapshot(hid=2)
    assert frame()[-1] == 0xef1234
    sources = 0
    c.apply(layer, {21: words(1)})
    assert frame() == (0x123456, 0, 0xabcdef, 0)
    c.packet(7, data=words(preset, scene[0]))
    assert c.call("zmk_led_matrix_get_profile") == 0
    c.reboot()
    assert c.group(preset) == inactive and c.group(layer)[3][21] == words(1)
    print("PASS main mask, ordered color output, active layer/status priority, removal, restart and active preset fallback")
    print("PASS negotiated directory geometry, v1/v2/v3 migration, mixed groups, conflicts, flash failure, restart, delete, defaults and indicator frames")


def check_empty_catalog(path):
    c = Controller(path)
    main, layer = 1 << 16, 3 << 16
    g = c.group(layer)
    assert g[2] == (4, 5)
    crc = zlib.crc32(words(layer) + struct.pack("<2H", *g[2]) + bytes([3, 8, 16]))
    c.flash["lm_editor/030000"] = (
        bytes([1]) + words(crc) + encode({16: words(0x123456, 0xAABBCC)})
    )
    c.reboot()
    assert c.group(layer)[2] == (4, 5) and c.group(layer)[3][16] == words(
        0x123456, 0xAABBCC
    )
    c.apply(layer, {19: words(7), 16: words(0x909090)})
    c.packet(7, data=words(layer, c.group(layer)[0]))
    assert c.group(layer) == g
    assert c.call("zmk_led_matrix_get_profile") == 255
    for id in (251, 252):
        c.apply(main | id, {21: words(0), 1: b"Test"})
    assert c.call("zmk_led_matrix_get_profile") == 251
    assert c.call("zmk_led_matrix_set_profile", 252) == 0
    # Bootloader presents saved active ID before editor groups on typical settings backends.
    c.cpu.mem_write(c.ram, c.boot)
    c.cpu.mem_write(c.scratch, b"state\0")
    c.cpu.mem_write(c.scratch + 64, bytes([1, 80, 252]))
    assert not c.call(
        "led_matrix_settings_load_cb", c.scratch, 3, c.reader | 1, c.scratch + 64
    )
    for key, raw in c.flash.items():
        c.cpu.mem_write(c.scratch, key.split("/")[1].encode() + b"\0")
        c.cpu.mem_write(c.scratch + 64, raw)
        assert not c.call(
            "editor_load", c.scratch, len(raw), c.reader | 1, c.scratch + 64
        )
    assert (
        not c.call("editor_load_commit") and c.call("zmk_led_matrix_get_profile") == 252
    )
    for id, fallback in ((252, 251), (251, 255)):
        c.packet(7, data=words(main | id, c.group(main | id)[0]))
        assert c.call("zmk_led_matrix_get_profile") == fallback
    print(
        "PASS v1 layer migration, restore, first custom activation, reboot selected ID, remaining-custom fallback, empty catalogue"
    )


if __name__ == "__main__":
    check(sys.argv[1])
    if len(sys.argv) > 2:
        check_empty_catalog(sys.argv[2])
