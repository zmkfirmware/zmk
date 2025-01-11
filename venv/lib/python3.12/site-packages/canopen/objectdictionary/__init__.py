"""
Object Dictionary module
"""
from __future__ import annotations

import struct
from typing import Dict, Iterator, List, Optional, TextIO, Union
from collections.abc import MutableMapping, Mapping
import logging

from canopen.objectdictionary.datatypes import *
from canopen.utils import pretty_index

logger = logging.getLogger(__name__)


def export_od(od, dest: Union[str, TextIO, None] = None, doc_type: Optional[str] = None):
    """ Export :class: ObjectDictionary to a file.

    :param od:
        :class: ObjectDictionary object to be exported
    :param dest:
        export destination. filename, or file-like object or None.
        if None, the document is returned as string
    :param doc_type: type of document to export.
       If a filename is given for dest, this default to the file extension.
       Otherwise, this defaults to "eds"
    :rtype: str or None
    """

    doctypes = {"eds", "dcf"}
    if isinstance(dest, str):
        if doc_type is None:
            for t in doctypes:
                if dest.endswith(f".{t}"):
                    doc_type = t
                    break

        if doc_type is None:
            doc_type = "eds"
        dest = open(dest, 'w')
    assert doc_type in doctypes

    if doc_type == "eds":
        from canopen.objectdictionary import eds
        return eds.export_eds(od, dest)
    elif doc_type == "dcf":
        from canopen.objectdictionary import eds
        return eds.export_dcf(od, dest)

    # If dest is opened in this fn, it should be closed
    if type(dest) is str:
        dest.close()


def import_od(
    source: Union[str, TextIO, None],
    node_id: Optional[int] = None,
) -> ObjectDictionary:
    """Parse an EDS, DCF, or EPF file.

    :param source:
        Path to object dictionary file or a file like object or an EPF XML tree.

    :return:
        An Object Dictionary instance.
    """
    if source is None:
        return ObjectDictionary()
    if hasattr(source, "read"):
        # File like object
        filename = source.name
    elif hasattr(source, "tag"):
        # XML tree, probably from an EPF file
        filename = "od.epf"
    else:
        # Path to file
        filename = source
    suffix = filename[filename.rfind("."):].lower()
    if suffix in (".eds", ".dcf"):
        from canopen.objectdictionary import eds
        return eds.import_eds(source, node_id)
    elif suffix == ".epf":
        from canopen.objectdictionary import epf
        return epf.import_epf(source)
    else:
        raise NotImplementedError("No support for this format")


class ObjectDictionary(MutableMapping):
    """Representation of the object dictionary as a Python dictionary."""

    def __init__(self):
        self.indices = {}
        self.names = {}
        self.comments = ""
        #: Default bitrate if specified by file
        self.bitrate: Optional[int] = None
        #: Node ID if specified by file
        self.node_id: Optional[int] = None
        #: Some information about the device
        self.device_information = DeviceInformation()

    def __getitem__(
        self, index: Union[int, str]
    ) -> Union[ODArray, ODRecord, ODVariable]:
        """Get object from object dictionary by name or index."""
        item = self.names.get(index) or self.indices.get(index)
        if item is None:
            if isinstance(index, str) and '.' in index:
                idx, sub = index.split('.', maxsplit=1)
                return self[idx][sub]
            raise KeyError(f"{pretty_index(index)} was not found in Object Dictionary")
        return item

    def __setitem__(
        self, index: Union[int, str], obj: Union[ODArray, ODRecord, ODVariable]
    ):
        assert index == obj.index or index == obj.name
        self.add_object(obj)

    def __delitem__(self, index: Union[int, str]):
        obj = self[index]
        del self.indices[obj.index]
        del self.names[obj.name]

    def __iter__(self) -> Iterator[int]:
        return iter(sorted(self.indices))

    def __len__(self) -> int:
        return len(self.indices)

    def __contains__(self, index: Union[int, str]):
        return index in self.names or index in self.indices

    def add_object(self, obj: Union[ODArray, ODRecord, ODVariable]) -> None:
        """Add object to the object dictionary.

        :param obj:
            Should be either one of
            :class:`~canopen.objectdictionary.ODVariable`,
            :class:`~canopen.objectdictionary.ODRecord`, or
            :class:`~canopen.objectdictionary.ODArray`.
        """
        obj.parent = self
        self.indices[obj.index] = obj
        self.names[obj.name] = obj

    def get_variable(
        self, index: Union[int, str], subindex: int = 0
    ) -> Optional[ODVariable]:
        """Get the variable object at specified index (and subindex if applicable).

        :return: ODVariable if found, else `None`
        """
        obj = self.get(index)
        if isinstance(obj, ODVariable):
            return obj
        elif isinstance(obj, (ODRecord, ODArray)):
            return obj.get(subindex)


class ODRecord(MutableMapping):
    """Groups multiple :class:`~canopen.objectdictionary.ODVariable` objects using
    subindices.
    """

    #: Description for the whole record
    description = ""

    def __init__(self, name: str, index: int):
        #: The :class:`~canopen.ObjectDictionary` owning the record.
        self.parent: Optional[ObjectDictionary] = None
        #: 16-bit address of the record
        self.index = index
        #: Name of record
        self.name = name
        #: Storage location of index
        self.storage_location = None
        self.subindices = {}
        self.names = {}

    def __repr__(self) -> str:
        return f"<{type(self).__qualname__} {self.name!r} at {pretty_index(self.index)}>"

    def __getitem__(self, subindex: Union[int, str]) -> ODVariable:
        item = self.names.get(subindex) or self.subindices.get(subindex)
        if item is None:
            raise KeyError(f"Subindex {pretty_index(None, subindex)} was not found")
        return item

    def __setitem__(self, subindex: Union[int, str], var: ODVariable):
        assert subindex == var.subindex
        self.add_member(var)

    def __delitem__(self, subindex: Union[int, str]):
        var = self[subindex]
        del self.subindices[var.subindex]
        del self.names[var.name]

    def __len__(self) -> int:
        return len(self.subindices)

    def __iter__(self) -> Iterator[int]:
        return iter(sorted(self.subindices))

    def __contains__(self, subindex: Union[int, str]) -> bool:
        return subindex in self.names or subindex in self.subindices

    def __eq__(self, other: ODRecord) -> bool:
        return self.index == other.index

    def add_member(self, variable: ODVariable) -> None:
        """Adds a :class:`~canopen.objectdictionary.ODVariable` to the record."""
        variable.parent = self
        self.subindices[variable.subindex] = variable
        self.names[variable.name] = variable


class ODArray(Mapping):
    """An array of :class:`~canopen.objectdictionary.ODVariable` objects using
    subindices.

    Actual length of array must be read from the node using SDO.
    """

    #: Description for the whole array
    description = ""

    def __init__(self, name: str, index: int):
        #: The :class:`~canopen.ObjectDictionary` owning the record.
        self.parent = None
        #: 16-bit address of the array
        self.index = index
        #: Name of array
        self.name = name
        #: Storage location of index
        self.storage_location = None
        self.subindices = {}
        self.names = {}

    def __repr__(self) -> str:
        return f"<{type(self).__qualname__} {self.name!r} at {pretty_index(self.index)}>"

    def __getitem__(self, subindex: Union[int, str]) -> ODVariable:
        var = self.names.get(subindex) or self.subindices.get(subindex)
        if var is not None:
            # This subindex is defined
            pass
        elif isinstance(subindex, int) and 0 < subindex < 256:
            # Create a new variable based on first array item
            template = self.subindices[1]
            name = f"{template.name}_{subindex:x}"
            var = ODVariable(name, self.index, subindex)
            var.parent = self
            for attr in ("data_type", "unit", "factor", "min", "max", "default",
                         "access_type", "description", "value_descriptions",
                         "bit_definitions", "storage_location"):
                if attr in template.__dict__:
                    var.__dict__[attr] = template.__dict__[attr]
        else:
            raise KeyError(f"Could not find subindex {pretty_index(None, subindex)}")
        return var

    def __len__(self) -> int:
        return len(self.subindices)

    def __iter__(self) -> Iterator[int]:
        return iter(sorted(self.subindices))

    def __eq__(self, other: ODArray) -> bool:
        return self.index == other.index

    def add_member(self, variable: ODVariable) -> None:
        """Adds a :class:`~canopen.objectdictionary.ODVariable` to the record."""
        variable.parent = self
        self.subindices[variable.subindex] = variable
        self.names[variable.name] = variable


class ODVariable:
    """Simple variable."""

    STRUCT_TYPES: dict[int, struct.Struct] = {
        # Use struct module to pack/unpack data where possible and use the
        # custom IntegerN and UnsignedN classes for the special data types.
        BOOLEAN: struct.Struct("?"),
        INTEGER8: struct.Struct("b"),
        INTEGER16: struct.Struct("<h"),
        INTEGER24: IntegerN(24),
        INTEGER32: struct.Struct("<l"),
        INTEGER40: IntegerN(40),
        INTEGER48: IntegerN(48),
        INTEGER56: IntegerN(56),
        INTEGER64: struct.Struct("<q"),
        UNSIGNED8: struct.Struct("B"),
        UNSIGNED16: struct.Struct("<H"),
        UNSIGNED24: UnsignedN(24),
        UNSIGNED32: struct.Struct("<L"),
        UNSIGNED40: UnsignedN(40),
        UNSIGNED48: UnsignedN(48),
        UNSIGNED56: UnsignedN(56),
        UNSIGNED64: struct.Struct("<Q"),
        REAL32: struct.Struct("<f"),
        REAL64: struct.Struct("<d")
    }

    def __init__(self, name: str, index: int, subindex: int = 0):
        #: The :class:`~canopen.ObjectDictionary`,
        #: :class:`~canopen.objectdictionary.ODRecord` or
        #: :class:`~canopen.objectdictionary.ODArray` owning the variable
        self.parent = None
        #: 16-bit address of the object in the dictionary
        self.index = index
        #: 8-bit sub-index of the object in the dictionary
        self.subindex = subindex
        #: String representation of the variable
        self.name = name
        #: Physical unit
        self.unit: str = ""
        #: Factor between physical unit and integer value
        self.factor: float = 1
        #: Minimum allowed value
        self.min: Optional[int] = None
        #: Maximum allowed value
        self.max: Optional[int] = None
        #: Default value at start-up
        self.default: Optional[int] = None
        #: Is the default value relative to the node-ID (only applies to COB-IDs)
        self.relative = False
        #: The value of this variable stored in the object dictionary
        self.value: Optional[int] = None
        #: Data type according to the standard as an :class:`int`
        self.data_type: Optional[int] = None
        #: Access type, should be "rw", "ro", "wo", or "const"
        self.access_type: str = "rw"
        #: Description of variable
        self.description: str = ""
        #: Dictionary of value descriptions
        self.value_descriptions: Dict[int, str] = {}
        #: Dictionary of bitfield definitions
        self.bit_definitions: Dict[str, List[int]] = {}
        #: Storage location of index
        self.storage_location = None
        #: Can this variable be mapped to a PDO
        self.pdo_mappable = False

    def __repr__(self) -> str:
        subindex = self.subindex if isinstance(self.parent, (ODRecord, ODArray)) else None
        return f"<{type(self).__qualname__} {self.qualname!r} at {pretty_index(self.index, subindex)}>"

    @property
    def qualname(self) -> str:
        """Fully qualified name of the variable. If the variable is a subindex
        of a record or array, the name will be prefixed with the parent's name."""
        if isinstance(self.parent, (ODRecord, ODArray)):
            return f"{self.parent.name}.{self.name}"
        return self.name

    def __eq__(self, other: ODVariable) -> bool:
        return (self.index == other.index and
                self.subindex == other.subindex)

    def __len__(self) -> int:
        if self.data_type in self.STRUCT_TYPES:
            return self.STRUCT_TYPES[self.data_type].size * 8
        else:
            return 8

    @property
    def writable(self) -> bool:
        return "w" in self.access_type

    @property
    def readable(self) -> bool:
        return "r" in self.access_type or self.access_type == "const"

    def add_value_description(self, value: int, descr: str) -> None:
        """Associate a value with a string description.

        :param value: Value to describe
        :param desc: Description of value
        """
        self.value_descriptions[value] = descr

    def add_bit_definition(self, name: str, bits: List[int]) -> None:
        """Associate bit(s) with a string description.

        :param name: Name of bit(s)
        :param bits: List of bits as integers
        """
        self.bit_definitions[name] = bits

    def decode_raw(self, data: bytes) -> Union[int, float, str, bytes, bytearray]:
        if self.data_type == VISIBLE_STRING:
            # Strip any trailing NUL characters from C-based systems
            return data.decode("ascii", errors="ignore").rstrip("\x00")
        elif self.data_type == UNICODE_STRING:
            # The CANopen standard does not specify the encoding. This
            # library assumes UTF-16, being the most common two-byte encoding format.
            # Strip any trailing NUL characters from C-based systems
            return data.decode("utf_16_le", errors="ignore").rstrip("\x00")
        elif self.data_type in self.STRUCT_TYPES:
            try:
                value, = self.STRUCT_TYPES[self.data_type].unpack(data)
                return value
            except struct.error:
                raise ObjectDictionaryError(
                    "Mismatch between expected and actual data size")
        else:
            # Just return the data as is
            return data

    def encode_raw(self, value: Union[int, float, str, bytes, bytearray]) -> bytes:
        if isinstance(value, (bytes, bytearray)):
            return value
        elif self.data_type == VISIBLE_STRING:
            return value.encode("ascii")
        elif self.data_type == UNICODE_STRING:
            return value.encode("utf_16_le")
        elif self.data_type in (DOMAIN, OCTET_STRING):
            return bytes(value)
        elif self.data_type in self.STRUCT_TYPES:
            if self.data_type in INTEGER_TYPES:
                value = int(value)
            if self.data_type in NUMBER_TYPES:
                if self.min is not None and value < self.min:
                    logger.warning(
                        "Value %d is less than min value %d", value, self.min)
                if self.max is not None and value > self.max:
                    logger.warning(
                        "Value %d is greater than max value %d",
                        value, self.max)
            try:
                return self.STRUCT_TYPES[self.data_type].pack(value)
            except struct.error:
                raise ValueError("Value does not fit in specified type")
        elif self.data_type is None:
            raise ObjectDictionaryError("Data type has not been specified")
        else:
            raise TypeError(
                f"Do not know how to encode {value!r} to data type 0x{self.data_type:X}")

    def decode_phys(self, value: int) -> Union[int, bool, float, str, bytes]:
        if self.data_type in INTEGER_TYPES:
            value *= self.factor
        return value

    def encode_phys(self, value: Union[int, bool, float, str, bytes]) -> int:
        if self.data_type in INTEGER_TYPES:
            value /= self.factor
            value = int(round(value))
        return value

    def decode_desc(self, value: int) -> str:
        if not self.value_descriptions:
            raise ObjectDictionaryError("No value descriptions exist")
        elif value not in self.value_descriptions:
            raise ObjectDictionaryError(
                f"No value description exists for {value}")
        else:
            return self.value_descriptions[value]

    def encode_desc(self, desc: str) -> int:
        if not self.value_descriptions:
            raise ObjectDictionaryError("No value descriptions exist")
        else:
            for value, description in self.value_descriptions.items():
                if description == desc:
                    return value
        valid_values = ", ".join(self.value_descriptions.values())
        raise ValueError(
            f"No value corresponds to '{desc}'. Valid values are: {valid_values}")

    def decode_bits(self, value: int, bits: List[int]) -> int:
        try:
            bits = self.bit_definitions[bits]
        except (TypeError, KeyError):
            pass
        mask = 0
        for bit in bits:
            mask |= 1 << bit
        return (value & mask) >> min(bits)

    def encode_bits(self, original_value: int, bits: List[int], bit_value: int):
        try:
            bits = self.bit_definitions[bits]
        except (TypeError, KeyError):
            pass
        temp = original_value
        mask = 0
        for bit in bits:
            mask |= 1 << bit
        temp &= ~mask
        temp |= bit_value << min(bits)
        return temp


class DeviceInformation:
    def __init__(self):
        self.allowed_baudrates = set()
        self.vendor_name:Optional[str] = None
        self.vendor_number:Optional[int] = None
        self.product_name:Optional[str] = None
        self.product_number:Optional[int] = None
        self.revision_number:Optional[int] = None
        self.order_code:Optional[str] = None
        self.simple_boot_up_master:Optional[bool] = None
        self.simple_boot_up_slave:Optional[bool] = None
        self.granularity:Optional[int] = None
        self.dynamic_channels_supported:Optional[bool] = None
        self.group_messaging:Optional[bool] = None
        self.nr_of_RXPDO:Optional[bool] = None
        self.nr_of_TXPDO:Optional[bool] = None
        self.LSS_supported:Optional[bool] = None


class ObjectDictionaryError(Exception):
    """Unsupported operation with the current Object Dictionary."""


# Compatibility for old names
Record = ODRecord
Array = ODArray
Variable = ODVariable
