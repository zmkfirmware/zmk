#!/usr/bin/env python3
"""Exercise the complete generic external-power driver with multi-GPIO failures."""
from pathlib import Path
import os
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parents[3]


def run():
    with tempfile.TemporaryDirectory(prefix="led-ext-power-tests-") as tmp:
        root = Path(tmp)
        for header in ("zephyr/device.h", "zephyr/pm/device.h", "zephyr/init.h",
                       "zephyr/kernel.h", "zephyr/settings/settings.h", "zephyr/drivers/gpio.h",
                       "zephyr/logging/log.h", "drivers/ext_power.h"):
            path = root / header
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('#include "ext_power_test_fakes.h"\n')
        binary = root / "ext-power"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unused-function", "-Wno-unused-variable", "-fsanitize=address,undefined",
                        "-g", "-I", str(root), "-I", str(HERE), "-I", str(APP / "include"),
                        "-I", str(APP), str(HERE / "ext_power_test.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    run()
