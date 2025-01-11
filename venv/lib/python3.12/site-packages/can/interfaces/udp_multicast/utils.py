"""
Defines common functions.
"""

from typing import Any, Dict, Optional

from can import CanInterfaceNotImplementedError, Message
from can.typechecking import ReadableBytesLike

try:
    import msgpack
except ImportError:
    msgpack = None


def check_msgpack_installed() -> None:
    """Raises a :class:`can.CanInterfaceNotImplementedError` if `msgpack` is not installed."""
    if msgpack is None:
        raise CanInterfaceNotImplementedError("msgpack not installed")


def pack_message(message: Message) -> bytes:
    """
    Pack a can.Message into a msgpack byte blob.

    :param message: the message to be packed
    """
    check_msgpack_installed()
    as_dict = {
        "timestamp": message.timestamp,
        "arbitration_id": message.arbitration_id,
        "is_extended_id": message.is_extended_id,
        "is_remote_frame": message.is_remote_frame,
        "is_error_frame": message.is_error_frame,
        "channel": message.channel,
        "dlc": message.dlc,
        "data": message.data,
        "is_fd": message.is_fd,
        "bitrate_switch": message.bitrate_switch,
        "error_state_indicator": message.error_state_indicator,
    }
    return msgpack.packb(as_dict, use_bin_type=True)


def unpack_message(
    data: ReadableBytesLike,
    replace: Optional[Dict[str, Any]] = None,
    check: bool = False,
) -> Message:
    """Unpack a can.Message from a msgpack byte blob.

    :param data: the raw data
    :param replace: a mapping from field names to values to be replaced after decoding the new message, or
                    `None` to disable this feature
    :param check: this is passed to :meth:`can.Message.__init__` to specify whether to validate the message

    :raise TypeError: if the data contains key that are not valid arguments for :meth:`can.Message.__init__`
    :raise ValueError: if `check` is true and the message metadata is invalid in some way
    :raise Exception: if there was another problem while unpacking
    """
    check_msgpack_installed()
    as_dict = msgpack.unpackb(data, raw=False)
    if replace is not None:
        as_dict.update(replace)
    return Message(check=check, **as_dict)
