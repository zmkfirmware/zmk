"""Types for mypy type-checking
"""

import gzip
import struct
import sys
import typing

if sys.version_info >= (3, 10):
    from typing import TypeAlias
else:
    from typing_extensions import TypeAlias

if sys.version_info >= (3, 12):
    from typing import TypedDict
else:
    from typing_extensions import TypedDict


if typing.TYPE_CHECKING:
    import os


class CanFilter(TypedDict):
    can_id: int
    can_mask: int


class CanFilterExtended(TypedDict):
    can_id: int
    can_mask: int
    extended: bool


CanFilters = typing.Sequence[typing.Union[CanFilter, CanFilterExtended]]

# TODO: Once buffer protocol support lands in typing, we should switch to that,
# since can.message.Message attempts to call bytearray() on the given data, so
# this should have the same typing info.
#
# See: https://github.com/python/typing/issues/593
CanData = typing.Union[bytes, bytearray, int, typing.Iterable[int]]

# Used for the Abstract Base Class
ChannelStr = str
ChannelInt = int
Channel = typing.Union[ChannelInt, ChannelStr]

# Used by the IO module
FileLike = typing.Union[typing.TextIO, typing.BinaryIO, gzip.GzipFile]
StringPathLike = typing.Union[str, "os.PathLike[str]"]
AcceptedIOType = typing.Union[FileLike, StringPathLike]

BusConfig = typing.NewType("BusConfig", typing.Dict[str, typing.Any])

# Used by CLI scripts
TAdditionalCliArgs: TypeAlias = typing.Dict[str, typing.Union[str, int, float, bool]]
TDataStructs: TypeAlias = typing.Dict[
    typing.Union[int, typing.Tuple[int, ...]],
    typing.Union[struct.Struct, typing.Tuple, None],
]


class AutoDetectedConfig(TypedDict):
    interface: str
    channel: Channel


ReadableBytesLike = typing.Union[bytes, bytearray, memoryview]


class BitTimingDict(TypedDict):
    f_clock: int
    brp: int
    tseg1: int
    tseg2: int
    sjw: int
    nof_samples: int


class BitTimingFdDict(TypedDict):
    f_clock: int
    nom_brp: int
    nom_tseg1: int
    nom_tseg2: int
    nom_sjw: int
    data_brp: int
    data_tseg1: int
    data_tseg2: int
    data_sjw: int
