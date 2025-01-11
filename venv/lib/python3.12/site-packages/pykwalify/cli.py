# -*- coding: utf-8 -*-

""" pyKwalify - cli.py """

# python std lib
import logging
import logging.config
import sys

# 3rd party imports
from docopt import docopt


def parse_cli():
    """
    The outline of this function needs to be like this:

    1. parse arguments
    2. validate arguments only, dont go into other logic/code
    3. run application logic
    """

    #
    # 1. parse cli arguments
    #

    __docopt__ = """
usage: pykwalify -d FILE -s FILE ... [-e FILE ...]
       [--strict-rule-validation] [--fix-ruby-style-regex] [--allow-assertions] [--encoding ENCODING]
       [-v ...] [-q]

optional arguments:
  -d FILE, --data-file FILE            the file to be tested
  -e FILE, --extension FILE            file containing python extension
  -s FILE, --schema-file FILE          schema definition file
  --fix-ruby-style-regex               This flag fixes some of the quirks of ruby style regex
                                       that is not compatible with python style regex
  --strict-rule-validation             enables strict validation of all keywords for all
                                       Rule objects to find unsupported keyword usage
  --allow-assertions                   By default assertions is disabled due to security risk.
                                       Error will be raised if assertion is used in schema
                                       but this flag is not used. This option enables assert keyword.
  --encoding ENCODING                  Specify encoding to open data and schema files with.
  -h, --help                           show this help message and exit
  -q, --quiet                          suppress terminal output
  -v, --verbose                        verbose terminal output (multiple -v increases verbosity)
  --version                            display the version number and exit
"""

    # Import pykwalify package
    import pykwalify

    args = docopt(__docopt__, version=pykwalify.__version__)

    pykwalify.init_logging(1 if args["--quiet"] else args["--verbose"])
    log = logging.getLogger(__name__)

    #
    # 2. validate arguments only, dont go into other code/logic
    #

    log.debug("Setting verbose level: %s", args["--verbose"])
    log.debug("Arguments from CLI: %s", args)

    return args


def run(cli_args):
    """
    Split the functionality into 2 methods.

    One for parsing the cli and one that runs the application.
    """
    from .core import Core

    c = Core(
        source_file=cli_args["--data-file"],
        schema_files=cli_args["--schema-file"],
        extensions=cli_args['--extension'],
        strict_rule_validation=cli_args['--strict-rule-validation'],
        fix_ruby_style_regex=cli_args['--fix-ruby-style-regex'],
        allow_assertions=cli_args['--allow-assertions'],
        file_encoding=cli_args['--encoding'],
    )
    c.validate()
    return c


def cli_entrypoint():
    """
    Main entrypoint for script. Used by setup.py to automatically
    create a cli script
    """
    # Check minimum version of Python
    if sys.version_info < (2, 7, 0):
        sys.stderr.write(u"WARNING: pykwalify: It is recommended to run pykwalify on python version 2.7.x or later...\n\n")

    run(parse_cli())
