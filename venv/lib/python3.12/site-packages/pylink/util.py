# -*- coding: utf-8 -*-
# Copyright 2017 Square, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from . import enums

import platform
import sys


def is_integer(val):
    """Returns whether the given value is an integer.

    Args:
      val (object): value to check

    Returns:
      ``True`` if the given value is an integer, otherwise ``False``.
    """
    try:
        val += 1
    except TypeError:
        return False
    return True


def is_natural(val):
    """Returns whether the given value is a natrual number.

    Args:
      val (object): value to check

    Returns:
      ``True`` if the given value is a natural number, otherwise ``False``.
    """
    return is_integer(val) and (val >= 0)


def is_os_64bit():
    """Returns whether the current running platform is 64bit.

    Returns:
      ``True`` if the platform is 64bit, otherwise ``False``.
    """
    return platform.machine().endswith('64')


def noop(*args, **kwargs):
    """No-op.  Does nothing.

    Args:
      args: list of arguments
      kwargs: keyword arguments dictionary

    Returns:
      ``None``
    """
    pass


def unsecure_hook_dialog(title, msg, flags):
    """No-op that ignores the dialog.

    Args:
      title (str): title of the unsecure dialog
      msg (str): text of the unsecure dialog
      flags (int): flags specifying which values can be returned

    Returns:
      ``enums.JLinkFlags.DLG_BUTTON_NO``
    """
    return enums.JLinkFlags.DLG_BUTTON_NO


def progress_bar(iteration,
                 total,
                 prefix=None,
                 suffix=None,
                 decs=1,
                 length=100):
    """Creates a console progress bar.

    This should be called in a loop to create a progress bar.

    See `StackOverflow <http://stackoverflow.com/questions/3173320/>`__.

    Args:
      iteration (int): current iteration
      total (int): total iterations
      prefix (str): prefix string
      suffix (str): suffix string
      decs (int): positive number of decimals in percent complete
      length (int): character length of the bar

    Returns:
      ``None``

    Note:
      This function assumes that nothing else is printed to the console in the
      interim.
    """
    if prefix is None:
        prefix = ''

    if suffix is None:
        suffix = ''

    format_str = '{0:.' + str(decs) + 'f}'
    percents = format_str.format(100 * (iteration / float(total)))
    filled_length = int(round(length * iteration / float(total)))
    bar = '█' * filled_length + '-' * (length - filled_length)

    prefix, suffix = prefix.strip(), suffix.strip()

    sys.stdout.write('\r%s |%s| %s%s %s' % (prefix, bar, percents, '%', suffix))
    sys.stdout.flush()

    if iteration == total:
        sys.stdout.write('\n')
        sys.stdout.flush()

    return None


def flash_progress_callback(action, progress_string, percentage):
    """Callback that can be used with ``JLink.flash()``.

    This callback generates a progress bar in the console to show the progress
    of each of the steps of the flash.

    Args:
      action (str): the current action being invoked
      progress_string (str): the current step in the progress
      percentage (int): the percent to which the current step has been done

    Returns:
      ``None``

    Note:
      This function ignores the compare action.
    """
    if action.lower() != 'compare':
        return progress_bar(min(100, percentage), 100, prefix=action)

    return None


def calculate_parity(n):
    """Calculates and returns the parity of a number.

    The parity of a number is ``1`` if the number has an odd number of ones
    in its binary representation, otherwise ``0``.

    Args:
      n (int): the number whose parity to calculate

    Returns:
      ``1`` if the number has an odd number of ones, otherwise ``0``.

    Raises:
      ValueError: if ``n`` is less than ``0``.
    """
    if not is_natural(n):
        raise ValueError('Expected n to be a positive integer.')

    y = 0
    n = abs(n)
    while n:
        y += n & 1
        n = n >> 1
    return y & 1
