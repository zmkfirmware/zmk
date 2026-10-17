#!/usr/bin/env python3
"""Exercise the real brightness/on/off API, suspend entry, output queue and save scheduler."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parents[3]


def function(source, name):
    match = re.search(r"(?m)^(?:static )?(?:inline )?(?:int|bool|void|uint8_t) " +
                      re.escape(name) + r"\([^;{]*\)\s*\{", source)
    if not match:
        raise ValueError(f"Definition not found: {name}")
    end = source.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


def run():
    api = (APP / "src/led_matrix/api.c").read_text()
    power = (APP / "src/led_matrix/power.c").read_text()
    render = (APP / "src/led_matrix/render.c").read_text()
    source = function(render, "led_matrix_queue_service_work")
    source += "\n".join(function(power, name) for name in
                         ("led_matrix_clear_runtime_suspend_state",
                          "led_matrix_suspend_runtime_output_async", "led_matrix_save_state"))
    source += "\n".join(function(api, name) for name in
                         ("zmk_led_matrix_on", "zmk_led_matrix_off", "zmk_led_matrix_is_on",
                          "zmk_led_matrix_set_brt", "zmk_led_matrix_get_brt"))
    with tempfile.TemporaryDirectory(prefix="led-api-tests-") as tmp:
        root = Path(tmp)
        (root / "api.inc").write_text(source)
        binary = root / "api"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-g", "-I", str(root), "-I", str(HERE),
                        str(HERE / "api_test.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    run()
