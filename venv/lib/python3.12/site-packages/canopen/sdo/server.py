import logging

from canopen.sdo.base import SdoBase
from canopen.sdo.constants import *
from canopen.sdo.exceptions import *

logger = logging.getLogger(__name__)


class SdoServer(SdoBase):
    """Creates an SDO server."""

    def __init__(self, rx_cobid, tx_cobid, node):
        """
        :param int rx_cobid:
            COB-ID that the server receives on (usually 0x600 + node ID)
        :param int tx_cobid:
            COB-ID that the server responds with (usually 0x580 + node ID)
        :param canopen.LocalNode od:
            Node object owning the server
        """
        SdoBase.__init__(self, rx_cobid, tx_cobid, node.object_dictionary)
        self._node = node
        self._buffer = None
        self._toggle = 0
        self._index = None
        self._subindex = None
        self.last_received_error = 0x00000000

    def on_request(self, can_id, data, timestamp):
        command, = struct.unpack_from("B", data, 0)
        ccs = command & 0xE0

        try:
            if ccs == REQUEST_UPLOAD:
                self.init_upload(data)
            elif ccs == REQUEST_SEGMENT_UPLOAD:
                self.segmented_upload(command)
            elif ccs == REQUEST_DOWNLOAD:
                self.init_download(data)
            elif ccs == REQUEST_SEGMENT_DOWNLOAD:
                self.segmented_download(command, data)
            elif ccs == REQUEST_BLOCK_UPLOAD:
                self.block_upload(data)
            elif ccs == REQUEST_BLOCK_DOWNLOAD:
                self.block_download(data)
            elif ccs == REQUEST_ABORTED:
                self.request_aborted(data)
            else:
                self.abort(0x05040001)
        except SdoAbortedError as exc:
            self.abort(exc.code)
        except KeyError as exc:
            self.abort(0x06020000)
        except Exception as exc:
            self.abort()
            logger.exception(exc)

    def init_upload(self, request):
        _, index, subindex = SDO_STRUCT.unpack_from(request)
        self._index = index
        self._subindex = subindex
        res_command = RESPONSE_UPLOAD | SIZE_SPECIFIED
        response = bytearray(8)

        data = self._node.get_data(index, subindex, check_readable=True)
        size = len(data)
        if size <= 4:
            logger.info("Expedited upload for 0x%04X:%02X", index, subindex)
            res_command |= EXPEDITED
            res_command |= (4 - size) << 2
            response[4:4 + size] = data
        else:
            logger.info("Initiating segmented upload for 0x%04X:%02X", index, subindex)
            struct.pack_into("<L", response, 4, size)
            self._buffer = bytearray(data)
            self._toggle = 0

        SDO_STRUCT.pack_into(response, 0, res_command, index, subindex)
        self.send_response(response)

    def segmented_upload(self, command):
        if command & TOGGLE_BIT != self._toggle:
            # Toggle bit mismatch
            raise SdoAbortedError(0x05030000)
        data = self._buffer[:7]
        size = len(data)

        # Remove sent data from buffer
        del self._buffer[:7]

        res_command = RESPONSE_SEGMENT_UPLOAD
        # Add toggle bit
        res_command |= self._toggle
        # Add nof bytes not used
        res_command |= (7 - size) << 1
        if not self._buffer:
            # Nothing left in buffer
            res_command |= NO_MORE_DATA
        # Toggle bit for next message
        self._toggle ^= TOGGLE_BIT

        response = bytearray(8)
        response[0] = res_command
        response[1:1 + size] = data
        self.send_response(response)

    def block_upload(self, data):
        # We currently don't support BLOCK UPLOAD
        # according to CIA301 the server is allowed
        # to switch to regular upload
        logger.info("Received block upload, switch to regular SDO upload")
        self.init_upload(data)

    def request_aborted(self, data):
        _, index, subindex, code = struct.unpack_from("<BHBL", data)
        self.last_received_error = code
        logger.info("Received request aborted for 0x%04X:%02X with code 0x%X", index, subindex, code)

    def block_download(self, data):
        # We currently don't support BLOCK DOWNLOAD
        logger.error("Block download is not supported")
        self.abort(0x05040001)

    def init_download(self, request):
        # TODO: Check if writable (now would fail on end of segmented downloads)
        command, index, subindex = SDO_STRUCT.unpack_from(request)
        self._index = index
        self._subindex = subindex
        res_command = RESPONSE_DOWNLOAD
        response = bytearray(8)

        if command & EXPEDITED:
            logger.info("Expedited download for 0x%04X:%02X", index, subindex)
            if command & SIZE_SPECIFIED:
                size = 4 - ((command >> 2) & 0x3)
            else:
                size = 4
            self._node.set_data(index, subindex, request[4:4 + size], check_writable=True)
        else:
            logger.info("Initiating segmented download for 0x%04X:%02X", index, subindex)
            if command & SIZE_SPECIFIED:
                size, = struct.unpack_from("<L", request, 4)
                logger.info("Size is %d bytes", size)
            self._buffer = bytearray()
            self._toggle = 0

        SDO_STRUCT.pack_into(response, 0, res_command, index, subindex)
        self.send_response(response)

    def segmented_download(self, command, request):
        if command & TOGGLE_BIT != self._toggle:
            # Toggle bit mismatch
            raise SdoAbortedError(0x05030000)
        last_byte = 8 - ((command >> 1) & 0x7)
        self._buffer.extend(request[1:last_byte])

        if command & NO_MORE_DATA:
            self._node.set_data(self._index,
                                self._subindex,
                                self._buffer,
                                check_writable=True)

        res_command = RESPONSE_SEGMENT_DOWNLOAD
        # Add toggle bit
        res_command |= self._toggle
        # Toggle bit for next message
        self._toggle ^= TOGGLE_BIT

        response = bytearray(8)
        response[0] = res_command
        self.send_response(response)

    def send_response(self, response):
        self.network.send_message(self.tx_cobid, response)

    def abort(self, abort_code=0x08000000):
        """Abort current transfer."""
        data = struct.pack("<BHBL", RESPONSE_ABORTED,
                           self._index, self._subindex, abort_code)
        self.send_response(data)
        # logger.error("Transfer aborted with code 0x%08X", abort_code)

    def upload(self, index: int, subindex: int) -> bytes:
        """May be called to make a read operation without an Object Dictionary.

        :param index:
            Index of object to read.
        :param subindex:
            Sub-index of object to read.

        :return: A data object.

        :raises canopen.SdoAbortedError:
            When node responds with an error.
        """
        return self._node.get_data(index, subindex)

    def download(
        self,
        index: int,
        subindex: int,
        data: bytes,
        force_segment: bool = False,
    ):
        """May be called to make a write operation without an Object Dictionary.

        :param index:
            Index of object to write.
        :param subindex:
            Sub-index of object to write.
        :param data:
            Data to be written.

        :raises canopen.SdoAbortedError:
            When node responds with an error.
        """
        return self._node.set_data(index, subindex, data)
