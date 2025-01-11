import threading
import logging
import struct
import time
from typing import Callable, Optional

logger = logging.getLogger(__name__)

NMT_STATES = {
    0: 'INITIALISING',
    4: 'STOPPED',
    5: 'OPERATIONAL',
    80: 'SLEEP',
    96: 'STANDBY',
    127: 'PRE-OPERATIONAL'
}

NMT_COMMANDS = {
    'OPERATIONAL': 1,
    'STOPPED': 2,
    'SLEEP': 80,
    'STANDBY': 96,
    'PRE-OPERATIONAL': 128,
    'INITIALISING': 129,
    'RESET': 129,
    'RESET COMMUNICATION': 130
}

COMMAND_TO_STATE = {
    1: 5,
    2: 4,
    80: 80,
    96: 96,
    128: 127,
    129: 0,
    130: 0
}


class NmtBase:
    """
    Can set the state of the node it controls using NMT commands and monitor
    the current state using the heartbeat protocol.
    """

    def __init__(self, node_id: int):
        self.id = node_id
        self.network = None
        self._state = 0

    def on_command(self, can_id, data, timestamp):
        cmd, node_id = struct.unpack_from("BB", data)
        if node_id in (self.id, 0):
            logger.info("Node %d received command %d", self.id, cmd)
            if cmd in COMMAND_TO_STATE:
                new_state = COMMAND_TO_STATE[cmd]
                if new_state != self._state:
                    logger.info("New NMT state %s, old state %s",
                                NMT_STATES[new_state], NMT_STATES[self._state])
                self._state = new_state

    def send_command(self, code: int):
        """Send an NMT command code to the node.

        :param code:
            NMT command code.
        """
        if code in COMMAND_TO_STATE:
            new_state = COMMAND_TO_STATE[code]
            logger.info("Changing NMT state on node %d from %s to %s",
                        self.id, NMT_STATES[self._state], NMT_STATES[new_state])
            self._state = new_state

    @property
    def state(self) -> str:
        """Attribute to get or set node's state as a string.

        Can be one of:

        - 'INITIALISING'
        - 'PRE-OPERATIONAL'
        - 'STOPPED'
        - 'OPERATIONAL'
        - 'SLEEP'
        - 'STANDBY'
        - 'RESET'
        - 'RESET COMMUNICATION'
        """
        if self._state in NMT_STATES:
            return NMT_STATES[self._state]
        else:
            return self._state

    @state.setter
    def state(self, new_state: str):
        if new_state in NMT_COMMANDS:
            code = NMT_COMMANDS[new_state]
        else:
            raise ValueError("'%s' is an invalid state. Must be one of %s." %
                             (new_state, ", ".join(NMT_COMMANDS)))

        self.send_command(code)


class NmtMaster(NmtBase):

    def __init__(self, node_id: int):
        super(NmtMaster, self).__init__(node_id)
        self._state_received = None
        self._node_guarding_producer = None
        #: Timestamp of last heartbeat message
        self.timestamp: Optional[float] = None
        self.state_update = threading.Condition()
        self._callbacks = []

    def on_heartbeat(self, can_id, data, timestamp):
        with self.state_update:
            self.timestamp = timestamp
            new_state, = struct.unpack_from("B", data)
            # Mask out toggle bit
            new_state &= 0x7F
            logger.debug("Received heartbeat can-id %d, state is %d", can_id, new_state)
            for callback in self._callbacks:
                callback(new_state)
            if new_state == 0:
                # Boot-up, will go to PRE-OPERATIONAL automatically
                self._state = 127
            else:
                self._state = new_state
            self._state_received = new_state
            self.state_update.notify_all()

    def send_command(self, code: int):
        """Send an NMT command code to the node.

        :param code:
            NMT command code.
        """
        super(NmtMaster, self).send_command(code)
        logger.info(
            "Sending NMT command 0x%X to node %d", code, self.id)
        self.network.send_message(0, [code, self.id])

    def wait_for_heartbeat(self, timeout: float = 10):
        """Wait until a heartbeat message is received."""
        with self.state_update:
            self._state_received = None
            self.state_update.wait(timeout)
        if self._state_received is None:
            raise NmtError("No boot-up or heartbeat received")
        return self.state

    def wait_for_bootup(self, timeout: float = 10) -> None:
        """Wait until a boot-up message is received."""
        end_time = time.time() + timeout
        while True:
            now = time.time()
            with self.state_update:
                self._state_received = None
                self.state_update.wait(end_time - now + 0.1)
            if now > end_time:
                raise NmtError("Timeout waiting for boot-up message")
            if self._state_received == 0:
                break

    def add_heartbeat_callback(self, callback: Callable[[int], None]):
        """Add function to be called on heartbeat reception.

        :param callback:
            Function that should accept an NMT state as only argument.
        """
        self._callbacks.append(callback)

    # Compatibility with previous typo
    add_hearbeat_callback = add_heartbeat_callback

    def start_node_guarding(self, period: float):
        """Starts the node guarding mechanism.

        :param period:
            Period (in seconds) at which the node guarding should be advertised to the slave node.
        """
        if self._node_guarding_producer : self.stop_node_guarding()
        self._node_guarding_producer = self.network.send_periodic(0x700 + self.id, None, period, True)

    def stop_node_guarding(self):
        """Stops the node guarding mechanism."""
        if self._node_guarding_producer is not None:
            self._node_guarding_producer.stop()
            self._node_guarding_producer = None


class NmtSlave(NmtBase):
    """
    Handles the NMT state and handles heartbeat NMT service.
    """

    def __init__(self, node_id: int, local_node):
        super(NmtSlave, self).__init__(node_id)
        self._send_task = None
        self._heartbeat_time_ms = 0
        self._local_node = local_node

    def on_command(self, can_id, data, timestamp):
        super(NmtSlave, self).on_command(can_id, data, timestamp)
        self.update_heartbeat()

    def send_command(self, code: int) -> None:
        """Send an NMT command code to the node.

        :param code:
            NMT command code.
        """
        old_state = self._state
        super(NmtSlave, self).send_command(code)

        if self._state == 0:
            logger.info("Sending boot-up message")
            self.network.send_message(0x700 + self.id, [0])

        # The heartbeat service should start on the transition
        # between INITIALIZING and PRE-OPERATIONAL state
        if old_state == 0 and self._state == 127:
            heartbeat_time_ms = self._local_node.sdo[0x1017].raw
            self.start_heartbeat(heartbeat_time_ms)
        else:
            self.update_heartbeat()

    def on_write(self, index, data, **kwargs):
        if index == 0x1017:
            heartbeat_time, = struct.unpack_from("<H", data)
            if heartbeat_time == 0:
                self.stop_heartbeat()
            else:
                self.start_heartbeat(heartbeat_time)

    def start_heartbeat(self, heartbeat_time_ms: int):
        """Start the heartbeat service.

        :param heartbeat_time_ms
            The heartbeat time in ms. If the heartbeat time is 0
            the heartbeating will not start.
        """
        self._heartbeat_time_ms = heartbeat_time_ms

        self.stop_heartbeat()
        if heartbeat_time_ms > 0:
            logger.info("Start the heartbeat timer, interval is %d ms", self._heartbeat_time_ms)
            self._send_task = self.network.send_periodic(
                0x700 + self.id, [self._state], heartbeat_time_ms / 1000.0)

    def stop_heartbeat(self):
        """Stop the heartbeat service."""
        if self._send_task is not None:
            logger.info("Stop the heartbeat timer")
            self._send_task.stop()
            self._send_task = None

    def update_heartbeat(self):
        if self._send_task is not None:
            self._send_task.update([self._state])


class NmtError(Exception):
    """Some NMT operation failed."""
