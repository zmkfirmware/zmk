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

import ctypes
import math


BITS_PER_BYTE = 8


def pack_size(value):
    """Returns the number of bytes required to represent a given value.

    Args:
      value (int): the natural number whose size to get

    Returns:
      The minimal number of bytes required to represent the given integer.

    Raises:
      ValueError: if ``value < 0``.
      TypeError: if ``value`` is not a number.
    """
    if value == 0:
        return 1
    elif value < 0:
        raise ValueError('Expected non-negative integer.')
    return int(math.log(value, 256)) + 1


def pack(value, nbits=None):
    """Packs a given value into an array of 8-bit unsigned integers.

    If ``nbits`` is not present, calculates the minimal number of bits required
    to represent the given ``value``.  The result is little endian.

    Args:
      value (int): the integer value to pack
      nbits (int): optional number of bits to use to represent the value

    Returns:
      An array of ``ctypes.c_uint8`` representing the packed ``value``.

    Raises:
      ValueError: if ``value < 0`` and ``nbits`` is ``None`` or ``nbits <= 0``.
      TypeError: if ``nbits`` or ``value`` are not numbers.
    """
    if nbits is None:
        nbits = pack_size(value) * BITS_PER_BYTE
    elif nbits <= 0:
        raise ValueError('Given number of bits must be greater than 0.')

    buf_size = int(math.ceil(nbits / float(BITS_PER_BYTE)))
    buf = (ctypes.c_uint8 * buf_size)()

    for (idx, _) in enumerate(buf):
        buf[idx] = (value >> (idx * BITS_PER_BYTE)) & 0xFF

    return buf
