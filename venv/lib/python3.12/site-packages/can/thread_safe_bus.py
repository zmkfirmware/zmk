from threading import RLock

try:
    # Only raise an exception on instantiation but allow module
    # to be imported
    from wrapt import ObjectProxy

    import_exc = None
except ImportError as exc:
    ObjectProxy = object
    import_exc = exc

from contextlib import nullcontext

from .interface import Bus


class ThreadSafeBus(ObjectProxy):  # pylint: disable=abstract-method
    """
    Contains a thread safe :class:`can.BusABC` implementation that
    wraps around an existing interface instance. All public methods
    of that base class are now safe to be called from multiple threads.
    The send and receive methods are synchronized separately.

    Use this as a drop-in replacement for :class:`~can.BusABC`.

    .. note::

        This approach assumes that both :meth:`~can.BusABC.send` and
        :meth:`~can.BusABC._recv_internal` of the underlying bus instance can be
        called simultaneously, and that the methods use :meth:`~can.BusABC._recv_internal`
        instead of :meth:`~can.BusABC.recv` directly.
    """

    def __init__(self, *args, **kwargs):
        if import_exc is not None:
            raise import_exc

        super().__init__(Bus(*args, **kwargs))

        # now, BusABC.send_periodic() does not need a lock anymore, but the
        # implementation still requires a context manager
        self.__wrapped__._lock_send_periodic = nullcontext()

        # init locks for sending and receiving separately
        self._lock_send = RLock()
        self._lock_recv = RLock()

    def recv(
        self, timeout=None, *args, **kwargs
    ):  # pylint: disable=keyword-arg-before-vararg
        with self._lock_recv:
            return self.__wrapped__.recv(timeout=timeout, *args, **kwargs)

    def send(
        self, msg, timeout=None, *args, **kwargs
    ):  # pylint: disable=keyword-arg-before-vararg
        with self._lock_send:
            return self.__wrapped__.send(msg, timeout=timeout, *args, **kwargs)

    # send_periodic does not need a lock, since the underlying
    # `send` method is already synchronized

    @property
    def filters(self):
        with self._lock_recv:
            return self.__wrapped__.filters

    @filters.setter
    def filters(self, filters):
        with self._lock_recv:
            self.__wrapped__.filters = filters

    def set_filters(
        self, filters=None, *args, **kwargs
    ):  # pylint: disable=keyword-arg-before-vararg
        with self._lock_recv:
            return self.__wrapped__.set_filters(filters=filters, *args, **kwargs)

    def flush_tx_buffer(self, *args, **kwargs):
        with self._lock_send:
            return self.__wrapped__.flush_tx_buffer(*args, **kwargs)

    def shutdown(self, *args, **kwargs):
        with self._lock_send, self._lock_recv:
            return self.__wrapped__.shutdown(*args, **kwargs)

    @property
    def state(self):
        with self._lock_send, self._lock_recv:
            return self.__wrapped__.state

    @state.setter
    def state(self, new_state):
        with self._lock_send, self._lock_recv:
            self.__wrapped__.state = new_state
