"""Run against the Studio Corne central and peripheral ELFs, in that order."""

import sys

from unicorn.arm_const import UC_ARM_REG_SP
from test_firmware import Controller, words


def check(left_path, right_path):
    left, right = Controller(left_path), Controller(right_path)
    preset, layer = 0x100FB, 0x30001
    for side, color in ((left, 0x336699), (right, 0x996633)):
        side.apply(preset, {1: b"Shared", 19: words(5), 16: words(color), 21: words(0)})
        side.apply(
            layer,
            {
                19: words(5),
                16: words(0x778899),
                3: words(100),
                8: words(0),
                21: words(0),
            },
        )
        side.reboot()
        for name in (
            "current_status_sources",
            "led_matrix_sync_ext_power",
            "flush_matrix",
            "led_matrix_sync_tick_state",
        ):
            if name in side.symbols:
                side.stub(side.symbols[name], lambda *_: 0)
        side.call("zmk_led_matrix_on")

    def pixel(side):
        side.cpu.mem_write(side.symbols["led_matrix_service_requests"], words(1))
        side.call("led_matrix_render_work_handler", 0)
        return side.color(5)

    # Route the actual ARM ABI command from the central worker through the
    # peripheral command handler. Only topology discovery, RF and scheduling are mocked.
    transport, api, send, discover = (
        left.scratch + 0x8000,
        left.scratch + 0x8040,
        left.stop - 64,
        left.stop - 80,
    )
    left.cpu.mem_write(transport, words(api))
    left.cpu.mem_write(api, words(send | 1, discover | 1, 0, 0, 0))
    for side in (left, right):
        side.cpu.mem_write(side.symbols["active_transport"], words(transport))
    connected = True

    def available(pointer, *_):
        left.cpu.mem_write(pointer, b"\x00")
        return int(connected)

    deliveries = []

    def deliver(source, a, b, c):
        assert source == 0
        args_on_stack = bytes(left.cpu.mem_read(left.cpu.reg_read(UC_ARM_REG_SP), 48))
        right.cpu.mem_write(right.ram + 0x7FF00, args_on_stack)
        assert (
            right.call(
                "zmk_split_transport_peripheral_command_handler", transport, a, b, c
            )
            == 0
        )
        right.call("apply_lighting_runtime", 0)
        packet = words(a, b, c) + args_on_stack
        deliveries.append((packet[9], packet[10]))
        return 0

    for address, callback in ((send, deliver), (discover, available)):
        left.cpu.mem_write(address, b"\x70\x47")
        left.stub(address, callback)
    # Stable layer 1 occupies index 0 after a keymap reorder.
    current_layer = 0
    left.stub(left.symbols["zmk_keymap_highest_layer_active"], lambda *_: 0)
    left.stub(left.symbols["zmk_keymap_layer_index_to_id"], lambda *_: current_layer)
    assert not left.call("zmk_led_matrix_set_profile", 251)
    left.call("sync_lighting_work", 0)
    assert deliveries[-1] == (0, 251)
    assert (pixel(left), pixel(right)) == (0x336699, 0x996633)
    connected = False
    current_layer = 1
    left.call("sync_lighting_work", 0)
    assert len(deliveries) == 1 and pixel(right) == 0x996633
    connected = True
    left.call("sync_lighting_work", 0)
    assert deliveries[-1] == (1, 251) and pixel(right) == 0x778899
    current_layer = 0
    left.call("sync_lighting_work", 0)
    assert pixel(right) == 0x996633
    print(
        "PASS split preset ID, independent colors, offline layer change, reconnect replay and overlay removal"
    )


if __name__ == "__main__":
    check(*sys.argv[1:])
