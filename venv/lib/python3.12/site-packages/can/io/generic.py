"""Contains generic base classes for file IO."""

import gzip
import locale
from abc import ABCMeta
from types import TracebackType
from typing import (
    Any,
    BinaryIO,
    ContextManager,
    Iterable,
    Literal,
    Optional,
    TextIO,
    Type,
    Union,
    cast,
)

from typing_extensions import Self

from .. import typechecking
from ..listener import Listener
from ..message import Message


class BaseIOHandler(ContextManager, metaclass=ABCMeta):
    """A generic file handler that can be used for reading and writing.

    Can be used as a context manager.

    :attr file:
        the file-like object that is kept internally, or `None` if none
        was opened
    """

    file: Optional[typechecking.FileLike]

    def __init__(
        self,
        file: Optional[typechecking.AcceptedIOType],
        mode: str = "rt",
        **kwargs: Any,
    ) -> None:
        """
        :param file: a path-like object to open a file, a file-like object
                     to be used as a file or `None` to not use a file at all
        :param mode: the mode that should be used to open the file, see
                     :func:`open`, ignored if *file* is `None`
        """
        if file is None or (hasattr(file, "read") and hasattr(file, "write")):
            # file is None or some file-like object
            self.file = cast(Optional[typechecking.FileLike], file)
        else:
            encoding: Optional[str] = (
                None
                if "b" in mode
                else kwargs.get("encoding", locale.getpreferredencoding(False))
            )
            # pylint: disable=consider-using-with
            # file is some path-like object
            self.file = cast(
                typechecking.FileLike,
                open(cast(typechecking.StringPathLike, file), mode, encoding=encoding),
            )

        # for multiple inheritance
        super().__init__()

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_val: Optional[BaseException],
        exc_tb: Optional[TracebackType],
    ) -> Literal[False]:
        self.stop()
        return False

    def stop(self) -> None:
        """Closes the underlying file-like object and flushes it, if it was opened in write mode."""
        if self.file is not None:
            # this also implies a flush()
            self.file.close()


class MessageWriter(BaseIOHandler, Listener, metaclass=ABCMeta):
    """The base class for all writers."""

    file: Optional[typechecking.FileLike]


class FileIOMessageWriter(MessageWriter, metaclass=ABCMeta):
    """A specialized base class for all writers with file descriptors."""

    file: typechecking.FileLike

    def __init__(
        self, file: typechecking.AcceptedIOType, mode: str = "wt", **kwargs: Any
    ) -> None:
        # Not possible with the type signature, but be verbose for user-friendliness
        if file is None:
            raise ValueError("The given file cannot be None")

        super().__init__(file, mode, **kwargs)

    def file_size(self) -> int:
        """Return an estimate of the current file size in bytes."""
        return self.file.tell()


class TextIOMessageWriter(FileIOMessageWriter, metaclass=ABCMeta):
    file: TextIO


class BinaryIOMessageWriter(FileIOMessageWriter, metaclass=ABCMeta):
    file: Union[BinaryIO, gzip.GzipFile]


class MessageReader(BaseIOHandler, Iterable[Message], metaclass=ABCMeta):
    """The base class for all readers."""


class TextIOMessageReader(MessageReader, metaclass=ABCMeta):
    file: TextIO


class BinaryIOMessageReader(MessageReader, metaclass=ABCMeta):
    file: Union[BinaryIO, gzip.GzipFile]
