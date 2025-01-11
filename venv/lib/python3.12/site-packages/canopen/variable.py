import logging
from typing import Union
from collections.abc import Mapping

from canopen import objectdictionary
from canopen.utils import pretty_index

logger = logging.getLogger(__name__)


class Variable:

    def __init__(self, od: objectdictionary.ODVariable):
        self.od = od
        #: Description of this variable from Object Dictionary, overridable
        self.name = od.name
        if isinstance(od.parent, (objectdictionary.ODRecord,
                                  objectdictionary.ODArray)):
            # Include the parent object's name for subentries
            self.name = od.parent.name + "." + od.name
        #: Holds a local, overridable copy of the Object Index
        self.index = od.index
        #: Holds a local, overridable copy of the Object Subindex
        self.subindex = od.subindex

    def __repr__(self) -> str:
        subindex = self.subindex if isinstance(self.od.parent,
            (objectdictionary.ODRecord, objectdictionary.ODArray)
        ) else None
        return f"<{type(self).__qualname__} {self.name!r} at {pretty_index(self.index, subindex)}>"

    def get_data(self) -> bytes:
        raise NotImplementedError("Variable is not readable")

    def set_data(self, data: bytes):
        raise NotImplementedError("Variable is not writable")

    @property
    def data(self) -> bytes:
        """Byte representation of the object as :class:`bytes`."""
        return self.get_data()

    @data.setter
    def data(self, data: bytes):
        self.set_data(data)

    @property
    def raw(self) -> Union[int, bool, float, str, bytes]:
        """Raw representation of the object.

        This table lists the translations between object dictionary data types
        and Python native data types.

        +---------------------------+----------------------------+
        | Data type                 | Python type                |
        +===========================+============================+
        | BOOLEAN                   | :class:`bool`              |
        +---------------------------+----------------------------+
        | UNSIGNEDxx                | :class:`int`               |
        +---------------------------+----------------------------+
        | INTEGERxx                 | :class:`int`               |
        +---------------------------+----------------------------+
        | REALxx                    | :class:`float`             |
        +---------------------------+----------------------------+
        | VISIBLE_STRING            | :class:`str`               |
        +---------------------------+----------------------------+
        | UNICODE_STRING            | :class:`str`               |
        +---------------------------+----------------------------+
        | OCTET_STRING              | :class:`bytes`             |
        +---------------------------+----------------------------+
        | DOMAIN                    | :class:`bytes`             |
        +---------------------------+----------------------------+

        Data types that this library does not handle yet must be read and
        written as :class:`bytes`.
        """
        value = self.od.decode_raw(self.data)
        text = f"Value of {self.name!r} ({pretty_index(self.index, self.subindex)}) is {value!r}"
        if value in self.od.value_descriptions:
            text += f" ({self.od.value_descriptions[value]})"
        logger.debug(text)
        return value

    @raw.setter
    def raw(self, value: Union[int, bool, float, str, bytes]):
        logger.debug("Writing %r (0x%04X:%02X) = %r",
                     self.name, self.index,
                     self.subindex, value)
        self.data = self.od.encode_raw(value)

    @property
    def phys(self) -> Union[int, bool, float, str, bytes]:
        """Physical value scaled with some factor (defaults to 1).

        On object dictionaries that support specifying a factor, this can be
        either a :class:`float` or an :class:`int`.
        Non integers will be passed as is.
        """
        value = self.od.decode_phys(self.raw)
        if self.od.unit:
            logger.debug("Physical value is %s %s", value, self.od.unit)
        return value

    @phys.setter
    def phys(self, value: Union[int, bool, float, str, bytes]):
        self.raw = self.od.encode_phys(value)

    @property
    def desc(self) -> str:
        """Converts to and from a description of the value as a string."""
        value = self.od.decode_desc(self.raw)
        logger.debug("Description is '%s'", value)
        return value

    @desc.setter
    def desc(self, desc: str):
        self.raw = self.od.encode_desc(desc)

    @property
    def bits(self) -> "Bits":
        """Access bits using integers, slices, or bit descriptions."""
        return Bits(self)

    def read(self, fmt: str = "raw") -> Union[int, bool, float, str, bytes]:
        """Alternative way of reading using a function instead of attributes.

        May be useful for asynchronous reading.

        :param str fmt:
            How to return the value
             - 'raw'
             - 'phys'
             - 'desc'

        :returns:
            The value of the variable.
        """
        if fmt == "raw":
            return self.raw
        elif fmt == "phys":
            return self.phys
        elif fmt == "desc":
            return self.desc

    def write(
        self, value: Union[int, bool, float, str, bytes], fmt: str = "raw"
    ) -> None:
        """Alternative way of writing using a function instead of attributes.

        May be useful for asynchronous writing.

        :param str fmt:
            How to write the value
             - 'raw'
             - 'phys'
             - 'desc'
        """
        if fmt == "raw":
            self.raw = value
        elif fmt == "phys":
            self.phys = value
        elif fmt == "desc":
            self.desc = value


class Bits(Mapping):

    def __init__(self, variable: Variable):
        self.variable = variable
        self.read()

    @staticmethod
    def _get_bits(key):
        if isinstance(key, slice):
            bits = range(key.start, key.stop, key.step)
        elif isinstance(key, int):
            bits = [key]
        else:
            bits = key
        return bits

    def __getitem__(self, key) -> int:
        return self.variable.od.decode_bits(self.raw, self._get_bits(key))

    def __setitem__(self, key, value: int):
        self.raw = self.variable.od.encode_bits(
            self.raw, self._get_bits(key), value)
        self.write()

    def __iter__(self):
        return iter(self.variable.od.bit_definitions)

    def __len__(self):
        return len(self.variable.od.bit_definitions)

    def read(self):
        self.raw = self.variable.raw

    def write(self):
        self.variable.raw = self.raw
