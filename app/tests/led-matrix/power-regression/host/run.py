#!/usr/bin/env python3
"""Compile production C against deterministic bus/GPIO/workqueue fakes (macOS or Linux).

This complements, and does not replace, the native_sim integration suite. Drivers are
included verbatim. Runtime functions are extracted verbatim from the amalgamated LED
Matrix implementation so tests need not fake the unrelated renderer/Devicetree graph.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parents[3]


def function(source, name):
    # These functions contain no braces in string literals. Require exact markers
    # so refactoring/removal fails the test build instead of silently skipping tests.
    matches = list(re.finditer(
        r"(?m)^static [^;{}]*?\b" + re.escape(name) +
        r"\([^;{}]*\)\s*\{", source))
    match = matches[-1] if matches else None
    if match is None:
        raise ValueError(f"Definition not found: {name}")
    start = match.start()
    start = source.rfind("\n", 0, start) + 1
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


with tempfile.TemporaryDirectory(prefix="led-power-tests-") as tmp:
    root = Path(tmp)
    for header in ["device.h", "drivers/gpio.h", "drivers/i2c.h", "drivers/led.h",
                   "drivers/led/is31fl3733.h", "kernel.h", "logging/log.h", "sys/util.h"]:
        path = root / "zephyr" / header
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "driver_fakes.h"\n')
    composite = (APP / "module/drivers/led/led_composite.c").read_text()
    (root / "composite.inc").write_text(
        function(composite, "zmk_led_composite_shutdown_locked") +
        function(composite, "zmk_led_composite_power_locked"))
    render = (APP / "src/led_matrix/render.c").read_text()
    power = (APP / "src/led_matrix/power.c").read_text()
    status = (APP / "src/led_matrix/status.c").read_text()
    (root / "service.inc").write_text(render[render.index("static void led_matrix_queue_service_work(uint32_t requests) {"):])
    (root / "sleep.inc").write_text(function(power, "led_matrix_suspend_runtime_output_sync"))
    (root / "shutdown.inc").write_text(function(power, "led_matrix_clear_and_disable_runtime_output"))
    (root / "active_power.inc").write_text(function(power, "led_matrix_sync_active_runtime_output"))
    (root / "usb.inc").write_text(function(status, "led_matrix_handle_usb_status_event"))
    (root / "position.inc").write_text(function(status, "trigger_ripple_for_position") +
        function(status, "led_matrix_handle_position_event"))
    core = (APP / "src/led_matrix/core.c").read_text()
    (root / "usb_policy.inc").write_text(function(core, "led_matrix_usb_powered") +
        function(power, "led_matrix_auto_suspend") + function(power, "led_matrix_auto_resume_if_needed"))
    (root / "demand_math.inc").write_text(core[
        core.index("static struct led_rgb8 scale_color("):
        core.index("#if LED_DRIVER_IS_MATRIX\nstatic uint8_t grayscale_value")])
    (root / "demand_core.inc").write_text("\n".join(function(core, name) for name in (
        "led_matrix_color_is_dynamic", "led_matrix_color_is_time_dynamic", "resolve_dynamic_color",
        "led_matrix_configured_color", "led_matrix_color_can_emit", "led_matrix_color_needs_time",
        "resolve_zone_color", "set_zone_color_from_config", "resolve_profile_color",
        "resolve_independent_profile_color", "resolve_battery_on_color",
        "led_matrix_usb_color_cfg_for_state", "led_matrix_ble_color_cfg_for_snapshot",
        "render_hid_indicator", "led_matrix_reset_rain_state", "rain_sync_state", "grayscale_value")))
    (root / "demand_render.inc").write_text(render[
        render.index("static void render_profile_solid("):
        render.index("static void led_matrix_invalidate_flushed_frame(")])
    (root / "demand_compose.inc").write_text(function(render, "compose_render_frame"))
    for suite in ("driver", "service", "usb", "demand"):
        binary = root / suite
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                        "-Werror", "-Wno-unused-function", "-Wno-unused-variable",
                        "-fsanitize=address,undefined", "-g", "-I", str(root),
                        "-I", str(HERE), "-I", str(APP / "include"),
                        "-I", str(APP), str(HERE / f"{suite}_test.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

from run_composite import run as run_composite
from run_sinks import run as run_sinks
from run_ext_power import run as run_ext_power
from run_api import run as run_api
run_composite()
run_sinks()
run_ext_power()
run_api()
