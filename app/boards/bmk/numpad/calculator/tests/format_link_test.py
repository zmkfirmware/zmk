# SPDX-License-Identifier: MIT
"""Exercise the actual LVGL formatter with Zephyr's libc-name aliases."""
from pathlib import Path
import os
import re
import subprocess
import tempfile


def check_format_link(here: Path):
    root = here.parents[5]
    formatter = root / "modules/lib/gui/lvgl/src/stdlib/builtin/lv_sprintf_builtin.c"
    if not formatter.exists():
        print("LVGL formatter regression skipped: initialize workspace modules to run it")
        return
    board = here.parent.parent
    assert "CONFIG_LV_USE_CUSTOM_SPRINTF=y" in (
        board / "numpad_nrf52840_zmk_defconfig"
    ).read_text()
    adapter = (root / "zephyr/modules/lvgl/include/lv_conf.h").read_text()
    assert re.search(r"#define\s+lv_snprintf\s+snprintf\b", adapter)
    assert re.search(r"#define\s+lv_vsnprintf\s+vsnprintf\b", adapter)
    with tempfile.TemporaryDirectory(prefix="numpad-formatter-test-") as temp:
        out = Path(temp)
        config = out / "lv_conf.h"
        config.write_text("""
#define LV_CONF_H
#define LV_USE_STDLIB_SPRINTF TEST_FORMATTER
#define LV_USE_FLOAT 0
#define lv_snprintf snprintf
#define lv_vsnprintf vsnprintf
""")
        probe = out / "probe.c"
        probe.write_text('''
#include "engine.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    struct calc_engine c;
    calc_clear(&c);
    const char *p = "2+3*4";
    while (*p) calc_input(&c, *p++);
    if (!calc_evaluate(&c) || strcmp(c.result, "g")) { puts(c.result); return 1; }
    puts("Reproduced old LVGL builtin bug: 2+3*4 -> g");
    return 0;
}
''')
        flags = [os.environ.get("CC", "cc"), "-std=c11", "-fno-builtin", "-D_FORTIFY_SOURCE=0",
                 "-I" + str(here.parent), '-DLV_CONF_PATH="' + str(config) + '"']
        def build_run(mode, test, name):
            exe = out / name
            subprocess.run([*flags, "-DTEST_FORMATTER=" + mode,
                            str(here.parent / "engine.c"), str(formatter), str(test),
                            "-o", str(exe)], check=True)
            subprocess.run([str(exe)], check=True)
        build_run("LV_STDLIB_BUILTIN", probe, "reproduce")
        build_run("LV_STDLIB_CUSTOM", here / "engine_test.c", "regression")
        print("LVGL CUSTOM / libc formatter regression passed")
