"""
This Listener simply prints to stdout / the terminal or a file.
"""

import logging
from typing import Any, Optional, TextIO, Union, cast

from ..message import Message
from ..typechecking import StringPathLike
from .generic import MessageWriter

log = logging.getLogger("can.io.printer")


class Printer(MessageWriter):
    """
    The Printer class is a subclass of :class:`~can.Listener` which simply prints
    any messages it receives to the terminal (stdout). A message is turned into a
    string using :meth:`~can.Message.__str__`.

    :attr write_to_file: `True` if this instance prints to a file instead of
                         standard out
    """

    file: Optional[TextIO]

    def __init__(
        self,
        file: Optional[Union[StringPathLike, TextIO]] = None,
        append: bool = False,
        **kwargs: Any,
    ) -> None:
        """
        :param file: An optional path-like object or a file-like object to "print"
                     to instead of writing to standard out (stdout).
                     If this is a file-like object, it has to be opened in text
                     write mode, not binary write mode.
        :param append: If set to `True` messages, are appended to the file,
                       else the file is truncated
        """
        self.write_to_file = file is not None
        mode = "a" if append else "w"
        super().__init__(file, mode=mode)

    def on_message_received(self, msg: Message) -> None:
        if self.write_to_file:
            cast(TextIO, self.file).write(str(msg) + "\n")
        else:
            print(msg)  # noqa: T201

    def file_size(self) -> int:
        """Return an estimate of the current file size in bytes."""
        if self.file is not None:
            return self.file.tell()
        return 0
