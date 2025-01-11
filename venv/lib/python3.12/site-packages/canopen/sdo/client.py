import struct
import logging
import io
import time
import queue

from canopen.network import CanError
from canopen import objectdictionary
from canopen.sdo.base import SdoBase
from canopen.utils import pretty_index
from canopen.sdo.constants import *
from canopen.sdo.exceptions import *

logger = logging.getLogger(__name__)


class SdoClient(SdoBase):
    """Handles communication with an SDO server."""

    #: Max time in seconds to wait for response from server
    RESPONSE_TIMEOUT = 0.3

    #: Max number of request retries before raising error
    MAX_RETRIES = 1

    #: Seconds to wait before sending a request, for rate limiting
    PAUSE_BEFORE_SEND = 0.0

    #: Seconds to wait before retrying a request after a send error
    RETRY_DELAY = 0.1

    def __init__(self, rx_cobid, tx_cobid, od):
        """
        :param int rx_cobid:
            COB-ID that the server receives on (usually 0x600 + node ID)
        :param int tx_cobid:
            COB-ID that the server responds with (usually 0x580 + node ID)
        :param canopen.ObjectDictionary od:
            Object Dictionary to use for communication
        """
        SdoBase.__init__(self, rx_cobid, tx_cobid, od)
        self.responses = queue.Queue()

    def on_response(self, can_id, data, timestamp):
        self.responses.put(bytes(data))

    def send_request(self, request):
        retries_left = self.MAX_RETRIES
        if self.PAUSE_BEFORE_SEND:
            time.sleep(self.PAUSE_BEFORE_SEND)
        while True:
            try:
                self.network.send_message(self.rx_cobid, request)
            except CanError as e:
                # Could be a buffer overflow. Wait some time before trying again
                retries_left -= 1
                if not retries_left:
                    raise
                logger.info(str(e))
                if self.RETRY_DELAY:
                    time.sleep(self.RETRY_DELAY)
            else:
                break

    def read_response(self):
        try:
            response = self.responses.get(
                block=True, timeout=self.RESPONSE_TIMEOUT)
        except queue.Empty:
            raise SdoCommunicationError("No SDO response received")
        res_command, = struct.unpack_from("B", response)
        if res_command == RESPONSE_ABORTED:
            abort_code, = struct.unpack_from("<L", response, 4)
            raise SdoAbortedError(abort_code)
        return response

    def request_response(self, sdo_request):
        retries_left = self.MAX_RETRIES
        if not self.responses.empty():
            # logger.warning("There were unexpected messages in the queue")
            self.responses = queue.Queue()
        while True:
            self.send_request(sdo_request)
            # Wait for node to respond
            try:
                return self.read_response()
            except SdoCommunicationError as e:
                retries_left -= 1
                if not retries_left:
                    self.abort(0x5040000)
                    raise
                logger.warning(str(e))

    def abort(self, abort_code=0x08000000):
        """Abort current transfer."""
        request = bytearray(8)
        request[0] = REQUEST_ABORTED
        # TODO: Is it necessary to include index and subindex?
        struct.pack_into("<L", request, 4, abort_code)
        self.send_request(request)
        logger.error("Transfer aborted by client with code 0x%08X", abort_code)

    def upload(self, index: int, subindex: int) -> bytes:
        """May be called to make a read operation without an Object Dictionary.

        :param index:
            Index of object to read.
        :param subindex:
            Sub-index of object to read.

        :return: A data object.

        :raises canopen.SdoCommunicationError:
            On unexpected response or timeout.
        :raises canopen.SdoAbortedError:
            When node responds with an error.
        """
        with self.open(index, subindex, buffering=0) as fp:
            response_size = fp.size
            data = fp.read()

        # If size is available through variable in OD, then use the smaller of the two sizes.
        # Some devices send U32/I32 even if variable is smaller in OD
        var = self.od.get_variable(index, subindex)
        if var is not None:
            # Found a matching variable in OD
            # If this is a data type (string, domain etc) the size is
            # unknown anyway so keep the data as is
            if var.data_type not in objectdictionary.DATA_TYPES:
                # Get the size in bytes for this variable
                var_size = len(var) // 8
                if response_size is None or var_size < response_size:
                    # Truncate the data to specified size
                    data = data[0:var_size]
        return data

    def download(
        self,
        index: int,
        subindex: int,
        data: bytes,
        force_segment: bool = False,
    ) -> None:
        """May be called to make a write operation without an Object Dictionary.

        :param index:
            Index of object to write.
        :param subindex:
            Sub-index of object to write.
        :param data:
            Data to be written.
        :param force_segment:
            Force use of segmented transfer regardless of data size.

        :raises canopen.SdoCommunicationError:
            On unexpected response or timeout.
        :raises canopen.SdoAbortedError:
            When node responds with an error.
        """
        with self.open(index, subindex, "wb", buffering=7, size=len(data),
                       force_segment=force_segment) as fp:
            fp.write(data)

    def open(self, index, subindex=0, mode="rb", encoding="ascii",
             buffering=1024, size=None, block_transfer=False, force_segment=False, request_crc_support=True):
        """Open the data stream as a file like object.

        :param int index:
            Index of object to open.
        :param int subindex:
            Sub-index of object to open.
        :param str mode:
            ========= ==========================================================
            Character Meaning
            --------- ----------------------------------------------------------
            'r'       open for reading (default)
            'w'       open for writing
            'b'       binary mode (default)
            't'       text mode
            ========= ==========================================================
        :param str encoding:
            The str name of the encoding used to decode or encode the file.
            This will only be used in text mode.
        :param int buffering:
            An optional integer used to set the buffering policy. Pass 0 to
            switch buffering off (only allowed in binary mode), 1 to select line
            buffering (only usable in text mode), and an integer > 1 to indicate
            the size in bytes of a fixed-size chunk buffer.
        :param int size:
            Size of data to that will be transmitted.
        :param bool block_transfer:
            If block transfer should be used.
        :param bool force_segment:
            Force use of segmented download regardless of data size.
        :param bool request_crc_support:
            If crc calculation should be requested when using block transfer

        :returns:
            A file like object.
        """
        buffer_size = buffering if buffering > 1 else io.DEFAULT_BUFFER_SIZE
        if "r" in mode:
            if block_transfer:
                raw_stream = BlockUploadStream(self, index, subindex, request_crc_support=request_crc_support)
            else:
                raw_stream = ReadableStream(self, index, subindex)
            if buffering:
                buffered_stream = io.BufferedReader(raw_stream, buffer_size=buffer_size)
            else:
                return raw_stream
        if "w" in mode:
            if block_transfer:
                raw_stream = BlockDownloadStream(self, index, subindex, size, request_crc_support=request_crc_support)
            else:
                raw_stream = WritableStream(self, index, subindex, size, force_segment)
            if buffering:
                buffered_stream = io.BufferedWriter(raw_stream, buffer_size=buffer_size)
            else:
                return raw_stream
        if "b" not in mode:
            # Text mode
            line_buffering = buffering == 1
            return io.TextIOWrapper(buffered_stream, encoding,
                                    line_buffering=line_buffering)
        return buffered_stream


class ReadableStream(io.RawIOBase):
    """File like object for reading from a variable."""

    #: Total size of data or ``None`` if not specified
    size = None

    def __init__(self, sdo_client, index, subindex=0):
        """
        :param canopen.sdo.SdoClient sdo_client:
            The SDO client to use for reading.
        :param int index:
            Object dictionary index to read from.
        :param int subindex:
            Object dictionary sub-index to read from.
        """
        self._done = False
        self.sdo_client = sdo_client
        self._toggle = 0
        self.pos = 0

        logger.debug("Reading 0x%04X:%02X from node %d", index, subindex,
                     sdo_client.rx_cobid - 0x600)
        request = bytearray(8)
        SDO_STRUCT.pack_into(request, 0, REQUEST_UPLOAD, index, subindex)
        response = sdo_client.request_response(request)
        res_command, res_index, res_subindex = SDO_STRUCT.unpack_from(response)
        res_data = response[4:8]

        if res_command & 0xE0 != RESPONSE_UPLOAD:
            raise SdoCommunicationError(f"Unexpected response 0x{res_command:02X}")

        # Check that the message is for us
        if res_index != index or res_subindex != subindex:
            raise SdoCommunicationError(
                f"Node returned a value for {pretty_index(res_index, res_subindex)} instead, "
                "maybe there is another SDO client communicating "
                "on the same SDO channel?")

        self.exp_data = None
        if res_command & EXPEDITED:
            # Expedited upload
            if res_command & SIZE_SPECIFIED:
                self.size = 4 - ((res_command >> 2) & 0x3)
                self.exp_data = res_data[:self.size]
            else:
                self.exp_data = res_data
            self.pos += len(self.exp_data)
        elif res_command & SIZE_SPECIFIED:
            self.size, = struct.unpack("<L", res_data)
            logger.debug("Using segmented transfer of %d bytes", self.size)
        else:
            logger.debug("Using segmented transfer")

    def read(self, size=-1):
        """Read one segment which may be up to 7 bytes.

        :param int size:
            If size is -1, all data will be returned. Other values are ignored.

        :returns: 1 - 7 bytes of data or no bytes if EOF.
        :rtype: bytes
        """
        if self._done:
            return b""
        if self.exp_data is not None:
            self._done = True
            return self.exp_data
        if size is None or size < 0:
            return self.readall()

        command = REQUEST_SEGMENT_UPLOAD
        command |= self._toggle
        request = bytearray(8)
        request[0] = command
        response = self.sdo_client.request_response(request)
        res_command, = struct.unpack_from("B", response)
        if res_command & 0xE0 != RESPONSE_SEGMENT_UPLOAD:
            raise SdoCommunicationError(f"Unexpected response 0x{res_command:02X}")
        if res_command & TOGGLE_BIT != self._toggle:
            raise SdoCommunicationError("Toggle bit mismatch")
        length = 7 - ((res_command >> 1) & 0x7)
        if res_command & NO_MORE_DATA:
            self._done = True
        self._toggle ^= TOGGLE_BIT
        self.pos += length
        return response[1:length + 1]

    def readinto(self, b):
        """
        Read bytes into a pre-allocated, writable bytes-like object b,
        and return the number of bytes read.
        """
        data = self.read(7)
        b[:len(data)] = data
        return len(data)

    def readable(self):
        return True

    def tell(self):
        return self.pos


class WritableStream(io.RawIOBase):
    """File like object for writing to a variable."""

    def __init__(self, sdo_client, index, subindex=0, size=None, force_segment=False):
        """
        :param canopen.sdo.SdoClient sdo_client:
            The SDO client to use for communication.
        :param int index:
            Object dictionary index to read from.
        :param int subindex:
            Object dictionary sub-index to read from.
        :param int size:
            Size of data in number of bytes if known in advance.
        :param bool force_segment:
            Force use of segmented transfer regardless of size.
        """
        self.sdo_client = sdo_client
        self.size = size
        self.pos = 0
        self._toggle = 0
        self._exp_header = None
        self._done = False

        if size is None or size > 4 or force_segment:
            # Initiate segmented download
            request = bytearray(8)
            command = REQUEST_DOWNLOAD
            if size is not None:
                command |= SIZE_SPECIFIED
                struct.pack_into("<L", request, 4, size)
            SDO_STRUCT.pack_into(request, 0, command, index, subindex)
            response = sdo_client.request_response(request)
            res_command, = struct.unpack_from("B", response)
            if res_command != RESPONSE_DOWNLOAD:
                raise SdoCommunicationError(
                    f"Unexpected response 0x{res_command:02X}")
        else:
            # Expedited download
            # Prepare header (first 4 bytes in CAN message)
            command = REQUEST_DOWNLOAD | EXPEDITED | SIZE_SPECIFIED
            command |= (4 - size) << 2
            self._exp_header = SDO_STRUCT.pack(command, index, subindex)

    def write(self, b):
        """
        Write the given bytes-like object, b, to the SDO server, and return the
        number of bytes written. This will be at most 7 bytes.
        """
        if self._done:
            raise RuntimeError("All expected data has already been transmitted")
        if self._exp_header is not None:
            # Expedited download
            if len(b) < self.size:
                # Not enough data provided
                return 0
            if len(b) > 4:
                raise AssertionError("More data received than expected")
            data = b.tobytes() if isinstance(b, memoryview) else b
            request = self._exp_header + data.ljust(4, b"\x00")
            response = self.sdo_client.request_response(request)
            res_command, = struct.unpack_from("B", response)
            if res_command & 0xE0 != RESPONSE_DOWNLOAD:
                raise SdoCommunicationError(
                    f"Unexpected response 0x{res_command:02X}")
            bytes_sent = len(b)
            self._done = True
        else:
            # Segmented download
            request = bytearray(8)
            command = REQUEST_SEGMENT_DOWNLOAD
            # Add toggle bit
            command |= self._toggle
            self._toggle ^= TOGGLE_BIT
            # Can send up to 7 bytes at a time
            bytes_sent = min(len(b), 7)
            if self.size is not None and self.pos + bytes_sent >= self.size:
                # No more data after this message
                command |= NO_MORE_DATA
                self._done = True
            # Specify number of bytes that do not contain segment data
            command |= (7 - bytes_sent) << 1
            request[0] = command
            request[1:bytes_sent + 1] = b[0:bytes_sent]
            response = self.sdo_client.request_response(request)
            res_command, = struct.unpack("B", response[0:1])
            if res_command & 0xE0 != RESPONSE_SEGMENT_DOWNLOAD:
                raise SdoCommunicationError(
                    f"Unexpected response 0x{res_command:02X} "
                    f"(expected 0x{RESPONSE_SEGMENT_DOWNLOAD:02X})")
        # Advance position
        self.pos += bytes_sent
        return bytes_sent

    def close(self):
        """Closes the stream.

        An empty segmented SDO message may be sent saying there is no more data.
        """
        super(WritableStream, self).close()
        if not self._done and not self._exp_header:
            # Segmented download not finished
            command = REQUEST_SEGMENT_DOWNLOAD | NO_MORE_DATA
            command |= self._toggle
            # No data in this message
            command |= 7 << 1
            request = bytearray(8)
            request[0] = command
            self.sdo_client.request_response(request)
            self._done = True

    def writable(self):
        return True

    def tell(self):
        return self.pos


class BlockUploadStream(io.RawIOBase):
    """File like object for reading from a variable using block upload."""

    #: Total size of data or ``None`` if not specified
    size = None

    blksize = 127

    crc_supported = False

    def __init__(self, sdo_client, index, subindex=0, request_crc_support=True):
        """
        :param canopen.sdo.SdoClient sdo_client:
            The SDO client to use for reading.
        :param int index:
            Object dictionary index to read from.
        :param int subindex:
            Object dictionary sub-index to read from.
        :param bool request_crc_support:
            If crc calculation should be requested when using block transfer
        """
        self._done = False
        self.sdo_client = sdo_client
        self.pos = 0
        self._crc = sdo_client.crc_cls()
        self._server_crc = None
        self._ackseq = 0
        self._error = False

        logger.debug("Reading 0x%04X:%02X from node %d", index, subindex,
                     sdo_client.rx_cobid - 0x600)
        # Initiate Block Upload
        request = bytearray(8)
        command = REQUEST_BLOCK_UPLOAD | INITIATE_BLOCK_TRANSFER
        if request_crc_support:
            command |= CRC_SUPPORTED
        struct.pack_into("<BHBBB", request, 0,
                         command, index, subindex, self.blksize, 0)
        response = sdo_client.request_response(request)
        res_command, res_index, res_subindex = SDO_STRUCT.unpack_from(response)
        if res_command & 0xE0 != RESPONSE_BLOCK_UPLOAD:
            self._error = True
            self.sdo_client.abort(0x05040001)
            raise SdoCommunicationError(f"Unexpected response 0x{res_command:02X}")
        # Check that the message is for us
        if res_index != index or res_subindex != subindex:
            self._error = True
            raise SdoCommunicationError(
                f"Node returned a value for {pretty_index(res_index, res_subindex)} instead, "
                "maybe there is another SDO client communicating "
                "on the same SDO channel?")
        if res_command & BLOCK_SIZE_SPECIFIED:
            self.size, = struct.unpack_from("<L", response, 4)
            logger.debug("Size is %d bytes", self.size)
        self.crc_supported = bool(res_command & CRC_SUPPORTED)
        # Start upload
        request = bytearray(8)
        request[0] = REQUEST_BLOCK_UPLOAD | START_BLOCK_UPLOAD
        sdo_client.send_request(request)

    def read(self, size=-1):
        """Read one segment which may be up to 7 bytes.

        :param int size:
            If size is -1, all data will be returned. Other values are ignored.

        :returns: 1 - 7 bytes of data or no bytes if EOF.
        :rtype: bytes
        """
        if self._done:
            return b""
        if size is None or size < 0:
            return self.readall()

        try:
            response = self.sdo_client.read_response()
        except SdoCommunicationError:
            response = self._retransmit()
        res_command, = struct.unpack_from("B", response)
        seqno = res_command & 0x7F
        if seqno == self._ackseq + 1:
            self._ackseq = seqno
        else:
            # Wrong sequence number
            response = self._retransmit()
            res_command, = struct.unpack_from("B", response)
        if self._ackseq >= self.blksize or res_command & NO_MORE_BLOCKS:
            self._ack_block()
        if res_command & NO_MORE_BLOCKS:
            n = self._end_upload()
            data = response[1:8 - n]
            self._done = True
        else:
            data = response[1:8]
        if self.crc_supported:
            self._crc.process(data)
            if self._done:
                if self._server_crc != self._crc.final():
                    self._error = True
                    self.sdo_client.abort(0x05040004)
                    raise SdoCommunicationError("CRC is not OK")
                logger.info("CRC is OK")
        self.pos += len(data)
        return data

    def _retransmit(self):
        logger.info("Only %d sequences were received. Requesting retransmission",
                    self._ackseq)
        end_time = time.time() + self.sdo_client.RESPONSE_TIMEOUT
        self._ack_block()
        while time.time() < end_time:
            response = self.sdo_client.read_response()
            res_command, = struct.unpack_from("B", response)
            seqno = res_command & 0x7F
            if seqno == self._ackseq + 1:
                # We should be back in sync
                self._ackseq = seqno
                return response
        self._error = True
        self.sdo_client.abort(0x05040000)
        raise SdoCommunicationError("Some data were lost and could not be retransmitted")

    def _ack_block(self):
        request = bytearray(8)
        request[0] = REQUEST_BLOCK_UPLOAD | BLOCK_TRANSFER_RESPONSE
        request[1] = self._ackseq
        request[2] = self.blksize
        self.sdo_client.send_request(request)
        if self._ackseq == self.blksize:
            self._ackseq = 0

    def _end_upload(self):
        response = self.sdo_client.read_response()
        res_command, self._server_crc = struct.unpack_from("<BH", response)
        if res_command & 0xE0 != RESPONSE_BLOCK_UPLOAD:
            self._error = True
            self.sdo_client.abort(0x05040001)
            raise SdoCommunicationError(f"Unexpected response 0x{res_command:02X}")
        if res_command & 0x3 != END_BLOCK_TRANSFER:
            self._error = True
            self.sdo_client.abort(0x05040001)
            raise SdoCommunicationError("Server did not end transfer as expected")
        # Return number of bytes not used in last message
        return (res_command >> 2) & 0x7

    def close(self):
        if self.closed:
            return
        super(BlockUploadStream, self).close()
        if self._done and not self._error:
            request = bytearray(8)
            request[0] = REQUEST_BLOCK_UPLOAD | END_BLOCK_TRANSFER
            self.sdo_client.send_request(request)

    def tell(self):
        return self.pos

    def readinto(self, b):
        """
        Read bytes into a pre-allocated, writable bytes-like object b,
        and return the number of bytes read.
        """
        data = self.read(7)
        b[:len(data)] = data
        return len(data)

    def readable(self):
        return True


class BlockDownloadStream(io.RawIOBase):
    """File like object for block download."""

    def __init__(self, sdo_client, index, subindex=0, size=None, request_crc_support=True):
        """
        :param canopen.sdo.SdoClient sdo_client:
            The SDO client to use for communication.
        :param int index:
            Object dictionary index to read from.
        :param int subindex:
            Object dictionary sub-index to read from.
        :param int size:
            Size of data in number of bytes if known in advance.
        :param bool request_crc_support:
            If crc calculation should be requested when using block transfer
        """
        self.sdo_client = sdo_client
        self.size = size
        self.pos = 0
        self._done = False
        self._seqno = 0
        self._crc = sdo_client.crc_cls()
        self._last_bytes_sent = 0
        self._current_block = []
        self._retransmitting = False
        command = REQUEST_BLOCK_DOWNLOAD | INITIATE_BLOCK_TRANSFER
        if request_crc_support:
            command |= CRC_SUPPORTED
        request = bytearray(8)
        logger.info("Initiating block download for 0x%04X:%02X", index, subindex)
        if size is not None:
            logger.debug("Expected size of data is %d bytes", size)
            command |= BLOCK_SIZE_SPECIFIED
            struct.pack_into("<L", request, 4, size)
        else:
            logger.warning("Data size has not been specified")
        SDO_STRUCT.pack_into(request, 0, command, index, subindex)
        response = sdo_client.request_response(request)
        res_command, res_index, res_subindex = SDO_STRUCT.unpack_from(response)
        if res_command & 0xE0 != RESPONSE_BLOCK_DOWNLOAD:
            self.sdo_client.abort(0x05040001)
            raise SdoCommunicationError(
                f"Unexpected response 0x{res_command:02X}")
        # Check that the message is for us
        if res_index != index or res_subindex != subindex:
            self.sdo_client.abort()
            raise SdoCommunicationError(
                f"Node returned a value for {pretty_index(res_index, res_subindex)} instead, "
                "maybe there is another SDO client communicating "
                "on the same SDO channel?")
        self._blksize, = struct.unpack_from("B", response, 4)
        logger.debug("Server requested a block size of %d", self._blksize)
        self.crc_supported = bool(res_command & CRC_SUPPORTED)

    def write(self, b):
        """
        Write the given bytes-like object, b, to the SDO server, and return the
        number of bytes written. This will be at most 7 bytes.

        :param bytes b:
            Data to be transmitted.

        :returns:
            Number of bytes successfully sent or ``None`` if length of data is
            less than 7 bytes and the total size has not been reached yet.
        """
        if self._done:
            raise RuntimeError("All expected data has already been transmitted")
        # Can send up to 7 bytes at a time
        data = b[0:7]
        if self.size is not None and self.pos + len(data) >= self.size:
            # This is the last data to be transmitted based on expected size
            self.send(data, end=True)
        elif len(data) < 7:
            # We can't send less than 7 bytes in the middle of a transmission
            return None
        else:
            self.send(data)
        return len(data)

    def send(self, b, end=False):
        """Send up to 7 bytes of data.

        :param bytes b:
            0 - 7 bytes of data to transmit.
        :param bool end:
            If this is the last data.
        """
        assert len(b) <= 7, "Max 7 bytes can be sent"
        if not end:
            assert len(b) == 7, "Less than 7 bytes only allowed if last data"
        self._seqno += 1
        command = self._seqno
        if end:
            command |= NO_MORE_BLOCKS
            self._done = True
            # Change expected ACK:ed sequence
            self._blksize = self._seqno
            # Save how many bytes this message contains since this is the last
            self._last_bytes_sent = len(b)
        request = bytearray(8)
        request[0] = command
        request[1:len(b) + 1] = b
        self.sdo_client.send_request(request)
        self.pos += len(b)
        # Add the sent data to the current block buffer
        self._current_block.append(b)
        # Don't calculate crc if retransmitting
        if self.crc_supported and not self._retransmitting:
            # Calculate CRC
            self._crc.process(b)
        if self._seqno >= self._blksize:
            # End of this block, wait for ACK
            self._block_ack()

    def tell(self):
        return self.pos

    def _block_ack(self):
        logger.debug("Waiting for acknowledgement of last block...")
        response = self.sdo_client.read_response()
        res_command, ackseq, blksize = struct.unpack_from("BBB", response)
        if res_command & 0xE0 != RESPONSE_BLOCK_DOWNLOAD:
            self.sdo_client.abort(0x05040001)
            raise SdoCommunicationError(
                f"Unexpected response 0x{res_command:02X}")
        if res_command & 0x3 != BLOCK_TRANSFER_RESPONSE:
            self.sdo_client.abort(0x05040001)
            raise SdoCommunicationError("Server did not respond with a "
                                        "block download response")
        if ackseq != self._blksize:
            # Sequence error, try to retransmit
            self._retransmit(ackseq, blksize)
            # We should be back in sync
            return
        # Clear the current block buffer
        self._current_block = []
        logger.debug("All %d sequences were received successfully", ackseq)
        logger.debug("Server requested a block size of %d", blksize)
        self._blksize = blksize
        self._seqno = 0

    def _retransmit(self, ackseq, blksize):
        """Retransmit the failed block"""
        logger.info("%d of %d sequences were received. "
                    "Will start retransmission", ackseq, self._blksize)
        # Sub blocks betwen ackseq and end of corrupted block need to be resent
        # Get the part of the block to resend
        block = self._current_block[ackseq:]
        # Go back to correct position in stream
        self.pos = self.pos - (len(block) * 7)
        # Reset the _current_block before starting the retransmission
        self._current_block = []
        # Reset _seqno and update blksize
        self._seqno = 0
        self._blksize = blksize
        # We are retransmitting
        self._retransmitting = True
        # Resend the block
        for b in block:
            self.write(b)
        self._retransmitting = False

    def close(self):
        """Closes the stream."""
        if self.closed:
            return
        super(BlockDownloadStream, self).close()
        if not self._done:
            logger.error("Block transfer was not finished")
        command = REQUEST_BLOCK_DOWNLOAD | END_BLOCK_TRANSFER
        # Specify number of bytes in last message that did not contain data
        command |= (7 - self._last_bytes_sent) << 2
        request = bytearray(8)
        request[0] = command
        if self.crc_supported:
            # Add CRC
            struct.pack_into("<H", request, 1, self._crc.final())
        logger.debug("Ending block transfer...")
        response = self.sdo_client.request_response(request)
        res_command, = struct.unpack_from("B", response)
        if not res_command & END_BLOCK_TRANSFER:
            raise SdoCommunicationError("Block download unsuccessful")
        logger.info("Block download successful")

    def writable(self):
        return True
