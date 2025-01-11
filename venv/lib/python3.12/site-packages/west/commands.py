# Copyright 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io Limited.
# Copyright 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from abc import ABC, abstractmethod
import argparse
from collections import OrderedDict
from dataclasses import dataclass
from enum import IntEnum
import importlib.util
import itertools
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from types import ModuleType
from typing import Callable, Dict, List, NoReturn, Optional

import colorama
import pykwalify
import yaml

from west.configuration import Configuration
from west.manifest import Manifest, Project
from west.util import escapes_directory, quote_sh_list, PathType

'''\
This package provides WestCommand, which is the common abstraction all
west commands subclass.

This package also provides support for extension commands.'''

__all__ = ['CommandContextError', 'CommandError', 'WestCommand']

_EXT_SCHEMA_PATH = os.path.join(os.path.dirname(__file__),
                                'west-commands-schema.yml')

# Cache which maps files implementing extension commands to their
# imported modules.
_EXT_MODULES_CACHE: Dict[str, ModuleType] = {}
# Infinite iterator of "fresh" extension command module names.
_EXT_MODULES_NAME_IT = (f'west.commands.ext.cmd_{i}'
                        for i in itertools.count(1))

class CommandError(RuntimeError):
    '''Indicates that a command failed.'''

    def __init__(self, returncode=1):
        super().__init__()
        self.returncode = returncode

class CommandContextError(CommandError):
    '''Indicates that a context-dependent command could not be run.'''

class ExtensionCommandError(CommandError):
    '''Exception class indicating an extension command was badly
    defined and could not be created.'''

    def __init__(self, **kwargs):
        self.hint = kwargs.pop('hint', None)
        super(ExtensionCommandError, self).__init__(**kwargs)

def _no_topdir_msg(cwd, name):
    return f'''\
no west workspace found from "{cwd}"; "west {name}" requires one.
Things to try:
  - Change directory to somewhere inside a west workspace and retry.
  - Set ZEPHYR_BASE to a zephyr repository path in a west workspace.
  - Run "west init" to set up a workspace here.
  - Run "west init -h" for additional information.
'''

class Verbosity(IntEnum):
    '''Verbosity levels for WestCommand instances.'''

    # DO NOT CHANGE THESE VALUES WITHOUT UPDATING main.py!

    #: No output is printed when WestCommand.dbg(), .inf(), etc.
    #: are called.
    QUIET = 0

    #: Only error messages are printed.
    ERR = 1

    #: Only error and warnings are printed.
    WRN = 2

    #: Errors, warnings, and informational messages are printed.
    INF = 3

    #: Like INFO, but WestCommand.dbg(..., level=Verbosity.DBG) output
    #: is also printed.
    DBG = 4

    #: Like DEBUG, but WestCommand.dbg(..., level=Verbosity.DBG_MORE)
    #: output is also printed.
    DBG_MORE = 5

    #: Like DEBUG_MORE, but WestCommand.dbg(..., level=Verbosity.DBG_EXTREME)
    #: output is also printed.
    DBG_EXTREME = 6

#: Color used (when applicable) for printing with inf()
INF_COLOR = colorama.Fore.LIGHTGREEN_EX

#: Color used (when applicable) for printing with wrn()
WRN_COLOR = colorama.Fore.LIGHTYELLOW_EX

#: Color used (when applicable) for printing with err() and die()
ERR_COLOR = colorama.Fore.LIGHTRED_EX

class WestCommand(ABC):
    '''Abstract superclass for a west command.'''

    def __init__(self, name: str, help: str, description: str,
                 accepts_unknown_args: bool = False,
                 requires_workspace: bool = True,
                 verbosity: Verbosity = Verbosity.INF):
        '''Abstract superclass for a west command.

        Some fields, such as *name*, *help*, and *description*,
        overlap with kwargs that should be passed to the
        ``argparse.ArgumentParser`` added by `WestCommand.add_parser`.
        This wart is by design: ``argparse`` doesn't make many API stability
        guarantees, so this information must be duplicated here for
        future-proofing.

        :param name: the command's name, as entered by the user
        :param help: one-line command help text
        :param description: multi-line command description
        :param accepts_unknown_args: if true, the command can handle
            arbitrary unknown command line arguments in `WestCommand.run`.
            Otherwise, it's a fatal to pass unknown arguments.
        :param requires_workspace: if true, the command requires a
            west workspace to run, and running it outside of one is
            a fatal error.
        :param verbosity: command output verbosity level; can be changed later
        '''
        self.name: str = name
        self.help: str = help
        self.description: str = description
        self.accepts_unknown_args: bool = accepts_unknown_args
        self.requires_workspace = requires_workspace
        self.verbosity = verbosity
        self.topdir: Optional[str] = None
        self.manifest = None
        self.config = None
        self._hooks: List[Callable[['WestCommand'], None]] = []

    def add_pre_run_hook(self,
                         hook: Callable[['WestCommand'], None]) -> None:
        '''Add a hook which will be called right before do_run().

        This can be useful to defer work that needs a fully set up
        command to work.

        :param hook: hook to add
        '''
        self._hooks.append(hook)

    def run(self, args: argparse.Namespace, unknown: List[str],
            topdir: PathType,
            manifest: Optional[Manifest] = None,
            config: Optional[Configuration] = None) -> None:
        '''Run the command.

        This raises `west.commands.CommandContextError` if the command
        cannot be run due to a context mismatch. Other exceptions may
        be raised as well.

        :param args: known arguments parsed via `WestCommand.add_parser`
        :param unknown: unknown arguments present on the command line;
            must be empty unless ``accepts_unknown_args`` is true
        :param topdir: west workspace topdir, accessible as a str via
            ``self.topdir`` from `WestCommand.do_run`
        :param manifest: `west.manifest.Manifest` or ``None``,
            accessible as ``self.manifest`` from `WestCommand.do_run`
        :param config: `west.configuration.Configuration` or ``None``,
            accessible as ``self.config`` from `WestCommand.do_run`
        '''
        self.config = config
        if unknown and not self.accepts_unknown_args:
            self.parser.error(f'unexpected arguments: {unknown}')
        if not topdir and self.requires_workspace:
            self.die(_no_topdir_msg(os.getcwd(), self.name))
        self.topdir = os.fspath(topdir) if topdir else None
        self.manifest = manifest
        for hook in self._hooks:
            hook(self)
        self.do_run(args, unknown)

    def add_parser(self, parser_adder) -> argparse.ArgumentParser:
        '''Registers a parser for this command, and returns it.

        The parser object is stored in a ``parser`` attribute.

        :param parser_adder: The return value of a call to
            ``argparse.ArgumentParser.add_subparsers()``
        '''
        parser = self.do_add_parser(parser_adder)

        if parser is None:
            raise ValueError('do_add_parser did not return a value')

        self.parser = parser
        return self.parser

    #
    # Mandatory subclass hooks
    #

    @abstractmethod
    def do_add_parser(self, parser_adder) -> argparse.ArgumentParser:
        '''Subclass method for registering command line arguments.

        This is called by `WestCommand.add_parser` to register the
        command's options and arguments.

        Subclasses should ``parser_adder.add_parser()`` to add an
        ``ArgumentParser`` for that subcommand, then add any
        arguments. The final parser must be returned.

        :param parser_adder: The return value of a call to
            ``argparse.ArgumentParser.add_subparsers()``
        '''

    @abstractmethod
    def do_run(self, args: argparse.Namespace, unknown: List[str]):
        '''Subclasses must implement; called to run the command.

        :param args: ``argparse.Namespace`` of parsed arguments
        :param unknown: If ``accepts_unknown_args`` is true, a
            sequence of un-parsed argument strings.
        '''

    #
    # Public API, mostly for subclasses.
    #
    # These are meant to be useful to subclasses during their do_run()
    # calls. Using this functionality outside of a WestCommand
    # subclass leads to undefined results.
    #

    @property
    def has_manifest(self) -> bool:
        '''Property which is True if self.manifest is safe to access.
        '''
        return self._manifest is not None

    def _get_manifest(self) -> Manifest:
        '''Property for the manifest which was passed to run().

        If `do_run` was given a *manifest* kwarg, it is returned.
        Otherwise, a fatal error occurs.
        '''
        if self._manifest is None:
            self.die(f"can't run west {self.name};",
                     "it requires the manifest, which was not available.",
                     'Try "west -vv manifest --validate" to debug.')
        return self._manifest

    def _set_manifest(self, manifest: Optional[Manifest]):
        self._manifest = manifest

    # Do not use @property decorator syntax to avoid a false positive
    # error from mypy by using this workaround:
    # https://github.com/python/mypy/issues/3004#issuecomment-726022329
    manifest = property(_get_manifest, _set_manifest)

    @property
    def has_config(self) -> bool:
        '''Property which is True if self.config is safe to access.
        '''
        return self._config is not None

    def _get_config(self) -> Configuration:
        '''Property for the config which was passed to run().

        If `do_run` was given a *config* kwarg, it is returned.
        Otherwise, a fatal error occurs.
        '''
        if self._config is None:
            self.die(f"can't run west {self.name}; it requires config "
                     "variables, which were not available.")
        return self._config

    def _set_config(self, config: Optional[Configuration]):
        self._config = config

    config = property(_get_config, _set_config)

    def _log_subproc(self, args, **kwargs):
        self.dbg(f"running '{quote_sh_list(args)}' in "
                 f"{kwargs.get('cwd') or os.getcwd()}",
                 level=Verbosity.DBG_MORE)

    #
    # Other public methods
    #

    def check_call(self, args, **kwargs):
        '''Runs ``subprocess.check_call(args, **kwargs)`` after
        logging the call at Verbosity.DBG_MORE level.'''

        self._log_subproc(args, **kwargs)
        subprocess.check_call(args, **kwargs)

    def check_output(self, args, **kwargs):
        '''Runs ``subprocess.check_output(args, **kwargs)`` after
        logging the call at Verbosity.DBG_MORE level.'''

        self._log_subproc(args, **kwargs)
        return subprocess.check_output(args, **kwargs)

    def run_subprocess(self, args, **kwargs):
        '''Runs ``subprocess.run(args, **kwargs)`` after logging
        the call at Verbosity.DBG_MORE level.'''

        self._log_subproc(args, **kwargs)
        return subprocess.run(args, errors='backslashreplace', **kwargs)

    def die_if_no_git(self):
        '''Abort if git is not installed on PATH.
        '''
        if not hasattr(self, '_git'):
            self._git = shutil.which('git')
        if self._git is None:
            self.die("can't find git; install it or ensure it's on your PATH")

    @property
    def git_version_info(self):
        '''Returns git version info as a tuple of ints, usually in
        (major, minor, patch) format, like (2, 29, 1) for git version
        2.29.1.

        Aborts the program if there is no git installed.

        In rare circumstances, you may get a (major, minor) tuple,
        like (2, 29).
        '''
        # It's perfectly safe to compare 2-tuples against 3-tuples.
        # For example, '(2, 29) > (2, 28, 0)' is True.
        # https://docs.python.org/3/reference/expressions.html#comparisons

        if not hasattr(self, '_git_ver'):
            self.die_if_no_git()
            raw_version = self.check_output([self._git, '--version'])
            self._git_ver = self._parse_git_version(raw_version)
            if self._git_ver is None:
                self.die(f"can't get git version from {raw_version!r}")
            self.dbg(f'git version: {self._git_ver}', level=Verbosity.DBG_MORE)
        return self._git_ver

    @staticmethod
    def _parse_git_version(raw_version):
        # Convert the raw 'git --version' output to a tuple.
        #
        # This is a @staticmethod so it can be white box tested.
        #
        # Usually the resulting tuple looks like (major, minor,
        # patch).
        #
        # We get a length 2 tuple in obscure situations like git
        # built from a development source tree created using 'git
        # archive'. (See GIT-VERSION-GEN in the git sources if you're
        # curious about details.)
        #
        # Downstream distributors sometimes tweak the results by
        # adding to the end of 'x.y.z' in the 'git version x.y.z'
        # string, but git itself always prints "git version %s", where
        # the %s is the version.
        #
        # https://github.com/git/git/blob/7e391989789db82983665667013a46eabc6fc570/help.c#L646
        #
        # Some example possibilities:
        #
        # git version 2.25.1
        # git version 2.28.0.windows.1
        # git version 2.24.3 (Apple Git-128)
        # git version 2.29.GIT
        #
        # We handle this by matching the first bit in the
        # whitespace-separated output that has a prefix that looks
        # like a semver.

        match = re.search(
            r'\s(?P<major>\d+)\.(?P<minor>\d+)(\.(?P<patch>\d+))?',
            raw_version.decode(), flags=re.ASCII)
        if not match:
            return None

        major, minor, patch = (match.group('major'), match.group('minor'),
                               match.group('patch'))
        version = int(major), int(minor)
        if patch is None:
            return version
        return version + (int(patch),)

    def dbg(self, *args, level: Verbosity = Verbosity.DBG, end: str = '\n'):
        '''Print a verbose debug message.

        The message is only printed if *self.verbosity* is at least *level*.

        :param args: sequence of arguments to print
        :param level: verbosity level of the message
        '''
        if self.verbosity < level:
            return
        print(*args, end=end)

    def inf(self, *args, colorize: bool = False, end: str = '\n'):
        '''Print an informational message.

        The message is only printed if *self.verbosity* is at least INF.

        :param args: sequence of arguments to print.
        :param colorize: If this is True, the configuration option ``color.ui``
                         is undefined or true, and stdout is a terminal, then
                         the message is printed in green.
        '''
        if self.verbosity < Verbosity.INF:
            return

        if not self.color_ui:
            colorize = False

        # This approach colorizes any sep= and end= text too, as expected.
        #
        # colorama automatically strips the ANSI escapes when stdout isn't a
        # terminal (by wrapping sys.stdout).
        if colorize:
            print(INF_COLOR, end='')

        print(*args, end=end)

        if colorize:
            self._reset_colors(sys.stdout)

    def banner(self, *args):
        '''Prints args as a "banner" using inf().

        The args are prefixed with '=== ' and colorized by default.'''
        self.inf('===', *args, colorize=True)

    def small_banner(self, *args):
        '''Prints args as a smaller banner(), i.e. prefixed with '-- ' and
        not colorized.'''
        self.inf('---', *args, colorize=False)

    def wrn(self, *args, end: str = '\n'):
        '''Print a warning.

        The message is only printed if *self.verbosity* is at least WRN.

        The message is prefixed with the string ``"WARNING: "``.

        If the configuration option ``color.ui`` is undefined or true and
        stdout is a terminal, then the message is printed in yellow.

        :param args: sequence of arguments to print.'''

        if self.verbosity < Verbosity.WRN:
            return

        if self.color_ui:
            print(WRN_COLOR, end='', file=sys.stderr)

        print('WARNING: ', end='', file=sys.stderr)
        print(*args, end=end, file=sys.stderr)

        if self.color_ui:
            self._reset_colors(sys.stderr)

    def err(self, *args, fatal: bool = False, end: str = '\n'):
        '''Print an error.

        The message is only printed if *self.verbosity* is at least ERR.

        This function does not abort the program. For that, use `die()`.

        If the configuration option ``color.ui`` is undefined or true and
        stdout is a terminal, then the message is printed in red.

        :param args: sequence of arguments to print.
        :param fatal: if True, the the message is prefixed with
                      "FATAL ERROR: "; otherwise, "ERROR: " is used.
        '''

        if self.verbosity < Verbosity.ERR:
            return

        if self.color_ui:
            print(ERR_COLOR, end='', file=sys.stderr)

        print('FATAL ERROR: ' if fatal else 'ERROR: ', end='', file=sys.stderr)
        print(*args, end=end, file=sys.stderr)

        if self.color_ui:
            self._reset_colors(sys.stderr)

    def die(self, *args, exit_code: int = 1) -> NoReturn:
        '''Print a fatal error using err(), and abort the program.

        :param args: sequence of arguments to print.
        :param exit_code: return code the program should use when aborting.

        Equivalent to ``die(*args, fatal=True)``, followed by an attempt to
        abort with the given *exit_code*.'''
        self.err(*args, fatal=True)
        if self.verbosity >= Verbosity.DBG_EXTREME:
            raise RuntimeError("die with -vvv or more shows a stack trace. "
                               "exit_code argument is ignored.")
        else:
            sys.exit(exit_code)

    @property
    def color_ui(self) -> bool:
        '''Should we colorize output?'''
        return self.config.getboolean('color.ui', default=True)

    #
    # Internal APIs. Not for public consumption.
    #

    def _reset_colors(self, file):
        # The flush=True avoids issues with unrelated output from
        # commands (usually Git) becoming colorized, due to the final
        # attribute reset ANSI escape getting line-buffered
        print(colorama.Style.RESET_ALL, end='', file=file, flush=True)


#
# Private extension API
#
# This is used internally by main.py but should be considered an
# implementation detail.
#

@dataclass
class _ExtFactory:

    py_file: str
    name: str
    attr: str

    def __call__(self):
        # Append the python file's directory to sys.path. This lets
        # its code import helper modules in a natural way.
        py_dir = os.path.dirname(self.py_file)
        sys.path.append(py_dir)

        # Load the module containing the command. Convert only
        # expected exceptions to ExtensionCommandError.
        try:
            mod = _commands_module_from_file(self.py_file)
        except ImportError as ie:
            raise ExtensionCommandError(
                hint=f'could not import {self.py_file}') from ie

        # Get the attribute which provides the WestCommand subclass.
        try:
            cls = getattr(mod, self.attr)
        except AttributeError as ae:
            raise ExtensionCommandError(
                hint=f'no attribute {self.attr} in {self.py_file}') from ae

        # Create the command instance and return it.
        try:
            return cls()
        except Exception as e:
            raise ExtensionCommandError(
                hint='command constructor threw an exception') from e

@dataclass
class WestExtCommandSpec:
    # An object which allows instantiating a west extension.

    # Command name, as known to the user
    name: str

    # Project instance which defined the command
    project: Project

    # Help string in west-commands.yml, or a default value
    help: str

    # "Factory" callable for the command.
    #
    # This returns a WestCommand instance when called.
    # It may do some additional steps (like importing the definition of
    # the command) before constructing it, however.
    factory: _ExtFactory

def extension_commands(config: Configuration,
                       manifest: Optional[Manifest] = None):
    # Get descriptions of available extension commands.
    #
    # The return value is an ordered map from project paths to lists of
    # WestExtCommandSpec objects, for projects which define extension
    # commands. The map's iteration order matches the manifest.projects
    # order.
    #
    # The return value is empty if configuration option
    # ``commands.allow_extensions`` is false.
    #
    # :param manifest: a parsed ``west.manifest.Manifest`` object, or None
    #                  to reload a new one.

    allow_extensions = config.getboolean('commands.allow_extensions',
                                         default=True)
    if not allow_extensions:
        return {}

    if manifest is None:
        manifest = Manifest.from_file()

    specs = OrderedDict()
    for project in manifest.projects:
        if project.west_commands:
            specs[project.path] = _ext_specs(project)
    return specs

def _ext_specs(project):
    # Get a list of WestExtCommandSpec objects for the given
    # west.manifest.Project.

    ret = []

    for cmd in project.west_commands:
        spec_file = os.path.join(project.abspath, cmd)

        # Verify project.west_commands isn't trying a directory traversal
        # outside of the project.
        if escapes_directory(spec_file, project.abspath):
            raise ExtensionCommandError(
                hint=f'west-commands file {cmd} '
                f'escapes project path {project.path}')

        # The project may not be cloned yet, or this might be coming
        # from a manifest that was copy/pasted into a self import
        # location.
        if not os.path.exists(spec_file):
            continue

        # Load the spec file and check the schema.
        with open(spec_file, 'r') as f:
            try:
                commands_spec = yaml.safe_load(f.read())
            except yaml.YAMLError as e:
                raise ExtensionCommandError from e
        try:
            pykwalify.core.Core(
                source_data=commands_spec,
                schema_files=[_EXT_SCHEMA_PATH]).validate()
        except pykwalify.errors.SchemaError as e:
            raise ExtensionCommandError from e

        for commands_desc in commands_spec['west-commands']:
            ret.extend(_ext_specs_from_desc(project, commands_desc))
    return ret

def _ext_specs_from_desc(project, commands_desc):
    py_file = os.path.join(project.abspath, commands_desc['file'])

    # Verify the YAML's python file doesn't escape the project directory.
    if escapes_directory(py_file, project.abspath):
        raise ExtensionCommandError(
            hint=f'extension command python file "{commands_desc["file"]}" '
            f'escapes project path {project.path}')

    # Create the command thunks.
    thunks = []
    for command_desc in commands_desc['commands']:
        name = command_desc['name']
        attr = command_desc.get('class', name)
        help = command_desc.get('help',
                                f'(no help provided; try "west {name} -h")')
        factory = _ExtFactory(py_file, name, attr)
        thunks.append(WestExtCommandSpec(name, project, help, factory))

    # Return the thunks for this project.
    return thunks

def _commands_module_from_file(file):
    # Python magic for importing a module containing west extension
    # commands. To avoid polluting the sys.modules key space, we put
    # these modules in an (otherwise unpopulated) west.commands.ext
    # package.
    #
    # The file is imported as a module named
    # west.commands.ext.A_FRESH_IDENTIFIER. This module object is
    # returned from a cache if the same file is ever imported again,
    # to avoid a double import in case the file maintains module-level
    # state or defines multiple commands.
    global _EXT_MODULES_CACHE
    global _EXT_MODULES_NAME_IT

    # Use an absolute pathobj to handle canonicalization, e.g.:
    #
    # - Windows and macOS have case insensitive names
    # - Windows accepts slash or backslash as separator
    # - POSIX operating systems have symlinks
    pathobj = Path(file).resolve()
    if pathobj in _EXT_MODULES_CACHE:
        return _EXT_MODULES_CACHE[pathobj]

    mod_name = next(_EXT_MODULES_NAME_IT)
    spec = importlib.util.spec_from_file_location(mod_name, os.fspath(pathobj))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    _EXT_MODULES_CACHE[file] = mod

    return mod
