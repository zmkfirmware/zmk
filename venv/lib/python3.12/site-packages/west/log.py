# Copyright 2018 Open Source Foundries Limited.
# Copyright 2019 Foundries.io Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Deprecated due to its use of global state, which makes
west harder to unit test. See:

  https://github.com/zephyrproject-rtos/west/issues/149

Removal won't come until west v2.0.

In the future, commands should use equivalent WestCommand methods
instead. For example, use WestCommand.dbg() instead of west.log.dbg(),
and so forth.

Provides common methods for printing messages to display to the user
which respect the ``color.ui`` configuration option and verbosity
level. These were formerly encouraged for WestCommand instances.
'''

import sys
from typing import NoReturn
import warnings

from west import configuration as config

import colorama

VERBOSE_NONE = 0
'''Default verbosity level, no dbg() messages printed.'''

VERBOSE_NORMAL = 1
'''Some verbose messages printed.'''

VERBOSE_VERY = 2
'''Very verbose output messages will be printed.'''

VERBOSE_EXTREME = 3
'''Extremely verbose output messages will be printed.'''

VERBOSE = VERBOSE_NONE
'''Global verbosity level. VERBOSE_NONE is the default.'''

#: Color used (when applicable) for printing with inf()
INF_COLOR = colorama.Fore.LIGHTGREEN_EX

#: Color used (when applicable) for printing with wrn()
WRN_COLOR = colorama.Fore.LIGHTYELLOW_EX

#: Color used (when applicable) for printing with err() and die()
ERR_COLOR = colorama.Fore.LIGHTRED_EX

def deprecated():
    warnings.warn('The west.log API is deprecated; '
                  'use an equivalent west.commands.WestCommand API routine',
                  DeprecationWarning, stacklevel=3)

def set_verbosity(value):
    '''Set the logging verbosity level.

    :param value: verbosity level to set, e.g. VERBOSE_VERY.
    '''
    global VERBOSE
    VERBOSE = int(value)

def dbg(*args, level=VERBOSE_NORMAL):
    '''Print a verbose debug logging message.

    :param args: sequence of arguments to print.
    :param level: verbosity level to set, e.g. VERBOSE_VERY.

    The message is only printed if the ``level`` parameter is at most
    the current verbosity level.
    '''
    deprecated()
    if level > VERBOSE:
        return
    print(*args)

def inf(*args, colorize=False):
    '''Print an informational message.

    :param args: sequence of arguments to print.
    :param colorize: If this is True, the configuration option ``color.ui``
                     is undefined or true, and stdout is a terminal, then
                     the message is printed in green.
    '''
    deprecated()
    if VERBOSE < 0:
        return

    if not _use_colors():
        colorize = False

    # This approach colorizes any sep= and end= text too, as expected.
    #
    # colorama automatically strips the ANSI escapes when stdout isn't a
    # terminal (by wrapping sys.stdout).
    if colorize:
        print(INF_COLOR, end='')

    print(*args)

    if colorize:
        _reset_colors(sys.stdout)

def banner(*args):
    '''Prints args as a "banner" at inf() level.

    The args are prefixed with '=== ' and colorized by default.'''
    deprecated()
    inf('===', *args, colorize=True)

def small_banner(*args):
    '''Prints args as a smaller banner(), i.e. prefixed with '-- ' and
    not colorized.'''
    deprecated()
    inf('---', *args, colorize=False)

def wrn(*args):
    '''Print a warning.

    :param args: sequence of arguments to print.

    The message is prefixed with the string ``"WARNING: "``.

    If the configuration option ``color.ui`` is undefined or true and
    stdout is a terminal, then the message is printed in yellow.'''
    deprecated()
    if VERBOSE < -1:
        return

    if _use_colors():
        print(WRN_COLOR, end='', file=sys.stderr)

    print('WARNING: ', end='', file=sys.stderr)
    print(*args, file=sys.stderr)

    if _use_colors():
        _reset_colors(sys.stderr)

def err(*args, fatal=False):
    '''Print an error.

    This function does not abort the program. For that, use `die()`.

    :param args: sequence of arguments to print.
    :param fatal: if True, the the message is prefixed with "FATAL ERROR: ";
                  otherwise, "ERROR: " is used.

    If the configuration option ``color.ui`` is undefined or true and
    stdout is a terminal, then the message is printed in red.'''
    deprecated()

    if _use_colors():
        print(ERR_COLOR, end='', file=sys.stderr)

    print('FATAL ERROR: ' if fatal else 'ERROR: ', end='', file=sys.stderr)
    print(*args, file=sys.stderr)

    if _use_colors():
        _reset_colors(sys.stderr)

def die(*args, exit_code=1) -> NoReturn:
    '''Print a fatal error, and abort the program.

    :param args: sequence of arguments to print.
    :param exit_code: return code the program should use when aborting.

    Equivalent to ``die(*args, fatal=True)``, followed by an attempt to
    abort with the given *exit_code*.'''
    deprecated()
    err(*args, fatal=True)
    sys.exit(exit_code)

def msg(*args, color=None, stream=sys.stdout):
    '''Print a message using a color.

    :param args: sequence of arguments to print.
    :param color: color to print in (e.g. INF_COLOR), must be given
    :param stream: file to print to (default is stdout)

    If color.ui is disabled, the message will still be printed, but
    without color.
    '''
    deprecated()
    if color is None:
        raise ValueError('no color was given')

    if _use_colors():
        print(color, end='', file=stream)

    print(*args, file=stream)

    if _use_colors():
        _reset_colors(stream)

def use_color():
    '''Returns True if the configuration requests colored output.'''
    deprecated()
    return _use_colors(warn=False)

_COLOR_UI_WARNED = False

def _use_colors(warn=True):
    # Convenience function for reading the color.ui setting
    try:
        return config.config.getboolean('color', 'ui', fallback=True)
    except ValueError as e:
        if warn:
            global _COLOR_UI_WARNED
            if not _COLOR_UI_WARNED:
                print(f"WARNING: invalid color.ui value: {e}.",
                      file=sys.stderr)
                print('         To fix: "west config color.ui <true|false>"',
                      file=sys.stderr)
                _COLOR_UI_WARNED = True
        return False

def _reset_colors(file):
    # The flush=True avoids issues with unrelated output from commands (usually
    # Git) becoming colorized, due to the final attribute reset ANSI escape
    # getting line-buffered
    print(colorama.Style.RESET_ALL, end='', file=file, flush=True)
