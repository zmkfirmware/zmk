import xml.etree.ElementTree as etree
import logging

from canopen import objectdictionary
from canopen.objectdictionary import ObjectDictionary

logger = logging.getLogger(__name__)

DATA_TYPES = {
    "BOOLEAN": objectdictionary.BOOLEAN,
    "INTEGER8": objectdictionary.INTEGER8,
    "INTEGER16": objectdictionary.INTEGER16,
    "INTEGER32": objectdictionary.INTEGER32,
    "UNSIGNED8": objectdictionary.UNSIGNED8,
    "UNSIGNED16": objectdictionary.UNSIGNED16,
    "UNSIGNED32": objectdictionary.UNSIGNED32,
    "REAL32": objectdictionary.REAL32,
    "VISIBLE_STRING": objectdictionary.VISIBLE_STRING,
    "DOMAIN": objectdictionary.DOMAIN
}


def import_epf(epf):
    """Import an EPF file.

    :param epf:
        Either a path to an EPF-file, a file-like object, or an instance of
        :class:`xml.etree.ElementTree.Element`.

    :returns:
        The Object Dictionary.
    :rtype: canopen.ObjectDictionary
    """
    od = ObjectDictionary()
    if etree.iselement(epf):
        tree = epf
    else:
        tree = etree.parse(epf).getroot()

    # Find and set default bitrate
    can_config = tree.find("Configuration/CANopen")
    if can_config is not None:
        bitrate = can_config.get("BitRate", "250")
        bitrate = bitrate.replace("U", "")
        od.bitrate = int(bitrate) * 1000

    # Parse Object Dictionary
    for group_tree in tree.iterfind("Dictionary/Parameters/Group"):
        name = group_tree.get("SymbolName")
        parameters = group_tree.findall("Parameter")
        index = int(parameters[0].get("Index"), 0)

        if len(parameters) == 1:
            # Simple variable
            var = build_variable(parameters[0])
            # Use top level index name instead
            var.name = name
            od.add_object(var)
        elif len(parameters) == 2 and parameters[1].get("ObjectType") == "ARRAY":
            # Array
            arr = objectdictionary.ODArray(name, index)
            for par_tree in parameters:
                var = build_variable(par_tree)
                arr.add_member(var)
            description = group_tree.find("Description")
            if description is not None:
                arr.description = description.text
            od.add_object(arr)
        else:
            # Complex record
            record = objectdictionary.ODRecord(name, index)
            for par_tree in parameters:
                var = build_variable(par_tree)
                record.add_member(var)
            description = group_tree.find("Description")
            if description is not None:
                record.description = description.text
            od.add_object(record)

    return od


def build_variable(par_tree):
    index = int(par_tree.get("Index"), 0)
    subindex = int(par_tree.get("SubIndex"))
    name = par_tree.get("SymbolName")
    data_type = par_tree.get("DataType")

    par = objectdictionary.ODVariable(name, index, subindex)
    factor = par_tree.get("Factor", "1")
    par.factor = int(factor) if factor.isdigit() else float(factor)
    unit = par_tree.get("Unit")
    if unit and unit != "-":
        par.unit = unit
    description = par_tree.find("Description")
    if description is not None:
        par.description = description.text
    if data_type in DATA_TYPES:
        par.data_type = DATA_TYPES[data_type]
    else:
        logger.warning("Don't know how to handle data type %s", data_type)
    par.access_type = par_tree.get("AccessType", "rw")
    try:
        par.min = int(par_tree.get("MinimumValue"))
    except (ValueError, TypeError):
        pass
    try:
        par.max = int(par_tree.get("MaximumValue"))
    except (ValueError, TypeError):
        pass
    try:
        par.default = int(par_tree.get("DefaultValue"))
    except (ValueError, TypeError):
        pass

    # Find value descriptions
    for value_field_def in par_tree.iterfind("ValueFieldDefs/ValueFieldDef"):
        value = int(value_field_def.get("Value"), 0)
        desc = value_field_def.get("Description")
        par.add_value_description(value, desc)

    # Find bit field descriptions
    for bits_tree in par_tree.iterfind("BitFieldDefs/BitFieldDef"):
        name = bits_tree.get("Name")
        bits = [int(bit) for bit in bits_tree.get("Bit").split(",")]
        par.add_bit_definition(name, bits)

    return par
