# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT
"""Test runner for ZMK."""

import pytest
from west.commands import WestCommand
from west import log  # use this for user output


class Test(WestCommand):
    def __init__(self):
        super().__init__(
            name="test",
            help="run ZMK testsuite",
            description="Run the ZMK testsuite. Arguments are passed through to pytest.",
            accepts_unknown_args=True,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
        )

        parser.add_argument(
            "test",
            help="The path to the test to run. Runs all tests if not specified.",
            nargs="?",
        )
        return parser

    def do_run(self, args, unknown_args):
        pytest_args = [f"{self.topdir}/app/test_zmk.py", "--numprocesses=auto"]
        pytest_args += unknown_args

        if args.test:
            pytest_args += ["-k", args.test]

        returncode = pytest.main(pytest_args)
        exit(returncode)
