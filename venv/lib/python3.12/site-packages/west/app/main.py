#!/usr/bin/env python3

# Copyright 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io Limited.
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Zephyr RTOS meta-tool (west) main module

Nothing in here is public API.
'''

import argparse
from collections import OrderedDict
import colorama
from io import StringIO
import logging
import os
from pathlib import Path, PurePath
import platform
import shlex
import shutil
import signal
import sys
from subprocess import CalledProcessError
import tempfile
import textwrap
import traceback
from typing import NamedTuple, Optional
from typing import List as ListType

from west import log
import west.configuration
from west.commands import WestCommand, extension_commands, \
    CommandError, ExtensionCommandError, Verbosity
from west.app.project import List, ManifestCommand, Compare, Diff, Status, \
    SelfUpdate, ForAll, Grep, Init, Update, Topdir
from west.app.config import Config
from west.manifest import Manifest, MalformedConfig, MalformedManifest, \
    ManifestVersionError, ManifestImportFailed, _ManifestImportDepth, \
    ManifestProject, MANIFEST_REV_BRANCH
from west.util import quote_sh_list, west_topdir, WestNotFound
from west.version import __version__

class EarlyArgs(NamedTuple):
    # Data type for storing "early" argument parsing results.
    #
    # We do some manual parsing of the command-line arguments before
    # delegating the hard parts to argparse. This extra work figures
    # out the command name, verbosity level, etc.
    #
    # This is necessary for:
    #
    # - nicer error-handling in situations where the command is an
    #   extension but we're not in a workspace
    #
    # - setting up log levels from the verbosity level

    # Expected arguments:
    help: bool                  # True if -h was given
    version: bool               # True if -V was given
    zephyr_base: Optional[str]  # -z argument value
    verbosity: int              # 0 if not given, otherwise counts
    command_name: Optional[str]

    # Other arguments are appended here.
    unexpected_arguments: ListType[str]

def parse_early_args(argv: ListType[str]) -> EarlyArgs:
    # Hand-rolled argument parser for early arguments.

    help = False
    version = False
    zephyr_base = None
    verbosity = 0
    command_name = None
    unexpected_arguments = []

    expecting_zephyr_base = False

    def consume_more_args(rest):
        # Handle the 'Vv' portion of 'west -hVv'.

        nonlocal help, version, zephyr_base, verbosity, command_name
        nonlocal unexpected_arguments
        nonlocal expecting_zephyr_base

        if not rest:
            return

        if rest.startswith('h'):
            help = True
            consume_more_args(rest[1:])
        elif rest.startswith('V'):
            version = True
            consume_more_args(rest[1:])
        elif rest.startswith('v'):
            verbosity += 1
            consume_more_args(rest[1:])
        elif rest.startswith('q'):
            verbosity -= 1
            consume_more_args(rest[1:])
        elif rest.startswith('z'):
            if not rest[1:]:
                expecting_zephyr_base = True
            elif rest[1] == '=':
                zephyr_base = rest[2:]
            else:
                zephyr_base = rest[1:]
        else:
            unexpected_arguments.append(rest)

    for arg in argv:
        if expecting_zephyr_base:
            zephyr_base = arg
        elif arg.startswith('-h'):
            help = True
            consume_more_args(arg[2:])
        elif arg.startswith('-V'):
            version = True
            consume_more_args(arg[2:])
        elif arg == '--version':
            version = True
        elif arg.startswith('-v'):
            verbosity += 1
            consume_more_args(arg[2:])
        elif arg.startswith('-q'):
            verbosity -= 1
            consume_more_args(arg[2:])
        elif arg == '--verbose':
            verbosity += 1
        elif arg == '--quiet':
            verbosity -= 1
        elif arg.startswith('-z'):
            if arg == '-z':
                expecting_zephyr_base = True
            elif arg.startswith('-z='):
                zephyr_base = arg[3:]
            else:
                zephyr_base = arg[2:]
        elif arg.startswith('-'):
            unexpected_arguments.append(arg)
        else:
            command_name = arg
            break

    return EarlyArgs(help, version, zephyr_base, verbosity,
                     command_name, unexpected_arguments)

class LogFormatter(logging.Formatter):

    def __init__(self):
        super().__init__(fmt='%(name)s: %(levelname)s: %(message)s')

class LogHandler(logging.Handler):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.setFormatter(LogFormatter())

    def emit(self, record):
        formatted = self.format(record)
        if record.levelno >= logging.WARNING:
            print(formatted, file=sys.stderr)
        else:
            print(formatted)

class WestApp:
    # The west 'application' object.
    #
    # There's enough state to keep track of when building the final
    # WestCommand we want to run that it's convenient to have an
    # object to stash it all in.
    #
    # We could use globals, but that would make it harder to white-box
    # test multiple main() invocations from the same Python process,
    # which is a goal. See #149.

    def __init__(self):
        self.topdir = None          # west_topdir()
        self.config = None          # west.configuration.Configuration
        self.manifest = None        # west.manifest.Manifest
        self.mle = None             # saved exception if load_manifest() fails
        self.builtins = {}          # command name -> WestCommand instance
        self.extensions = {}        # extension command name -> spec
        self.aliases = {}           # alias -> WestCommand instance
        self.builtin_groups = OrderedDict()    # group name -> WestCommand list
        self.extension_groups = OrderedDict()  # project path -> ext spec list
        self.west_parser = None     # a WestArgumentParser
        self.subparser_gen = None   # an add_subparsers() return value
        self.cmd = None             # west.commands.WestCommand, eventually
        self.queued_io = []         # I/O hooks we want self.cmd to do

        for group, classes in BUILTIN_COMMAND_GROUPS.items():
            lst = [cls() for cls in classes]
            self.builtins.update({command.name: command for command in lst})
            self.builtin_groups[group] = lst

        # Give the help instance a back-pointer up here.
        #
        # A dirty layering violation, but it does need this data:
        #
        # - 'west help <command>' needs to call into <command>'s
        #   parser's print_help()
        # - 'west help' needs self.west_parser, which
        #   the argparse API does not give us a future-proof way
        #   to access from the Help object's parser attribute,
        #   which comes from subparser_gen.
        self.builtins['help'].app = self

    def run(self, argv):
        # Run the command-line application with argument list 'argv'.

        early_args = parse_early_args(argv)

        # Silence validation errors from pykwalify, which are logged at
        # logging.ERROR level. We want to handle those ourselves as
        # needed.
        logging.getLogger('pykwalify').setLevel(logging.CRITICAL)

        # Use verbosity to determine west API log levels
        self.setup_west_logging(early_args.verbosity)

        # Makes ANSI color escapes work on Windows, and strips them when
        # stdout/stderr isn't a terminal
        colorama.init()

        # See if we're in a workspace. It's fine if we're not.
        # Note that this falls back on searching from ZEPHYR_BASE
        # if the current directory isn't inside a west workspace.
        try:
            self.topdir = west_topdir()
        except WestNotFound:
            pass

        # Read the configuration files. We need this to get
        # manifest.path to parse the manifest, etc.
        self.config = west.configuration.Configuration(topdir=self.topdir)

        # Also set up the global configuration object to match, for
        # backwards compatibility.
        self.config._copy_to_configparser(west.configuration.config)

        # Set self.manifest and self.extensions.
        self.load_manifest()
        self.load_extension_specs()
        self.load_aliases()

        # Set up initial argument parsers. This requires knowing
        # self.extensions, so it can't happen before now.
        self.setup_parsers()

        # OK, we are all set. Run the command.
        self.run_command(argv, early_args)

    def load_manifest(self):
        # Try to parse the manifest. We'll save it if that works, so
        # it doesn't have to be re-parsed.

        if not self.topdir:
            return

        try:
            self.manifest = Manifest.from_topdir(topdir=self.topdir,
                                                 config=self.config)
        except (ManifestVersionError, MalformedManifest, MalformedConfig,
                ManifestImportFailed, FileNotFoundError, PermissionError) as e:
            # Defer exception handling to WestCommand.run(), which uses
            # handle_builtin_manifest_load_err() to decide what to do.
            #
            # Make sure to update that function if you change the
            # exceptions caught here. Unexpected exceptions should
            # propagate up and fail fast.
            #
            # This might be OK, e.g. if we're running 'west config
            # manifest.path foo' to fix the MalformedConfig error, but
            # there's no way to know until we've parsed the command
            # line arguments.
            self.mle = e

    def handle_builtin_manifest_load_err(self, args):
        # Deferred handling for expected load_manifest() exceptions.
        # Called before attempting to run a built-in command. (No
        # extension commands can be run, because we learn about them
        # from the manifest itself, which we have failed to load.)

        # A few commands are always safe to run without a manifest.
        # The update command is sometimes safe and sometimes not, but
        # we need to include it in this list because it's the only way
        # to fix a manifest-rev revision in a project which is being
        # imported to point from a bogus manifest to a non-bogus one.
        no_manifest_ok = ['help', 'config', 'topdir', 'init', 'manifest',
                          'update']

        # Handle ManifestVersionError is a special case.
        if isinstance(self.mle, ManifestVersionError):
            if args.command == 'help':
                self.queued_io.append(
                    lambda cmd:
                    cmd.wrn(mve_msg(self.mle, suggest_upgrade=False) +
                            '\n  Cannot get extension command help, ' +
                            "and most commands won't run." +
                            '\n  To silence this warning, upgrade west.'))
                return
            elif args.command in ['config', 'topdir']:
                # config and topdir are safe to run, but let's
                # warn the user that most other commands won't be.
                self.queued_io.append(
                    lambda cmd:
                    cmd.wrn(mve_msg(self.mle, suggest_upgrade=False) +
                            "\n  This should work, but most commands won't." +
                            '\n  To silence this warning, upgrade west.'))
                return
            elif args.command == 'init':
                # init is fine to run -- it will print its own error,
                # with context about where the workspace was found,
                # and what the user's choices are.
                return
            else:
                self.queued_io.append(lambda cmd: cmd.die(mve_msg(self.mle)))
                return

        # Other errors generally just fall back on no_manifest_ok.
        def isinst(*args):
            return any(isinstance(self.mle, t) for t in args)

        if args.command not in no_manifest_ok:
            if isinst(MalformedManifest, MalformedConfig):
                self.queued_io.append(
                    lambda cmd:
                    cmd.die("can't load west manifest: " +
                            "\n".join(list(self.mle.args))))
            elif isinst(_ManifestImportDepth):
                self.queued_io.append(
                    lambda cmd:
                    cmd.die(
                        'recursion depth exceeded during manifest resolution; '
                        'your manifest likely contains an import loop. '
                        'Run "west -v manifest --resolve" to debug.'))
            elif isinst(ManifestImportFailed):
                if args.command == 'update':
                    return      # that's fine

                self.queued_io.append(lambda cmd: cmd.die(mie_msg(self.mle)))
            elif isinst(FileNotFoundError):
                # This should ordinarily only happen when the top
                # level manifest is not found.
                self.queued_io.append(
                    lambda cmd:
                    cmd.die(f"manifest file not found: {self.mle.filename}\n"
                            "Please check manifest.file and manifest.path in "
                            f"{self.topdir + '/' or ''}.west/config"))
            elif isinst(PermissionError):
                self.queued_io.append(
                    lambda cmd:
                    cmd.die("permission denied when loading manifest file: "
                            f"{self.mle.filename}"))
            else:
                self.queued_io.append(
                    lambda cmd:
                    cmd.die('internal error:',
                            f'unhandled manifest load exception: {self.mle}'))

    def load_extension_specs(self):
        if self.manifest is None:
            # "None" means "extensions could not be determined".
            # Leaving this an empty dict would mean "there are no
            # extensions", which is different.
            self.extensions = None
            return

        try:
            path_specs = extension_commands(self.config,
                                            manifest=self.manifest)
        except ExtensionCommandError as ece:
            self.handle_extension_command_error(ece)
        extension_names = set()

        for path, specs in path_specs.items():
            # Filter out attempts to shadow built-in commands as well as
            # command names which are already used.

            filtered = []
            for spec in specs:
                if spec.name in self.builtins:
                    self.queued_io.append(
                        lambda cmd: cmd.wrn(
                            f'ignoring project {spec.project.name} '
                            f'extension command "{spec.name}"; '
                            'this is a built in command'))
                    continue
                if spec.name in extension_names:
                    self.queued_io.append(
                        lambda cmd: cmd.wrn(
                            f'ignoring project {spec.project.name} '
                            f'extension command "{spec.name}"; '
                            f'command "{spec.name}" is '
                            'already defined as extension command'))
                    continue

                filtered.append(spec)
                extension_names.add(spec.name)
                self.extensions[spec.name] = spec

            self.extension_groups[path] = filtered

    def load_aliases(self):
        if not self.config:
            return

        self.aliases = {
            k[6:]: Alias(k[6:], v) for k, v in self.config.items() if k.startswith('alias.')
        }

    def handle_extension_command_error(self, ece):
        if self.cmd is not None:
            msg = f"extension command \"{self.cmd.name}\" couldn't be run"
        else:
            msg = "could not load extension command(s)"
        if ece.hint:
            msg += '\n  Hint: ' + ece.hint

        if self.cmd and self.cmd.verbosity >= Verbosity.DBG_EXTREME:
            self.cmd.err(msg, fatal=True)
            self.cmd.banner('Traceback (enabled by -vvv):')
            traceback.print_exc()
        else:
            tb_file = dump_traceback()
            msg += f'\n  See {tb_file} for a traceback.'
            self.cmd.err(msg, fatal=True)
        sys.exit(ece.returncode)

    def setup_parsers(self):
        # Set up and install command-line argument parsers.

        west_parser, subparser_gen = self.make_parsers()
        real_command_names = set()

        # Add sub-parsers for the built-in commands.
        for name, command in self.builtins.items():
            real_command_names.add(name)
            command.add_parser(subparser_gen)

        # Add stub parsers for extensions.
        #
        # These just reserve the names of each extension. The real parser
        # for each extension can't be added until we import the
        # extension's code, which we won't do unless parse_known_args()
        # says to run that extension.
        if self.extensions:
            for specs in self.extension_groups.values():
                for spec in specs:
                    real_command_names.add(spec.name)
                    subparser_gen.add_parser(spec.name, add_help=False)

        # Add aliases, but skip aliases that shadow other commands
        # The help parser requires unique commands to be added
        if self.aliases:
            for name, alias in self.aliases.items():
                # Advanced users shadowing real commands do not get "alias help"
                if name not in real_command_names:
                    alias.add_parser(subparser_gen)

        # Save the instance state.
        self.west_parser = west_parser
        self.subparser_gen = subparser_gen

    def make_parsers(self):
        # Make a fresh instance of the top level argument parser
        # and subparser generator, and return them in that order.

        # The prog='west' override avoids the absolute path of the
        # main.py script showing up when West is run via the wrapper
        parser = WestArgumentParser(
            prog='west', description='The Zephyr RTOS meta-tool.',
            epilog='''Run "west help <command>" for help on each <command>.''',
            add_help=False, west_app=self, allow_abbrev=False)

        # Remember to update zephyr's west-completion.bash if you add or
        # remove flags. This is currently the only place where shell
        # completion is available.
        #
        # If you update these, also update parse_early_args().

        parser.add_argument('-h', '--help', action=WestHelpAction, nargs=0,
                            help='get help for west or a command')

        parser.add_argument('-z', '--zephyr-base', default=None,
                            help='''Override the Zephyr base directory. The
                            default is the manifest project with path
                            "zephyr".''')

        parser.add_argument('-v', '--verbose', default=0, action='count',
                            help='''Display verbose output. May be given
                            multiple times to increase verbosity.''')

        parser.add_argument('-q', '--quiet', default=0, action='count',
                            help='''Display less verbose output. May be given
                            multiple times to decrease verbosity.''')

        parser.add_argument('-V', '--version', action='version',
                            version=f'West version: v{__version__}',
                            help='print the program version and exit')

        subparser_gen = parser.add_subparsers(metavar='<command>',
                                              dest='command')

        return parser, subparser_gen

    def run_command(self, argv, early_args):
        # Parse command line arguments and run the WestCommand.
        # If we're running an extension, instantiate it from its
        # spec and re-parse arguments before running.

        if not early_args.help and early_args.command_name != "help":
            # Recursively replace alias command(s) if set
            aliases = self.aliases.copy()
            while early_args.command_name in aliases:
                # Make sure we don't end up in an infinite loop
                alias = aliases.pop(early_args.command_name)

                self.queued_io.append(lambda cmd, alias=alias: cmd.dbg(
                    f'Replacing alias {alias.name} with {alias.args}'
                ))

                if len(alias.args) == 0:
                    # This loses the cmd.dbg() above - too bad, don't use empty aliases
                    self.print_usage_and_exit(f'west: empty alias "{alias.name}"')

                # Find and replace the command name. Must skip any other early args like -v
                for i, arg in enumerate(argv):
                    if arg == early_args.command_name:
                        argv = argv[:i] + alias.args + argv[i + 1:]
                        break
                early_args = early_args._replace(command_name=alias.args[0])

        self.handle_early_arg_errors(early_args)
        args, unknown = self.west_parser.parse_known_args(args=argv)

        # Set up logging verbosity before running the command, for
        # backwards compatibility. Remove this when we can part ways
        # with the log module.
        log.set_verbosity(args.verbose - args.quiet)

        # If we were run as 'west -h ...' or 'west --help ...',
        # monkeypatch the args namespace so we end up running Help.  The
        # user might have also provided a command. If so, print help about
        # that command.
        if args.help or args.command is None:
            args.command_name = args.command
            args.command = 'help'

        # Finally, run the command.
        try:
            # Both run_builtin() and run_extension() set self.cmd so
            # we can use it in the exception handling blocks below.
            if args.command in self.builtins:
                self.run_builtin(args, unknown)
            else:
                self.run_extension(args.command, argv)
        except KeyboardInterrupt:
            # Catching this avoids dumping stack.
            #
            # Here we replicate CPython's behavior in exit_sigint() in
            # Modules/main.c (as of
            # 2f62a5da949cd368a9498e6a03e700f4629fa97f), but in pure
            # Python since it's not clear how or if we can call that
            # directly from here.
            #
            # For more discussion on this behavior, see:
            #
            # https://bugs.python.org/issue1054041
            if platform.system() == 'Windows':
                # The hex number is a standard value (STATUS_CONTROL_C_EXIT):
                # https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-erref/596a1078-e883-4972-9bbc-49e60bebca55
                #
                # Subtracting 2**32 seems to be the convention for
                # making it fit in an int32_t.
                CONTROL_C_EXIT_CODE = 0xC000013A - 2**32
                sys.exit(CONTROL_C_EXIT_CODE)
            else:
                # On Unix, just reinstate the default SIGINT handler
                # and send the signal again, overriding the CPython
                # KeyboardInterrupt stack dump.
                #
                # In addition to exiting with the correct "dying due
                # to SIGINT" status code (usually 130), this signals
                # to the calling environment that we were interrupted,
                # so that e.g. "while true; do west ... ; done" will
                # exit out of the entire while loop and not just
                # the west command.
                signal.signal(signal.SIGINT, signal.SIG_DFL)
                os.kill(os.getpid(), signal.SIGINT)
        except BrokenPipeError:
            sys.exit(0)
        except CalledProcessError as cpe:
            self.cmd.err(f'command exited with status {cpe.returncode}: '
                         f'{quote_sh_list(cpe.cmd)}', fatal=True)
            if self.cmd.verbosity >= Verbosity.DBG_EXTREME:
                self.cmd.banner('Traceback (enabled by -vvv):')
                traceback.print_exc()
            sys.exit(cpe.returncode)
        except ExtensionCommandError as ece:
            self.handle_extension_command_error(ece)
        except CommandError as ce:
            # No need to dump_traceback() here. The command is responsible
            # for logging its own errors.
            sys.exit(ce.returncode)
        except MalformedManifest as mm:
            # We can get here because 'west update' is allowed to run
            # even when an invalid manifest was detected, as a way to
            # try to fix a previous update that left 'manifest-rev'
            # branches pointing at revisions with invalid manifest
            # data in projects that get imported.
            self.cmd.die('\n  '.join(str(arg) for arg in mm.args))
        except WestNotFound as wnf:
            self.cmd.die(str(wnf))

    def handle_early_arg_errors(self, early_args):
        # If early_args indicates we should error out, handle it
        # gracefully. This provides more user-friendly output than
        # argparse can do on its own.

        if (early_args.command_name and not early_args.help and
            (early_args.command_name not in self.builtins and
             (not self.extensions or
              early_args.command_name not in self.extensions))):
            self.handle_unknown_command(early_args.command_name)

    def handle_unknown_command(self, command_name):
        # "status" needs "-vv" to show git errors like "dubious ownership"; see #726
        if self.topdir:
            extra_help = (f'workspace {self.topdir} does not define '
                          'this extension command -- try "west help"'
                          ' and "west -vv status"')
        else:
            extra_help = 'do you need to run this inside a workspace?'
        self.print_usage_and_exit(f'west: unknown command "{command_name}"; '
                                  f'{extra_help}')

    def print_usage_and_exit(self, message):
        self.west_parser.print_usage(file=sys.stderr)
        sys.exit(message)

    def setup_west_logging(self, verbosity):
        logger = logging.getLogger('west.manifest')

        if verbosity >= 2:
            logger.setLevel(logging.DEBUG)
        elif verbosity == 1:
            logger.setLevel(logging.INFO)
        elif verbosity == 0:
            logger.setLevel(logging.WARNING)
        elif verbosity == -1:
            logger.setLevel(logging.ERROR)
        else:
            logger.setLevel(logging.CRITICAL)

        logger.addHandler(LogHandler())

    def run_builtin(self, args, unknown):
        self.queued_io.append(
            lambda cmd: cmd.dbg('args namespace:', args,
                                level=Verbosity.DBG_EXTREME))
        self.cmd = self.builtins.get(args.command,
                                     self.builtins['help'])
        adjust_command_verbosity(self.cmd, args)
        if self.mle:
            self.handle_builtin_manifest_load_err(args)
        for io_hook in self.queued_io:
            self.cmd.add_pre_run_hook(io_hook)
        self.cmd.run(args, unknown, self.topdir,
                     manifest=self.manifest,
                     config=self.config)

    def run_extension(self, name, argv):
        # Check a program invariant. We should never get here
        # unless we were able to parse the manifest. That's where
        # information about extensions is loaded from.
        assert self.manifest is not None and self.mle is None, \
            f'internal error: running extension "{name}" ' \
            f'but got {self.mle}'

        self.cmd = self.extensions[name].factory()

        # Our original top level parser and subparser generator have some
        # garbage state that prevents us from registering the 'real'
        # command subparser. Just make new ones.
        west_parser, subparser_gen = self.make_parsers()
        self.cmd.add_parser(subparser_gen)

        # Parse arguments again.
        args, unknown = west_parser.parse_known_args(argv)

        adjust_command_verbosity(self.cmd, args)
        self.queued_io.append(
            lambda cmd: cmd.dbg('args namespace:', args,
                                level=Verbosity.DBG_EXTREME))
        for io_hook in self.queued_io:
            self.cmd.add_pre_run_hook(io_hook)

        # HACK: try to set ZEPHYR_BASE.
        #
        # Currently required by zephyr extensions like "west build".
        #
        # TODO: get rid of this. Instead:
        #
        # - support a WEST_DIR environment variable to specify the
        #   workspace if we're not running under a .west directory
        #   (controversial)
        # - make zephyr extensions that need ZEPHYR_BASE just set it
        #   themselves (easy if above is OK, unnecessary if it isn't)
        self.set_zephyr_base(args)

        self.cmd.run(args, unknown, self.topdir, manifest=self.manifest,
                     config=self.config)

    def set_zephyr_base(self, args):
        '''Ensure ZEPHYR_BASE is set
        Order of precedence:
        1) Value given as command line argument
        2) Value from environment setting: ZEPHYR_BASE
        3) Value of zephyr.base setting in west config file
        4) Project in the manifest with name, or path, "zephyr" (will
           be persisted as zephyr.base in the local config if found)

        Order of precedence between 2) and 3) can be changed with the setting
        zephyr.base-prefer.
        zephyr.base-prefer takes the values 'env' and 'configfile'

        If 2) and 3) have different values and zephyr.base-prefer is unset,
        a warning is printed.'''
        manifest = self.manifest
        topdir = self.topdir
        config = self.config

        if args.zephyr_base:
            # The command line --zephyr-base takes precedence over
            # everything else.
            zb = os.path.abspath(args.zephyr_base)
            zb_origin = 'command line'
        else:
            # If the user doesn't specify it concretely, then use ZEPHYR_BASE
            # from the environment or zephyr.base from west.configuration.
            #
            # (We will configure zephyr.base to the project that has path
            # 'zephyr' as a last resort here.)
            #
            # At some point, we need a more flexible way to set environment
            # variables based on manifest contents, but this is good enough
            # to get started with and to ask for wider testing.
            zb_env = os.environ.get('ZEPHYR_BASE')
            zb_prefer = config.get('zephyr.base-prefer')
            rel_zb_config = config.get('zephyr.base')
            if rel_zb_config is None:
                # Try to find a project named 'zephyr', or with path
                # 'zephyr' inside the workspace.
                projects = None
                try:
                    projects = manifest.get_projects(['zephyr'],
                                                     allow_paths=False)
                except ValueError:
                    try:
                        projects = manifest.get_projects([Path(topdir) /
                                                          'zephyr'])
                    except ValueError:
                        pass
                if projects:
                    zephyr = projects[0]
                    config.set('zephyr.base', zephyr.path)
                    rel_zb_config = zephyr.path
            if rel_zb_config is not None:
                zb_config = Path(topdir) / rel_zb_config
            else:
                zb_config = None

            if zb_prefer == 'env' and zb_env is not None:
                zb = zb_env
                zb_origin = 'env'
            elif zb_prefer == 'configfile' and zb_config is not None:
                zb = str(zb_config)
                zb_origin = 'configfile'
            elif zb_env is not None:
                zb = zb_env
                zb_origin = 'env'
                try:
                    different = (zb_config and not zb_config.samefile(zb_env))
                except FileNotFoundError:
                    different = (zb_config and
                                 (PurePath(zb_config)) !=
                                 PurePath(zb_env))
                if different:
                    # The environment ZEPHYR_BASE takes precedence
                    # over the config setting, but is different than
                    # the zephyr.base config value.
                    #
                    # Therefore, issue a warning as the user might have
                    # run zephyr-env.sh/cmd in some other zephyr
                    # workspace and forgotten about it.
                    self.queued_io.append(
                        lambda cmd:
                        cmd.wrn(
                            f'ZEPHYR_BASE={zb_env} '
                            f'in the calling environment will be used,\n'
                            f'but the zephyr.base config option in {topdir} '
                            f'is "{rel_zb_config}"\n'
                            'which implies a different '
                            f'ZEPHYR_BASE={zb_config}\n'
                            f'To disable this warning in the future, execute '
                            f"'west config --global zephyr.base-prefer env'"))
            elif zb_config:
                zb = str(zb_config)
                zb_origin = 'configfile'
            else:
                zb = None
                zb_origin = None
                # No --zephyr-base, no ZEPHYR_BASE, and no zephyr.base.
                self.queued_io.append(
                    lambda cmd:
                    cmd.wrn(
                        "can't find the zephyr repository\n"
                        '  - no --zephyr-base given\n'
                        '  - ZEPHYR_BASE is unset\n'
                        '  - west config contains no zephyr.base setting\n'
                        '  - no manifest project has name or path "zephyr"\n'
                        '\n'
                        "  If this isn't a Zephyr workspace, you can "
                        "  silence this warning with something like this:\n"
                        '    west config zephyr.base not-using-zephyr'))

        if zb is not None:
            os.environ['ZEPHYR_BASE'] = zb
            self.queued_io.append(
                lambda cmd:
                cmd.dbg(f'ZEPHYR_BASE={zb} (origin: {zb_origin})'))

class Help(WestCommand):
    # west help <command> implementation.

    def __init__(self):
        super().__init__('help', 'get help for west or a command',
                         textwrap.dedent('''\
                         With an argument, prints help for that command.
                         Without one, prints top-level help for west.'''),
                         requires_workspace=False)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description,
            formatter_class=argparse.RawDescriptionHelpFormatter)
        parser.add_argument('command_name', nargs='?', default=None,
                            help='name of command to get help for')
        return parser

    def do_run(self, args, ignored):
        assert self.app, "Help has no WestApp and can't do its job"
        app = self.app
        name = args.command_name

        if not name:
            app.west_parser.print_help(top_level=True)
        elif name == 'help':
            self.parser.print_help()
        elif name in app.builtins:
            app.builtins[name].parser.print_help()
        elif app.extensions is not None and name in app.extensions:
            # It's fine that we don't handle any errors here. The
            # exception handling block in app.run_command is in a
            # parent stack frame.
            app.run_extension(name, [name, '--help'])
        elif app.aliases is not None and name in app.aliases:
            app.aliases[name].parser.print_help()
        else:
            self.wrn(f'unknown command "{name}"')
            app.west_parser.print_help(top_level=True)
            if app.mle:
                self.wrn('your manifest could not be loaded, '
                         'which may be causing this issue.\n'
                         '  Try running "west update" or fixing the manifest.')

class Alias(WestCommand):
    # An alias command, it does not run itself

    def __init__(self, cmd, args):
        super().__init__(cmd, args or '<empty>', f'An alias that expands to: {args}')

        self.args = shlex.split(args)

    # Pseudo-parser that will never actually run except for ".print_help()"
    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description, add_help=False,
            formatter_class=argparse.RawDescriptionHelpFormatter)

        return parser

    def do_run(self, args, ignored):
        assert False

class WestHelpAction(argparse.Action):

    def __call__(self, parser, namespace, values, option_string=None):
        # Just mark that help was requested.
        namespace.help = True

class WestArgumentParser(argparse.ArgumentParser):
    # The argparse module is infuriatingly coy about its parser and
    # help formatting APIs, marking almost everything you need to
    # customize help output an "implementation detail". Even accessing
    # the parser's description and epilog attributes as we do here is
    # technically breaking the rules.
    #
    # Even though the implementation details have been pretty stable
    # since the module was first introduced in Python 3.2, let's avoid
    # possible headaches by overriding some "proper" argparse APIs
    # here instead of monkey-patching the module or breaking
    # abstraction barriers. This is duplicative but more future-proof.

    def __init__(self, *args, **kwargs):
        # The super constructor calls add_argument(), so this has to
        # come first as our override of that method relies on it.
        self.west_optionals = []
        self.west_app = kwargs.pop('west_app', None)
        super(WestArgumentParser, self).__init__(*args, **kwargs)

    def print_help(self, file=None, top_level=False):
        print(self.format_help(top_level=top_level), end='',
              file=file or sys.stdout)

    def format_help(self, top_level=False):
        # When top_level is True, we override the parent method to
        # produce more readable output, which separates commands into
        # logical groups. In order to print optionals, we rely on the
        # data available in our add_argument() override below.
        #
        # If top_level is False, it's because we're being called from
        # one of the subcommand parsers, and we delegate to super.

        if not top_level:
            return super(WestArgumentParser, self).format_help()

        # Format the help to be at most 75 columns wide, the maximum
        # generally recommended by typographers for readability.
        #
        # If the terminal width (COLUMNS) is less than 75, use width
        # (COLUMNS - 2) instead, unless that is less than 30 columns
        # wide, which we treat as a hard minimum.
        width = min(75, max(shutil.get_terminal_size().columns - 2, 30))

        with StringIO() as sio:

            def append(*strings):
                for s in strings:
                    print(s, file=sio)

            append(self.format_usage(),
                   self.description,
                   '')

            append('optional arguments:')
            for wo in self.west_optionals:
                self.format_west_optional(append, wo, width)

            append('')
            for group, commands in self.west_app.builtin_groups.items():
                if group is None:
                    # Skip hidden commands.
                    continue

                append(group + ':')
                for command in commands:
                    self.format_command(append, command, width)
                append('')

            if self.west_app.extensions is None:
                if not self.west_app.mle:
                    # This only happens when there is an error.
                    # If there are simply no extensions, it's an empty dict.
                    # If the user has already been warned about the error
                    # because it's due to a ManifestVersionError, don't
                    # warn them again.
                    append('Cannot load extension commands; '
                           'help for them is not available.')
                    append('(To debug, try: "west -vv manifest --validate".)')
                    append('')
            else:
                # TODO we may want to be more aggressive about loading
                # command modules by default: the current implementation
                # prevents us from formatting one-line help here.
                #
                # Perhaps a commands.extension_paranoid that if set, uses
                # thunks, and otherwise just loads the modules and
                # provides help for each command.
                #
                # This has its own wrinkle: we can't let a failed
                # import break the built-in commands.
                for path, specs in self.west_app.extension_groups.items():
                    # This may occur in case a project defines commands already
                    # defined, in which case it has been filtered out.
                    if not specs:
                        continue

                    project = specs[0].project  # they're all from this project
                    append('extension commands from project '
                           f'{project.name} (path: {project.path}):')

                    for spec in specs:
                        self.format_extension_spec(append, spec, width)
                    append('')

            if self.west_app.aliases:
                append('aliases:')
                for alias in self.west_app.aliases.values():
                    self.format_command(append, alias, width)
                append('')

            if self.epilog:
                append(self.epilog)

            return sio.getvalue()

    def format_west_optional(self, append, wo, width):
        metavar = wo['metavar']
        options = wo['options']
        help = wo.get('help')

        # Join the various options together as a comma-separated list,
        # with the metavar if there is one. That's our "thing".
        if metavar is not None:
            opt_str = '  ' + ', '.join(f'{o} {metavar}' for o in options)
        else:
            opt_str = '  ' + ', '.join(options)

        # Delegate to the generic formatter.
        self.format_thing_and_help(append, opt_str, help, width)

    def format_command(self, append, command, width):
        thing = f'  {command.name}:'
        self.format_thing_and_help(append, thing, command.help, width)

    def format_extension_spec(self, append, spec, width):
        self.format_thing_and_help(append, '  ' + spec.name + ':',
                                   spec.help, width)

    def format_thing_and_help(self, append, thing, help, width):
        # Format help for some "thing" (arbitrary text) and its
        # corresponding help text an argparse-like way.
        help_offset = min(max(10, width - 20), 24)
        help_indent = ' ' * help_offset

        thinglen = len(thing)

        if help is None:
            # If there's no help string, just print the thing.
            append(thing)
        else:
            # Reflow the lines in help to the desired with, using
            # the help_offset as an initial indent.
            help = ' '.join(help.split())
            help_lines = textwrap.wrap(help, width=width,
                                       initial_indent=help_indent,
                                       subsequent_indent=help_indent)

            if thinglen > help_offset - 1:
                # If the "thing" (plus room for a space) is longer
                # than the initial help offset, print it on its own
                # line, followed by the help on subsequent lines.
                append(thing)
                append(*help_lines)
            else:
                # The "thing" is short enough that we can start
                # printing help on the same line without overflowing
                # the help offset, so combine the "thing" with the
                # first line of help.
                help_lines[0] = thing + help_lines[0][thinglen:]
                append(*help_lines)

    def add_argument(self, *args, **kwargs):
        # Track information we want for formatting help.  The argparse
        # module calls kwargs.pop(), so can't call super first without
        # losing data.
        optional = {'options': [], 'metavar': kwargs.get('metavar', None)}
        need_metavar = (optional['metavar'] is None and
                        kwargs.get('action') in (None, 'store'))
        for arg in args:
            if not arg.startswith('-'):
                break
            optional['options'].append(arg)
            # If no metavar was given, the last option name is
            # used. By convention, long options go last, so this
            # matches the default argparse behavior.
            if need_metavar:
                optional['metavar'] = arg.lstrip('-').translate(
                    {ord('-'): '_'}).upper()
        optional['help'] = kwargs.get('help')
        self.west_optionals.append(optional)

        # Let argparse handle the actual argument.
        super().add_argument(*args, **kwargs)

    def error(self, message):
        if self.west_app and self.west_app.mle:
            # If we have a known WestApp instance and the manifest
            # failed to load, then try to specialize the generic error
            # message we're getting from argparse to handle west-specific
            # errors better.

            app = self.west_app
            mle = self.west_app.mle
            if app.cmd:
                cmd = app.cmd
            else:
                # No app.cmd probably means that the user is
                # running an extension command that they expected
                # to work, but we don't know about because the
                # import failed and thus we have no manifest to
                # load extensions from.
                #
                # Just use the help command as a stand-in instead.
                # We have to manually patch up its config
                # attribute in order to do this outside of
                # do_run().
                cmd = app.builtins['help']
                cmd.config = west.configuration.Configuration(
                    topdir=app.topdir)
            if isinstance(mle, ManifestVersionError):
                cmd.die(mve_msg(mle))
            elif isinstance(mle, ManifestImportFailed):
                cmd.die(mie_msg(mle))
        super().error(message=message)

def mve_msg(mve, suggest_upgrade=True):
    # Helper for getting a message for a ManifestVersionError.
    return '\n  '.join(
        [f'west v{mve.version} or later is required by the manifest',
         f'West version: v{__version__}'] +
        ([f'Manifest file: {mve.file}'] if mve.file else []) +
        (['Please upgrade west and retry.'] if suggest_upgrade else []))

def mie_msg(mie):
    # Helper for getting a message for a ManifestImportError.

    p, imp = mie.project, mie.imp
    ret = (f'failed manifest import in {p.name_and_path}:\n'
           f'  Failed importing "{imp}"')
    if not isinstance(p, ManifestProject):
        # Try to be more helpful by explaining exactly
        # what west.manifest needs to happen before we can
        # resolve the missing import.
        ret += (f' from revision "{p.revision}"\n'
                f'  Hint: {p.name} must be cloned, owned by the user and its '
                f'{MANIFEST_REV_BRANCH} ref must point to a '
                'commit with the import data\n'
                '  To fix, run "west update. If it still fails, try "west -vv ..."')

    return ret

def adjust_command_verbosity(command, args):
    command.verbosity = max(
        min(command.verbosity + args.verbose - args.quiet, Verbosity.DBG_EXTREME),
        Verbosity.QUIET
    )

def dump_traceback():
    # Save the current exception to a file and return its path.
    fd, name = tempfile.mkstemp(prefix='west-exc-', suffix='.txt')
    os.close(fd)        # traceback has no use for the fd
    with open(name, 'w') as f:
        traceback.print_exc(file=f)
    return name

def main(argv=None):
    # Create the WestApp instance and let it run.
    app = WestApp()
    app.run(argv or sys.argv[1:])

# If you add a command here, make sure to think about how it should be
# handled in case of ManifestVersionError or other reason the manifest
# might fail to load (import error, configuration file error, etc.)
BUILTIN_COMMAND_GROUPS = {
    'built-in commands for managing git repositories': [
        Init,
        Update,
        List,
        ManifestCommand,
        Compare,
        Diff,
        Status,
        ForAll,
        Grep,
    ],

    'other built-in commands': [
        Help,
        Config,
        Topdir,
    ],

    # None is for hidden commands we don't want to show to the user.
    None: [SelfUpdate]
}

if __name__ == "__main__":
    main()
