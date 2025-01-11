import struct
import logging
import threading
import time
from typing import Callable, List, Optional

# Error code, error register, vendor specific data
EMCY_STRUCT = struct.Struct("<HB5s")

logger = logging.getLogger(__name__)


class EmcyConsumer:

    def __init__(self):
        #: Log of all received EMCYs for this node
        self.log: List["EmcyError"] = []
        #: Only active EMCYs. Will be cleared on Error Reset
        self.active: List["EmcyError"] = []
        self.callbacks = []
        self.emcy_received = threading.Condition()

    def on_emcy(self, can_id, data, timestamp):
        code, register, data = EMCY_STRUCT.unpack(data)
        entry = EmcyError(code, register, data, timestamp)

        with self.emcy_received:
            if code & 0xFF00 == 0:
                # Error reset
                self.active = []
            else:
                self.active.append(entry)
            self.log.append(entry)
            self.emcy_received.notify_all()

        for callback in self.callbacks:
            callback(entry)

    def add_callback(self, callback: Callable[["EmcyError"], None]):
        """Get notified on EMCY messages from this node.

        :param callback:
            Callable which must take one argument of an
            :class:`~canopen.emcy.EmcyError` instance.
        """
        self.callbacks.append(callback)

    def reset(self):
        """Reset log and active lists."""
        self.log = []
        self.active = []

    def wait(
        self, emcy_code: Optional[int] = None, timeout: float = 10
    ) -> "EmcyError":
        """Wait for a new EMCY to arrive.

        :param emcy_code: EMCY code to wait for
        :param timeout: Max time in seconds to wait

        :return: The EMCY exception object or None if timeout
        """
        end_time = time.time() + timeout
        while True:
            with self.emcy_received:
                prev_log_size = len(self.log)
                self.emcy_received.wait(timeout)
                if len(self.log) == prev_log_size:
                    # Resumed due to timeout
                    return None
                # Get last logged EMCY
                emcy = self.log[-1]
                logger.info("Got %s", emcy)
                if time.time() > end_time:
                    # No valid EMCY received on time
                    return None
                if emcy_code is None or emcy.code == emcy_code:
                    # This is the one we're interested in
                    return emcy


class EmcyProducer:

    def __init__(self, cob_id: int):
        self.network = None
        self.cob_id = cob_id

    def send(self, code: int, register: int = 0, data: bytes = b""):
        payload = EMCY_STRUCT.pack(code, register, data)
        self.network.send_message(self.cob_id, payload)

    def reset(self, register: int = 0, data: bytes = b""):
        payload = EMCY_STRUCT.pack(0, register, data)
        self.network.send_message(self.cob_id, payload)


class EmcyError(Exception):
    """EMCY exception."""

    DESCRIPTIONS = [
        # Code   Mask    Description
        (0x0000, 0xFF00, "Error Reset / No Error"),
        (0x1000, 0xFF00, "Generic Error"),
        (0x2000, 0xF000, "Current"),
        (0x3000, 0xF000, "Voltage"),
        (0x4000, 0xF000, "Temperature"),
        (0x5000, 0xFF00, "Device Hardware"),
        (0x6000, 0xF000, "Device Software"),
        (0x7000, 0xFF00, "Additional Modules"),
        (0x8000, 0xF000, "Monitoring"),
        (0x9000, 0xFF00, "External Error"),
        (0xF000, 0xFF00, "Additional Functions"),
        (0xFF00, 0xFF00, "Device Specific")
    ]

    def __init__(self, code: int, register: int, data: bytes, timestamp: float):
        #: EMCY code
        self.code = code
        #: Error register
        self.register = register
        #: Vendor specific data
        self.data = data
        #: Timestamp of message
        self.timestamp = timestamp

    def get_desc(self) -> str:
        for code, mask, description in self.DESCRIPTIONS:
            if self.code & mask == code:
                return description
        return ""

    def __str__(self):
        text = f"Code 0x{self.code:04X}"
        description = self.get_desc()
        if description:
            text = text + ", " + description
        return text
