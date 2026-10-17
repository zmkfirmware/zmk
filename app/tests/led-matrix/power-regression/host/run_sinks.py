#!/usr/bin/env python3
"""Run production RGB/scalar sink functions against per-LED fault injection."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
APP = HERE.parents[3]


def function(source, name):
    match = re.search(r"(?m)^static (?:inline )?[^\n{;]+\b" + re.escape(name) +
                      r"\([^;{]*\)\s*\{", source)
    if not match:
        raise ValueError(f"Definition not found: {name}")
    end = source.index("{", match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


def run():
    core = (APP / "src/led_matrix/core.c").read_text()
    render = (APP / "src/led_matrix/render.c").read_text()
    source = "\n".join(function(core, name) for name in
                       ("scale_color", "grayscale_value", "write_scalar_led_value"))
    source += "\n".join(function(render, name) for name in
                         ("flush_matrix_rgb_sink", "flush_matrix_scalar_led_sink",
                          "flush_matrix_scalar_triplet_sink"))
    with tempfile.TemporaryDirectory(prefix="led-sink-tests-") as tmp:
        root = Path(tmp)
        (root / "sinks.inc").write_text(source)
        binary = root / "sinks"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-g", "-I", str(root), "-I", str(HERE),
                        str(HERE / "sinks_test.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    run()
