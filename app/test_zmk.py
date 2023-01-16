#!/usr/bin/env python3

# Copyright (c) 2023 The ZMK Contributors
# SPDX-License-Identifier: MIT


from io import StringIO
from pathlib import Path
import subprocess
from subprocess import CalledProcessError, PIPE
from typing import Sequence, Union

from PythonSed import Sed
import pytest


APP_PATH = Path(__file__).parent
TESTS_PATH = APP_PATH / "tests"
BUILD_PATH = APP_PATH / "build/tests"

# TODO: change all tests to build with a unit test shield
TEST_KEYMAP_NAME = "native_posix_64.keymap"


def check_call_teed(args: Sequence[Union[str, Path]], **kwargs):
    """
    Call a subprocess, print its output, and also return its output.
    """
    with subprocess.Popen(
        args, bufsize=1, stdout=PIPE, text=True, encoding="utf-8", **kwargs
    ) as p:
        stdout = StringIO()

        for line in p.stdout:
            stdout.write(line)
            print(line, end="")

        returncode = p.wait()
        output = stdout.getvalue()

        if returncode:
            raise CalledProcessError(returncode, p.args, output)

        return output


def strip_log_tags(log: str):
    def strip(line: str):
        left, _, right = line.partition("> ")
        return right or left

    return "\n".join(strip(line) for line in log.splitlines())


def filter_log(log: str, patterns_file: Path):
    sed = Sed(encoding="utf-8")
    sed.no_autoprint = True
    sed.load_script(str(patterns_file))

    input = StringIO(log)
    output = StringIO()
    sed.apply(input, output)

    return output.getvalue()


class Runner:
    def __init__(self, path: Path, build_path: Path):
        self.path = path
        self.build_path = build_path

    @property
    def board(self) -> str:
        """The Zephyr board to build"""
        raise NotImplementedError()

    @classmethod
    def check(cls):
        """Check that the environment is valid for the runner."""
        pass

    def run(self) -> str:
        """Run the unit test and return its output."""
        raise NotImplementedError()


class PosixRunner(Runner):
    @property
    def board(self):
        return "native_posix_64"

    def run(self):
        return check_call_teed([self.build_path / "zephyr/zmk.exe"], cwd=APP_PATH)


# TODO: Add QemuRunner for non-Posix platforms


class ZmkTestCase:
    def __init__(self, path: Path) -> None:
        self.path = path
        self.runner = self._get_runner()
        self.runner.check()

        self.patterns_file = self.path / "events.patterns"
        self.snapshot_file = self.path / "keycode_events.snapshot"

        if not self.patterns_file.exists():
            pytest.fail(f"Missing patterns file: {self.patterns_file}")

        if not self.snapshot_file.exists():
            pytest.fail(f"Missing snapshot file: {self.snapshot_file}")

    def run(self):
        if reason := self.get_pending_reason():
            pytest.skip(reason)

        self._build()
        self._test()

    @property
    def build_path(self):
        return BUILD_PATH / self.path.relative_to(TESTS_PATH)

    @property
    def relative_build_path(self):
        return self.build_path.relative_to(APP_PATH)

    def get_pending_reason(self):
        for file in ["pending", f"pending-{self.runner.board}"]:
            try:
                return (self.path / file).read_text(encoding="utf-8")
            except:
                pass

        return None

    def _build(self):
        subprocess.check_call(
            [
                "west",
                "build",
                "-d",
                self.build_path.relative_to(APP_PATH).as_posix(),
                "-b",
                self.runner.board,
                "--",
                f"-DZMK_CONFIG={self.path.as_posix()}",
            ],
            cwd=APP_PATH,
        )

    def _test(self):
        output = self.runner.run()
        output = strip_log_tags(output)
        with self._open_log("keycode_events_full.log") as log:
            log.write(output)

        output = filter_log(output, self.patterns_file)
        with self._open_log("keycode_events.log") as log:
            log.write(output)

        assert output == self.snapshot_file.read_text(encoding="utf-8")

    def _get_runner(self) -> type[Runner]:
        # TODO: return a QemuRunner for non-Posix platforms
        return PosixRunner(self.path, self.build_path)

    def _open_log(self, name: str, mode="w"):
        log_path = self.build_path / name
        log_path.parent.mkdir(parents=True, exist_ok=True)

        return log_path.open(mode, encoding="utf-8")


def get_tests():
    paths = sorted(keymap.parent for keymap in TESTS_PATH.rglob(TEST_KEYMAP_NAME))

    return [
        pytest.param(path, id=str(path.relative_to(TESTS_PATH).as_posix()))
        for path in paths
    ]


@pytest.mark.parametrize("name", get_tests())
def test(name):
    ZmkTestCase(TESTS_PATH / name).run()
