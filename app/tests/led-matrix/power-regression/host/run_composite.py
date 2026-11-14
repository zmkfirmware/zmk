#!/usr/bin/env python3
"""Compile the complete production composite driver against deterministic child devices."""
from pathlib import Path
import os
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parents[3]


def run():
    with tempfile.TemporaryDirectory(prefix="led-composite-tests-") as tmp:
        root = Path(tmp)
        for header in ("zephyr/device.h", "zephyr/drivers/gpio.h", "zephyr/drivers/i2c.h",
                       "zephyr/drivers/led.h", "zephyr/drivers/led_strip.h", "zephyr/kernel.h",
                       "zephyr/logging/log.h", "zephyr/sys/util.h", "zephyr/sys/util_macro.h",
                       "drivers/ext_power.h", "drivers/led/led_mock.h"):
            path = root / header
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('#include "composite_test_fakes.h"\n')
        binary = root / "composite"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unused-function", "-Wno-unused-variable", "-fsanitize=address,undefined",
                        "-g", "-I", str(root), "-I", str(HERE), "-I", str(APP / "include"),
                        "-I", str(APP), str(HERE / "composite_test.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    run()
