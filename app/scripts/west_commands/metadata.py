# Copyright (c) 2021 The ZMK Contributors
# SPDX-License-Identifier: MIT
'''Metadata command for ZMK.'''

from functools import cached_property
import glob
import json
from jsonschema import validate, ValidationError
import os
import sys
import yaml
from textwrap import dedent            # just for nicer code indentation

from west.commands import WestCommand
from west import log                   # use this for user output


class Metadata(WestCommand):
    def __init__(self):
        super().__init__(
            'metadata',  # gets stored as self.name
            'ZMK hardware metadata commands',  # self.help
            # self.description:
            dedent('''Operate on the board/shield metadata.'''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name,
                                         help=self.help,
                                         description=self.description)

        parser.add_argument('subcommand', default="check",
                            help='The subcommand to run. Defaults to "check".', nargs="?")
        return parser           # gets stored as self.parser

    @cached_property
    def schema(self):
        return json.load(
            open("../schema/hardware-metadata.schema.json", 'r'))

    def validate_file(self, file):
        print("Validating: " + file)
        with open(file, 'r') as stream:
            try:
                validate(yaml.safe_load(stream), self.schema)
            except yaml.YAMLError as exc:
                print("Failed loading metadata yaml: " + file)
                print(exc)
                return False
            except ValidationError as vexc:
                print("Failed validation of: " + file)
                print(vexc)
                return False
        return True

    def do_run(self, args, unknown_args):
        status = all([self.validate_file(f) for f in glob.glob(
            "boards/**/*.zmk.yml", recursive=True)])

        sys.exit(0 if status else 1)
