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

from .. import util

import ctypes


class Response(object):
    """Response class to hold the response from the send of a SWD request."""
    STATUS_ACK = (1 << 0)
    STATUS_WAIT = (1 << 1)
    STATUS_FAULT = (1 << 2)
    STATUS_INVALID = -1

    def __init__(self, status, data=None):
        """Initializes the response.

        Args:
          self (Response): the ``Response`` instance
          status (int): the status the response exited with
          data (object): the optional data returned from the request

        Returns:
          ``None``
        """
        self.status = status
        self.data = data

    def ack(self):
        """Returns whether the response was ACK'd.

        Args:
          self (Response): the ``Response`` instance

        Returns:
          ``True`` if response was ACK'd, otherwise ``False``.
        """
        return (self.status == self.STATUS_ACK)

    def wait(self):
        """Returns whether the response was a wait.

        Args:
          self (Response): the ``Response`` instance

        Returns:
          ``True`` if response exited with wait, otherwise ``False``.
        """
        return (self.status == self.STATUS_WAIT)

    def fault(self):
        """Returns whether the response exited with fault.

        Args:
          self (Response): the ``Response`` instance

        Returns:
          ``True`` if response exited with a fault, otherwise ``False``.
        """
        return (self.status == self.STATUS_FAULT)

    def invalid(self):
        """Returns whether the response exited with a bad result.

        This occurs when the parity is invalid.

        Args:
          self (Response): the ``Response`` instance

        Returns:
          ``True`` if the parity checked failed, otherwise ``False``.
        """
        return (self.status == self.STATUS_INVALID)


class RequestBits(ctypes.LittleEndianStructure):
    """SWD request bits."""
    _fields_ = [
        ('start',      ctypes.c_uint8, 1),
        ('ap_dp',      ctypes.c_uint8, 1),
        ('read_write', ctypes.c_uint8, 1),
        ('addr2',      ctypes.c_uint8, 1),
        ('addr3',      ctypes.c_uint8, 1),
        ('parity',     ctypes.c_uint8, 1),
        ('stop',       ctypes.c_uint8, 1),
        ('park',       ctypes.c_uint8, 1)
    ]


class Request(ctypes.Union):
    """Definition of a SWD (Serial Wire Debug) Request.

    An SWD Request is composed of 8 bits.

    Attributes:
      start: the start bit is always one
      ap_dp: indicates whether the transaction is DP (``0``) or AP (``1``).
      read_write: indicates if the transaction is a read-access (``1``) or a
          write-access (``0``).
      address:
      parity: the parity bit, the bit is used by the target to verify the
          integrity of the request.  Should be ``1`` if bits ``1-4`` contain an
          odd number of ``1``s, otherwise ``0``.
      stop: the stop bit, should always be zero.
      park: the park bit, should always be one.
      value: the overall value of the request.
    """
    _anonymous_ = ('bit', )
    _fields_ = [
        ('bit',   RequestBits),
        ('value', ctypes.c_uint8),
    ]

    def __init__(self, address, ap, data=None):
        """Initializes the SWD request.

        Calculates the parity and sets the ``APnDP``, ``start``, ``stop``,
        ``park`` and ``address`` bits.

        Args:
          self (Request): the ``Request`` instance
          address (int): the register index (``A[3:2]``)
          ap (bool): ``True`` if this request is to an Access Port Access
              Register, otherwise ``False`` for a Debug Port Access Register
          data (int): data to write, if any (indicates a write request)

        Returns:
          ``None``
        """
        super(Request, self).__init__()

        self.start = 1
        self.ap_dp = 1 if ap else 0
        self.read_write = 0 if data is not None else 1
        self.addr2 = (address >> 0) & 1
        self.addr3 = (address >> 1) & 1

        parity = self.ap_dp ^ self.read_write ^ self.addr2 ^ self.addr3
        self.parity = parity

        self.stop = 0
        self.park = 1

        self.data = data

    def send(self, jlink):
        """Starts the SWD transaction.

        Sends the request and recieves an ACK for the request.

        Args:
          self (Request): the ``Request`` instance
          jlink (JLink): the ``JLink`` instance to use for write/read

        Returns:
          The bit position of the ACK response.
        """
        # Send the request over SWD.
        jlink.swd_write8(0xFF, self.value)

        # Receive the ACK.
        return jlink.swd_write(0x0, 0x0, 3)


class ReadRequest(Request):
    """Definition for a SWD (Serial Wire Debug) Read Request."""

    def __init__(self, address, ap):
        """Initializes the base class.

        Args:
          self (ReadRequest): the ``ReadRequest`` instance
          address (int): the register index
          ap (bool): ``True`` if this request is to an Access Port Access
              Register, otherwise ``False`` for a Debug Port Access Register

        Returns:
          ``None``
        """
        super(ReadRequest, self).__init__(address=address, ap=ap)

    def send(self, jlink):
        """Starts the SWD transaction.

        Steps for a Read Transaction:
          1.  First phase in which the request is sent.
          2.  Second phase in which an ACK is received.  This phase consists of
              three bits. An OK response has the value ``1``.
          3.  Once the ACK is received, the data phase can begin.  Consists of
              ``32`` data bits followed by ``1`` parity bit calclulated based
              on all ``32`` data bits.
          4.  After the data phase, the interface must be clocked for at least
              eight cycles to clock the transaction through the SW-DP; this is
              done by reading an additional eight bits (eight clocks).

        Args:
          self (ReadRequest): the ``ReadRequest`` instance
          jlink (JLink): the ``JLink`` instance to use for write/read

        Returns:
          An ``Response`` instance.
        """
        ack = super(ReadRequest, self).send(jlink)

        # Write the read command, then read the data and status.
        jlink.swd_write32(0x0, 0x0)
        jlink.swd_write8(0xFC, 0x0)
        status = jlink.swd_read8(ack) & 7
        data = jlink.swd_read32(ack + 3)

        if status == Response.STATUS_ACK:
            # Check the parity
            parity = jlink.swd_read8(ack + 35) & 1
            if util.calculate_parity(data) != parity:
                return Response(-1, data)

        return Response(status, data)


class WriteRequest(Request):
    """Definition for a SWD (Serial Wire Debug) Write Request."""

    def __init__(self, address, ap, data):
        """Initializes the base class.

        Args:
          self (WriteRequest): the ``WriteRequest`` instance
          address (int): the register index
          ap (bool): ``True`` if this request is to an Access Port Access
              Register, otherwise ``False`` for a Debug Port Access Register

        Returns:
          ``None``
        """
        super(WriteRequest, self).__init__(address=address, ap=ap, data=data)

    def send(self, jlink):
        """Starts the SWD transaction.

        Steps for a Write Transaction:
          1.  First phase in which the request is sent.
          2.  Second phase in which an ACK is received.  This phase consists of
              three bits. An OK response has the value ``1``.
          3.  Everytime the SWD IO may change directions, a turnaround phase is
              inserted.  For reads, this happens after the data phase, while
              for writes this happens after between the acknowledge and data
              phase, so we have to do the turnaround before writing data.  This
              phase consists of two bits.
          4.  Write the data and parity bits.

        Args:
          self (WriteRequest): the ``WriteRequest`` instance
          jlink (JLink): the ``JLink`` instance to use for write/read

        Returns:
          An ``Response`` instance.
        """
        ack = super(WriteRequest, self).send(jlink)

        # Turnaround phase for write.
        jlink.swd_write(0x0, 0x0, 2)

        # Write the data and the parity bits.
        jlink.swd_write32(0xFFFFFFFF, self.data)
        jlink.swd_write8(0xFF, util.calculate_parity(self.data))
        return Response(jlink.swd_read8(ack) & 7)
