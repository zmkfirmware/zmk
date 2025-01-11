import logging
import time
import struct
import queue

logger = logging.getLogger(__name__)

# Command Specifier (CS)
CS_SWITCH_STATE_GLOBAL = 0x04
CS_CONFIGURE_NODE_ID = 0x11
CS_CONFIGURE_BIT_TIMING = 0x13
CS_ACTIVATE_BIT_TIMING = 0x15
CS_STORE_CONFIGURATION = 0x17
CS_SWITCH_STATE_SELECTIVE_VENDOR_ID = 0x40
CS_SWITCH_STATE_SELECTIVE_PRODUCT_CODE = 0x41
CS_SWITCH_STATE_SELECTIVE_REVISION_NUMBER = 0x42
CS_SWITCH_STATE_SELECTIVE_SERIAL_NUMBER = 0x43
CS_SWITCH_STATE_SELECTIVE_RESPONSE = 0x44
CS_IDENTIFY_REMOTE_SLAVE_VENDOR_ID = 0x46               # m -> s
CS_IDENTIFY_REMOTE_SLAVE_PRODUCT_CODE = 0x47            # m -> s
CS_IDENTIFY_REMOTE_SLAVE_REVISION_NUMBER_LOW = 0x48     # m -> s
CS_IDENTIFY_REMOTE_SLAVE_REVISION_NUMBER_HIGH = 0x49    # m -> s
CS_IDENTIFY_REMOTE_SLAVE_SERIAL_NUMBER_LOW = 0x4A       # m -> s
CS_IDENTIFY_REMOTE_SLAVE_SERIAL_NUMBER_HIGH = 0x4B      # m -> s
CS_IDENTIFY_NON_CONFIGURED_REMOTE_SLAVE = 0x4C          # m -> s
CS_IDENTIFY_SLAVE = 0x4F                                # s -> m
CS_IDENTIFY_NON_CONFIGURED_SLAVE = 0x50                 # s -> m
CS_FAST_SCAN = 0x51                                     # m -> s
CS_INQUIRE_VENDOR_ID = 0x5A
CS_INQUIRE_PRODUCT_CODE = 0x5B
CS_INQUIRE_REVISION_NUMBER = 0x5C
CS_INQUIRE_SERIAL_NUMBER = 0x5D
CS_INQUIRE_NODE_ID = 0x5E

# obsolete
SWITCH_MODE_GLOBAL = 0x04
CONFIGURE_NODE_ID = 0x11
CONFIGURE_BIT_TIMING = 0x13
STORE_CONFIGURATION = 0x17
INQUIRE_NODE_ID = 0x5E

ERROR_NONE = 0
ERROR_INADMISSIBLE = 1

ERROR_STORE_NONE = 0
ERROR_STORE_NOT_SUPPORTED = 1
ERROR_STORE_ACCESS_PROBLEM = 2

ERROR_VENDOR_SPECIFIC = 0xff

ListMessageNeedResponse = [
    CS_CONFIGURE_NODE_ID,
    CS_CONFIGURE_BIT_TIMING,
    CS_STORE_CONFIGURATION,
    CS_SWITCH_STATE_SELECTIVE_SERIAL_NUMBER,
    CS_FAST_SCAN,
    CS_INQUIRE_VENDOR_ID,
    CS_INQUIRE_PRODUCT_CODE,
    CS_INQUIRE_REVISION_NUMBER,
    CS_INQUIRE_SERIAL_NUMBER,
    CS_INQUIRE_NODE_ID,
]


class LssMaster:
    """The Master of Layer Setting Services"""

    LSS_TX_COBID = 0x7E5
    LSS_RX_COBID = 0x7E4

    WAITING_STATE = 0x00
    CONFIGURATION_STATE = 0x01

    # obsolete
    NORMAL_MODE = 0x00
    CONFIGURATION_MODE = 0x01

    #: Max time in seconds to wait for response from server
    RESPONSE_TIMEOUT = 0.5

    def __init__(self):
        self.network = None
        self._node_id = 0
        self._data = None
        self.responses = queue.Queue()

    def send_switch_state_global(self, mode):
        """switch mode to CONFIGURATION_STATE or WAITING_STATE
        in the all slaves on CAN bus.
        There is no reply for this request

        :param int mode:
            CONFIGURATION_STATE or WAITING_STATE
        """
        # LSS messages are always a full 8 bytes long.
        # Unused bytes are reserved and should be initialized with 0.
        message = bytearray(8)

        message[0] = CS_SWITCH_STATE_GLOBAL
        message[1] = mode
        self.__send_command(message)

    def send_switch_mode_global(self, mode):
        """obsolete"""
        self.send_switch_state_global(mode)

    def send_switch_state_selective(self,
                                    vendorId, productCode, revisionNumber, serialNumber):
        """switch mode from WAITING_STATE to CONFIGURATION_STATE
        only if 128bits LSS address matches with the arguments.
        It sends 4 messages for each argument.
        Then wait the response from the slave.
        There will be no response if there is no matching slave

        :param int vendorId:
            object index 0x1018 subindex 1
        :param int productCode:
            object index 0x1018 subindex 2
        :param int revisionNumber:
            object index 0x1018 subindex 3
        :param int serialNumber:
            object index 0x1018 subindex 4

        :return:
            True if any slave responds.
            False if there is no response.
        :rtype: bool
        """

        self.__send_lss_address(CS_SWITCH_STATE_SELECTIVE_VENDOR_ID, vendorId)
        self.__send_lss_address(CS_SWITCH_STATE_SELECTIVE_PRODUCT_CODE, productCode)
        self.__send_lss_address(CS_SWITCH_STATE_SELECTIVE_REVISION_NUMBER, revisionNumber)
        response = self.__send_lss_address(CS_SWITCH_STATE_SELECTIVE_SERIAL_NUMBER, serialNumber)

        cs = struct.unpack_from("<B", response)[0]
        if cs == CS_SWITCH_STATE_SELECTIVE_RESPONSE:
            return True

        return False

    def inquire_node_id(self):
        """Read the node id.
        CANopen node id must be within the range from 1 to 127.

        :return:
            node id. 0 means it is not read by LSS protocol
        :rtype: int
        """
        return self.__send_inquire_node_id()

    def inquire_lss_address(self, req_cs):
        """Read the part of LSS address.
            VENDOR_ID, PRODUCT_CODE, REVISION_NUMBER, or SERIAL_NUMBER

        :param int req_cs:
            command specifier for request

        :return:
            part of LSS address
        :rtype: int
        """
        return self.__send_inquire_lss_address(req_cs)

    def configure_node_id(self, new_node_id):
        """Set the node id

        :param int new_node_id:
            new node id to set
        """
        self.__send_configure(CS_CONFIGURE_NODE_ID, new_node_id)

    def configure_bit_timing(self, new_bit_timing):
        """Set the bit timing.

        :param int new_bit_timing:
            bit timing index.
            0: 1 MBit/sec, 1: 800 kBit/sec,
            2: 500 kBit/sec, 3: 250 kBit/sec,
            4: 125 kBit/sec  5: 100 kBit/sec,
            6: 50 kBit/sec, 7: 20 kBit/sec,
            8: 10 kBit/sec
        """
        self.__send_configure(CS_CONFIGURE_BIT_TIMING, 0, new_bit_timing)

    def activate_bit_timing(self, switch_delay_ms):
        """Activate the bit timing.

        :param uint16_t switch_delay_ms:
            The slave that receives this message waits for switch delay,
            then activate the bit timing. But it shouldn't send any message
            until another switch delay is elapsed.
        """

        message = bytearray(8)

        message[0] = CS_ACTIVATE_BIT_TIMING
        message[1:3] = struct.pack('<H', switch_delay_ms)
        self.__send_command(message)

    def store_configuration(self):
        """Store node id and baud rate.
        """
        self.__send_configure(CS_STORE_CONFIGURATION)

    def send_identify_remote_slave(self,
                                   vendorId, productCode,
                                   revisionNumberLow, revisionNumberHigh,
                                   serialNumberLow, serialNumberHigh):

        """This command sends the range of LSS address to find the slave nodes
        in the specified range

        :param int vendorId:
        :param int productCode:
        :param int revisionNumberLow:
        :param int revisionNumberHigh:
        :param int serialNumberLow:
        :param int serialNumberHigh:

        :return:
            True if any slave responds.
            False if there is no response.
        :rtype: bool
        """

        # TODO it should handle the multiple respones from slaves

        self.__send_lss_address(CS_IDENTIFY_REMOTE_SLAVE_VENDOR_ID, vendorId)
        self.__send_lss_address(CS_IDENTIFY_REMOTE_SLAVE_PRODUCT_CODE, productCode)
        self.__send_lss_address(CS_IDENTIFY_REMOTE_SLAVE_REVISION_NUMBER_LOW, revisionNumberLow)
        self.__send_lss_address(CS_IDENTIFY_REMOTE_SLAVE_REVISION_NUMBER_HIGH, revisionNumberHigh)
        self.__send_lss_address(CS_IDENTIFY_REMOTE_SLAVE_SERIAL_NUMBER_LOW, serialNumberLow)
        self.__send_lss_address(CS_IDENTIFY_REMOTE_SLAVE_SERIAL_NUMBER_HIGH, serialNumberHigh)

    def send_identify_non_configured_remote_slave(self):
        # TODO it should handle the multiple respones from slaves
        message = bytearray(8)
        message[0] = CS_IDENTIFY_NON_CONFIGURED_REMOTE_SLAVE
        self.__send_command(message)

    def fast_scan(self):
        """This command sends a series of fastscan message
        to find unconfigured slave with lowest number of LSS idenities

        :return:
            True if a slave is found.
            False if there is no candidate.
            list is the LSS identities [vendor_id, product_code, revision_number, serial_number]
        :rtype: bool, list
        """
        lss_id = [0] * 4
        lss_bit_check = 128
        lss_sub = 0
        lss_next = 0

        if self.__send_fast_scan_message(lss_id[0], lss_bit_check, lss_sub, lss_next):
            time.sleep(0.01)
            while lss_sub < 4:
                lss_bit_check = 32
                while lss_bit_check > 0:
                    lss_bit_check -= 1

                    if not self.__send_fast_scan_message(lss_id[lss_sub], lss_bit_check, lss_sub, lss_next):
                        lss_id[lss_sub] |= 1<<lss_bit_check

                    time.sleep(0.01)

                lss_next = (lss_sub + 1) & 3
                if not self.__send_fast_scan_message(lss_id[lss_sub], lss_bit_check, lss_sub, lss_next):
                    return False, None

                time.sleep(0.01)

                # Now the next 32 bits will be scanned
                lss_sub += 1

            # Now lss_id contains the entire 128 bits scanned
            return True, lss_id

        return False, None

    def __send_fast_scan_message(self, id_number, bit_checker, lss_sub, lss_next):
        message = bytearray(8)
        message[0:8] = struct.pack('<BIBBB', CS_FAST_SCAN, id_number, bit_checker, lss_sub, lss_next)
        try:
            recv_msg = self.__send_command(message)
        except LssError:
            return False

        cs = struct.unpack_from("<B", recv_msg)[0]
        if cs == CS_IDENTIFY_SLAVE:
                return True

        return False

    def __send_lss_address(self, req_cs, number):
        message = bytearray(8)

        message[0] = req_cs
        message[1:5] = struct.pack('<I', number)
        response = self.__send_command(message)
        # some device needs these delays between messages
        # because it can't handle messages arriving with no delay
        time.sleep(0.2)

        return response

    def __send_inquire_node_id(self):
        """
        :return:
            Current node id
        :rtype: int
        """
        message = bytearray(8)
        message[0] = CS_INQUIRE_NODE_ID
        response = self.__send_command(message)

        cs, current_node_id = struct.unpack_from("<BB", response)

        if cs != CS_INQUIRE_NODE_ID:
            raise LssError("Response message is not for the request")

        return current_node_id

    def __send_inquire_lss_address(self, req_cs):
        """
        :return:
            part of address. e.g., vendor ID or product code,  ..
        :rtype: int
        """
        message = bytearray(8)
        message[0] = req_cs
        response = self.__send_command(message)

        res_cs, part_of_address = struct.unpack_from("<BI", response)

        if res_cs != req_cs:
            raise LssError("Response message is not for the request")

        return part_of_address

    def __send_configure(self, req_cs, value1=0, value2=0):
        """Send a message to set a key with values"""
        message = bytearray(8)
        message[0] = req_cs
        message[1] = value1
        message[2] = value2
        response = self.__send_command(message)

        res_cs, error_code = struct.unpack_from("<BB", response)

        if res_cs != req_cs:
            raise LssError("Response message is not for the request")

        if error_code != ERROR_NONE:
            error_msg = f"LSS Error: {error_code}"
            raise LssError(error_msg)

    def __send_command(self, message):
        """Send a LSS operation code to the network

        :param bytearray message:
            LSS request message.

        :return:
            response
            None if there is no response
        :rtype: bytes
        """

        logger.info("Sending LSS message %s", message.hex(" ").upper())

        response = None
        if not self.responses.empty():
            logger.info("There were unexpected messages in the queue")
            self.responses = queue.Queue()

        self.network.send_message(self.LSS_TX_COBID, message)

        if not bool(message[0] in ListMessageNeedResponse):
            return response

        # Wait for the slave to respond
        # TODO check if the response is LSS response message
        try:
            response = self.responses.get(
                block=True, timeout=self.RESPONSE_TIMEOUT)
        except queue.Empty:
            raise LssError("No LSS response received")

        return response

    def on_message_received(self, can_id, data, timestamp):
        self.responses.put(bytes(data))


class LssError(Exception):
    """Some LSS operation failed."""
