"""
This module contains the implementation of :class:`~can.Notifier`.
"""

import asyncio
import functools
import logging
import threading
import time
from typing import Any, Awaitable, Callable, Iterable, List, Optional, Union

from can.bus import BusABC
from can.listener import Listener
from can.message import Message

logger = logging.getLogger("can.Notifier")

MessageRecipient = Union[Listener, Callable[[Message], Union[Awaitable[None], None]]]


class Notifier:
    def __init__(
        self,
        bus: Union[BusABC, List[BusABC]],
        listeners: Iterable[MessageRecipient],
        timeout: float = 1.0,
        loop: Optional[asyncio.AbstractEventLoop] = None,
    ) -> None:
        """Manages the distribution of :class:`~can.Message` instances to listeners.

        Supports multiple buses and listeners.

        .. Note::

            Remember to call `stop()` after all messages are received as
            many listeners carry out flush operations to persist data.


        :param bus: A :ref:`bus` or a list of buses to listen to.
        :param listeners:
            An iterable of :class:`~can.Listener` or callables that receive a :class:`~can.Message`
            and return nothing.
        :param timeout: An optional maximum number of seconds to wait for any :class:`~can.Message`.
        :param loop: An :mod:`asyncio` event loop to schedule the ``listeners`` in.
        """
        self.listeners: List[MessageRecipient] = list(listeners)
        self.bus = bus
        self.timeout = timeout
        self._loop = loop

        #: Exception raised in thread
        self.exception: Optional[Exception] = None

        self._running = True
        self._lock = threading.Lock()

        self._readers: List[Union[int, threading.Thread]] = []
        buses = self.bus if isinstance(self.bus, list) else [self.bus]
        for each_bus in buses:
            self.add_bus(each_bus)

    def add_bus(self, bus: BusABC) -> None:
        """Add a bus for notification.

        :param bus:
            CAN bus instance.
        """
        reader: int = -1
        try:
            reader = bus.fileno()
        except NotImplementedError:
            # Bus doesn't support fileno, we fall back to thread based reader
            pass

        if self._loop is not None and reader >= 0:
            # Use bus file descriptor to watch for messages
            self._loop.add_reader(reader, self._on_message_available, bus)
            self._readers.append(reader)
        else:
            reader_thread = threading.Thread(
                target=self._rx_thread,
                args=(bus,),
                name=f'can.notifier for bus "{bus.channel_info}"',
            )
            reader_thread.daemon = True
            reader_thread.start()
            self._readers.append(reader_thread)

    def stop(self, timeout: float = 5) -> None:
        """Stop notifying Listeners when new :class:`~can.Message` objects arrive
        and call :meth:`~can.Listener.stop` on each Listener.

        :param timeout:
            Max time in seconds to wait for receive threads to finish.
            Should be longer than timeout given at instantiation.
        """
        self._running = False
        end_time = time.time() + timeout
        for reader in self._readers:
            if isinstance(reader, threading.Thread):
                now = time.time()
                if now < end_time:
                    reader.join(end_time - now)
            elif self._loop:
                # reader is a file descriptor
                self._loop.remove_reader(reader)
        for listener in self.listeners:
            if hasattr(listener, "stop"):
                listener.stop()

    def _rx_thread(self, bus: BusABC) -> None:
        # determine message handling callable early, not inside while loop
        if self._loop:
            handle_message: Callable[[Message], Any] = functools.partial(
                self._loop.call_soon_threadsafe,
                self._on_message_received,  # type: ignore[arg-type]
            )
        else:
            handle_message = self._on_message_received

        while self._running:
            try:
                if msg := bus.recv(self.timeout):
                    with self._lock:
                        handle_message(msg)
            except Exception as exc:  # pylint: disable=broad-except
                self.exception = exc
                if self._loop is not None:
                    self._loop.call_soon_threadsafe(self._on_error, exc)
                    # Raise anyway
                    raise
                elif not self._on_error(exc):
                    # If it was not handled, raise the exception here
                    raise
                else:
                    # It was handled, so only log it
                    logger.debug("suppressed exception: %s", exc)

    def _on_message_available(self, bus: BusABC) -> None:
        if msg := bus.recv(0):
            self._on_message_received(msg)

    def _on_message_received(self, msg: Message) -> None:
        for callback in self.listeners:
            res = callback(msg)
            if res and self._loop and asyncio.iscoroutine(res):
                # Schedule coroutine
                self._loop.create_task(res)

    def _on_error(self, exc: Exception) -> bool:
        """Calls ``on_error()`` for all listeners if they implement it.

        :returns: ``True`` if at least one error handler was called.
        """
        was_handled = False

        for listener in self.listeners:
            if hasattr(listener, "on_error"):
                try:
                    listener.on_error(exc)
                except NotImplementedError:
                    pass
                else:
                    was_handled = True

        return was_handled

    def add_listener(self, listener: MessageRecipient) -> None:
        """Add new Listener to the notification list.
        If it is already present, it will be called two times
        each time a message arrives.

        :param listener: Listener to be added to the list to be notified
        """
        self.listeners.append(listener)

    def remove_listener(self, listener: MessageRecipient) -> None:
        """Remove a listener from the notification list. This method
        throws an exception if the given listener is not part of the
        stored listeners.

        :param listener: Listener to be removed from the list to be notified
        :raises ValueError: if `listener` was never added to this notifier
        """
        self.listeners.remove(listener)
