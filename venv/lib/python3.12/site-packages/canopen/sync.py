from typing import Optional


class SyncProducer:
    """Transmits a SYNC message periodically."""

    #: COB-ID of the SYNC message
    cob_id = 0x80

    def __init__(self, network):
        self.network = network
        self.period: Optional[float] = None
        self._task = None

    def transmit(self, count: Optional[int] = None):
        """Send out a SYNC message once.

        :param count:
            Counter to add in message.
        """
        data = [count] if count is not None else []
        self.network.send_message(self.cob_id, data)

    def start(self, period: Optional[float] = None):
        """Start periodic transmission of SYNC message in a background thread.

        :param period:
            Period of SYNC message in seconds.
        """
        if period is not None:
            self.period = period

        if not self.period:
            raise ValueError("A valid transmission period has not been given")

        self._task = self.network.send_periodic(self.cob_id, [], self.period)

    def stop(self):
        """Stop periodic transmission of SYNC message."""
        if self._task is not None:
            self._task.stop()
