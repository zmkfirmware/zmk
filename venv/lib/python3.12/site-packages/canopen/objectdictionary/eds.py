import copy
import logging
import re
from configparser import RawConfigParser, NoOptionError, NoSectionError

from canopen import objectdictionary
from canopen.objectdictionary import ObjectDictionary, datatypes
from canopen.sdo import SdoClient

logger = logging.getLogger(__name__)

# Object type. Don't confuse with Data type
DOMAIN = 2
VAR = 7
ARR = 8
RECORD = 9


def import_eds(source, node_id):
    eds = RawConfigParser(inline_comment_prefixes=(';',))
    eds.optionxform = str
    opened_here = False
    try:
        if hasattr(source, "read"):
            fp = source
        else:
            fp = open(source)
            opened_here = True
        eds.read_file(fp)
    finally:
        # Only close object if opened in this fn
        if opened_here:
            fp.close()

    od = ObjectDictionary()

    if eds.has_section("FileInfo"):
        od.__edsFileInfo = {
            opt: eds.get("FileInfo", opt)
            for opt in eds.options("FileInfo")
        }

    if eds.has_section("Comments"):
        linecount = int(eds.get("Comments", "Lines"), 0)
        od.comments = '\n'.join([
            eds.get("Comments", f"Line{line}")
            for line in range(1, linecount+1)
        ])

    if not eds.has_section("DeviceInfo"):
        logger.warn("eds file does not have a DeviceInfo section. This section is mandatory")
    else:
        for rate in [10, 20, 50, 125, 250, 500, 800, 1000]:
            baudPossible = int(
                eds.get("DeviceInfo", f"BaudRate_{rate}", fallback='0'), 0)
            if baudPossible != 0:
                od.device_information.allowed_baudrates.add(rate*1000)

        for t, eprop, odprop in [
            (str, "VendorName", "vendor_name"),
            (int, "VendorNumber", "vendor_number"),
            (str, "ProductName", "product_name"),
            (int, "ProductNumber", "product_number"),
            (int, "RevisionNumber", "revision_number"),
            (str, "OrderCode", "order_code"),
            (bool, "SimpleBootUpMaster", "simple_boot_up_master"),
            (bool, "SimpleBootUpSlave", "simple_boot_up_slave"),
            (bool, "Granularity", "granularity"),
            (bool, "DynamicChannelsSupported", "dynamic_channels_supported"),
            (bool, "GroupMessaging", "group_messaging"),
            (int, "NrOfRXPDO", "nr_of_RXPDO"),
            (int, "NrOfTXPDO", "nr_of_TXPDO"),
            (bool, "LSS_Supported", "LSS_supported"),
        ]:
            try:
                if t in (int, bool):
                    setattr(od.device_information, odprop,
                            t(int(eds.get("DeviceInfo", eprop), 0))
                            )
                elif t is str:
                    setattr(od.device_information, odprop,
                            eds.get("DeviceInfo", eprop)
                            )
            except NoOptionError:
                pass

    if eds.has_section("DeviceComissioning"):
        od.bitrate = int(eds.get("DeviceComissioning", "Baudrate")) * 1000
        od.node_id = int(eds.get("DeviceComissioning", "NodeID"), 0)
        node_id = node_id or od.node_id

    for section in eds.sections():
        # Match dummy definitions
        match = re.match(r"^[Dd]ummy[Uu]sage$", section)
        if match is not None:
            for i in range(1, 8):
                key = f"Dummy{i:04d}"
                if eds.getint(section, key) == 1:
                    var = objectdictionary.ODVariable(key, i, 0)
                    var.data_type = i
                    var.access_type = "const"
                    od.add_object(var)

        # Match indexes
        match = re.match(r"^[0-9A-Fa-f]{4}$", section)
        if match is not None:
            index = int(section, 16)
            name = eds.get(section, "ParameterName")
            try:
                object_type = int(eds.get(section, "ObjectType"), 0)
            except NoOptionError:
                # DS306 4.6.3.2 object description
                # If the keyword ObjectType is missing, this is regarded as
                # "ObjectType=0x7" (=VAR).
                object_type = VAR
            try:
                storage_location = eds.get(section, "StorageLocation")
            except NoOptionError:
                storage_location = None

            if object_type in (VAR, DOMAIN):
                var = build_variable(eds, section, node_id, index)
                od.add_object(var)
            elif object_type == ARR and eds.has_option(section, "CompactSubObj"):
                arr = objectdictionary.ODArray(name, index)
                last_subindex = objectdictionary.ODVariable(
                    "Number of entries", index, 0)
                last_subindex.data_type = datatypes.UNSIGNED8
                arr.add_member(last_subindex)
                arr.add_member(build_variable(eds, section, node_id, index, 1))
                arr.storage_location = storage_location
                od.add_object(arr)
            elif object_type == ARR:
                arr = objectdictionary.ODArray(name, index)
                arr.storage_location = storage_location
                od.add_object(arr)
            elif object_type == RECORD:
                record = objectdictionary.ODRecord(name, index)
                record.storage_location = storage_location
                od.add_object(record)

            continue

        # Match subindexes
        match = re.match(r"^([0-9A-Fa-f]{4})[S|s]ub([0-9A-Fa-f]+)$", section)
        if match is not None:
            index = int(match.group(1), 16)
            subindex = int(match.group(2), 16)
            entry = od[index]
            if isinstance(entry, (objectdictionary.ODRecord,
                                  objectdictionary.ODArray)):
                var = build_variable(eds, section, node_id, index, subindex)
                entry.add_member(var)

        # Match [index]Name
        match = re.match(r"^([0-9A-Fa-f]{4})Name", section)
        if match is not None:
            index = int(match.group(1), 16)
            num_of_entries = int(eds.get(section, "NrOfEntries"))
            entry = od[index]
            # For CompactSubObj index 1 is were we find the variable
            src_var = od[index][1]
            for subindex in range(1, num_of_entries + 1):
                var = copy_variable(eds, section, subindex, src_var)
                if var is not None:
                    entry.add_member(var)

    return od


def import_from_node(node_id, network):
    """ Download the configuration from the remote node
    :param int node_id: Identifier of the node
    :param network: network object
    """
    # Create temporary SDO client
    sdo_client = SdoClient(0x600 + node_id, 0x580 + node_id, ObjectDictionary())
    sdo_client.network = network
    # Subscribe to SDO responses
    network.subscribe(0x580 + node_id, sdo_client.on_response)
    # Create file like object for Store EDS variable
    try:
        with sdo_client.open(0x1021, 0, "rt") as eds_fp:
            od = import_eds(eds_fp, node_id)
    except Exception as e:
        logger.error("No object dictionary could be loaded for node %d: %s",
                     node_id, e)
        od = None
    finally:
        network.unsubscribe(0x580 + node_id)
    return od


def _calc_bit_length(data_type):
    if data_type == datatypes.INTEGER8:
        return 8
    elif data_type == datatypes.INTEGER16:
        return 16
    elif data_type == datatypes.INTEGER32:
        return 32
    elif data_type == datatypes.INTEGER64:
        return 64
    else:
        raise ValueError(f"Invalid data_type '{data_type}', expecting a signed integer data_type.")


def _signed_int_from_hex(hex_str, bit_length):
    number = int(hex_str, 0)
    max_value = (1 << (bit_length - 1)) - 1

    if number > max_value:
        return number - (1 << bit_length)
    else:
        return number


def _convert_variable(node_id, var_type, value):
    if var_type in (datatypes.OCTET_STRING, datatypes.DOMAIN):
        return bytes.fromhex(value)
    elif var_type in (datatypes.VISIBLE_STRING, datatypes.UNICODE_STRING):
        return value
    elif var_type in datatypes.FLOAT_TYPES:
        return float(value)
    else:
        # COB-ID can contain '$NODEID+' so replace this with node_id before converting
        value = value.replace(" ", "").upper()
        if '$NODEID' in value and node_id is not None:
            return int(re.sub(r'\+?\$NODEID\+?', '', value), 0) + node_id
        else:
            return int(value, 0)


def _revert_variable(var_type, value):
    if value is None:
        return None
    if var_type in (datatypes.OCTET_STRING, datatypes.DOMAIN):
        return bytes.hex(value)
    elif var_type in (datatypes.VISIBLE_STRING, datatypes.UNICODE_STRING):
        return value
    elif var_type in datatypes.FLOAT_TYPES:
        return value
    else:
        return f"0x{value:02X}"


def build_variable(eds, section, node_id, index, subindex=0):
    """Creates a object dictionary entry.
    :param eds: String stream of the eds file
    :param section:
    :param node_id: Node ID
    :param index: Index of the CANOpen object
    :param subindex: Subindex of the CANOpen object (if presente, else 0)
    """
    name = eds.get(section, "ParameterName")
    var = objectdictionary.ODVariable(name, index, subindex)
    try:
        var.storage_location = eds.get(section, "StorageLocation")
    except NoOptionError:
        var.storage_location = None
    var.data_type = int(eds.get(section, "DataType"), 0)
    var.access_type = eds.get(section, "AccessType").lower()
    if var.data_type > 0x1B:
        # The object dictionary editor from CANFestival creates an optional object if min max values are used
        # This optional object is then placed in the eds under the section [A0] (start point, iterates for more)
        # The eds.get function gives us 0x00A0 now convert to String without hex representation and upper case
        # The sub2 part is then the section where the type parameter stands
        try:
            var.data_type = int(eds.get(f"{var.data_type:X}sub1", "DefaultValue"), 0)
        except NoSectionError:
            logger.warning("%s has an unknown or unsupported data type (0x%X)", name, var.data_type)
            # Assume DOMAIN to force application to interpret the byte data
            var.data_type = datatypes.DOMAIN

    var.pdo_mappable = bool(int(eds.get(section, "PDOMapping", fallback="0"), 0))

    if eds.has_option(section, "LowLimit"):
        try:
            min_string = eds.get(section, "LowLimit")
            if var.data_type in datatypes.SIGNED_TYPES:
                var.min = _signed_int_from_hex(min_string, _calc_bit_length(var.data_type))
            else:
                var.min = int(min_string, 0)
        except ValueError:
            pass
    if eds.has_option(section, "HighLimit"):
        try:
            max_string = eds.get(section, "HighLimit")
            if var.data_type in datatypes.SIGNED_TYPES:
                var.max = _signed_int_from_hex(max_string, _calc_bit_length(var.data_type))
            else:
                var.max = int(max_string, 0)
        except ValueError:
            pass
    if eds.has_option(section, "DefaultValue"):
        try:
            var.default_raw = eds.get(section, "DefaultValue")
            if '$NODEID' in var.default_raw:
                var.relative = True
            var.default = _convert_variable(node_id, var.data_type, eds.get(section, "DefaultValue"))
        except ValueError:
            pass
    if eds.has_option(section, "ParameterValue"):
        try:
            var.value_raw = eds.get(section, "ParameterValue")
            var.value = _convert_variable(node_id, var.data_type, eds.get(section, "ParameterValue"))
        except ValueError:
            pass
    # Factor, Description and Unit are not standard according to the CANopen specifications, but they are implemented in the python canopen package, so we can at least try to use them
    if eds.has_option(section, "Factor"):
        try:
            var.factor = float(eds.get(section, "Factor"))
        except ValueError:
            pass
    if eds.has_option(section, "Description"):
        try:
            var.description = eds.get(section, "Description")
        except ValueError:
            pass
    if eds.has_option(section, "Unit"):
        try:
            var.unit = eds.get(section, "Unit")
        except ValueError:
            pass
    return var


def copy_variable(eds, section, subindex, src_var):
    name = eds.get(section, str(subindex))
    var = copy.copy(src_var)
    # It is only the name and subindex that varies
    var.name = name
    var.subindex = subindex
    return var


def export_dcf(od, dest=None, fileInfo={}):
    return export_eds(od, dest, fileInfo, True)


def export_eds(od, dest=None, file_info={}, device_commisioning=False):
    def export_object(obj, eds):
        if isinstance(obj, objectdictionary.ODVariable):
            return export_variable(obj, eds)
        if isinstance(obj, objectdictionary.ODRecord):
            return export_record(obj, eds)
        if isinstance(obj, objectdictionary.ODArray):
            return export_array(obj, eds)

    def export_common(var, eds, section):
        eds.add_section(section)
        eds.set(section, "ParameterName", var.name)
        if var.storage_location:
            eds.set(section, "StorageLocation", var.storage_location)

    def export_variable(var, eds):
        if isinstance(var.parent, ObjectDictionary):
            # top level variable
            section = f"{var.index:04X}"
        else:
            # nested variable
            section = f"{var.index:04X}sub{var.subindex:X}"

        export_common(var, eds, section)
        eds.set(section, "ObjectType", f"0x{VAR:X}")
        if var.data_type:
            eds.set(section, "DataType", f"0x{var.data_type:04X}")
        if var.access_type:
            eds.set(section, "AccessType", var.access_type)

        if getattr(var, 'default_raw', None) is not None:
            eds.set(section, "DefaultValue", var.default_raw)
        elif getattr(var, 'default', None) is not None:
            eds.set(section, "DefaultValue", _revert_variable(
                var.data_type, var.default))

        if device_commisioning:
            if getattr(var, 'value_raw', None) is not None:
                eds.set(section, "ParameterValue", var.value_raw)
            elif getattr(var, 'value', None) is not None:
                eds.set(section, "ParameterValue",
                        _revert_variable(var.data_type, var.value))

        eds.set(section, "DataType", f"0x{var.data_type:04X}")
        eds.set(section, "PDOMapping", hex(var.pdo_mappable))

        if getattr(var, 'min', None) is not None:
            eds.set(section, "LowLimit", var.min)
        if getattr(var, 'max', None) is not None:
            eds.set(section, "HighLimit", var.max)

        if getattr(var, 'description', '') != '':
            eds.set(section, "Description", var.description)
        if getattr(var, 'factor', 1) != 1:
            eds.set(section, "Factor", var.factor)
        if getattr(var, 'unit', '') != '':
            eds.set(section, "Unit", var.unit)

    def export_record(var, eds):
        section = f"{var.index:04X}"
        export_common(var, eds, section)
        eds.set(section, "SubNumber", f"0x{len(var.subindices):X}")
        ot = RECORD if isinstance(var, objectdictionary.ODRecord) else ARR
        eds.set(section, "ObjectType", f"0x{ot:X}")
        for i in var:
            export_variable(var[i], eds)

    export_array = export_record

    eds = RawConfigParser()
    # both disables lowercasing, and allows int keys
    eds.optionxform = str

    from datetime import datetime as dt
    defmtime = dt.utcnow()

    try:
        # only if eds was loaded by us
        origFileInfo = od.__edsFileInfo
    except AttributeError:
        origFileInfo = {
            # just set some defaults
            "CreationDate": defmtime.strftime("%m-%d-%Y"),
            "CreationTime": defmtime.strftime("%I:%m%p"),
            "EdsVersion": 4.2,
        }

    file_info.setdefault("ModificationDate", defmtime.strftime("%m-%d-%Y"))
    file_info.setdefault("ModificationTime", defmtime.strftime("%I:%m%p"))
    for k, v in origFileInfo.items():
        file_info.setdefault(k, v)

    eds.add_section("FileInfo")
    for k, v in file_info.items():
        eds.set("FileInfo", k, v)

    eds.add_section("DeviceInfo")
    for eprop, odprop in [
        ("VendorName", "vendor_name"),
        ("VendorNumber", "vendor_number"),
        ("ProductName", "product_name"),
        ("ProductNumber", "product_number"),
        ("RevisionNumber", "revision_number"),
        ("OrderCode", "order_code"),
        ("SimpleBootUpMaster", "simple_boot_up_master"),
        ("SimpleBootUpSlave", "simple_boot_up_slave"),
        ("Granularity", "granularity"),
        ("DynamicChannelsSupported", "dynamic_channels_supported"),
        ("GroupMessaging", "group_messaging"),
        ("NrOfRXPDO", "nr_of_RXPDO"),
        ("NrOfTXPDO", "nr_of_TXPDO"),
        ("LSS_Supported", "LSS_supported"),
    ]:
        val = getattr(od.device_information, odprop, None)
        if val is None:
            continue
        elif isinstance(val, str):
            eds.set("DeviceInfo", eprop, val)
        elif isinstance(val, (int, bool)):
            eds.set("DeviceInfo", eprop, int(val))

    # we are also adding out of spec baudrates here.
    for rate in od.device_information.allowed_baudrates.union(
            {10e3, 20e3, 50e3, 125e3, 250e3, 500e3, 800e3, 1000e3}):
        eds.set(
            "DeviceInfo", f"BaudRate_{rate//1000}",
            int(rate in od.device_information.allowed_baudrates))

    if device_commisioning and (od.bitrate or od.node_id):
        eds.add_section("DeviceComissioning")
        if od.bitrate:
            eds.set("DeviceComissioning", "Baudrate", int(od.bitrate / 1000))
        if od.node_id:
            eds.set("DeviceComissioning", "NodeID", int(od.node_id))

    eds.add_section("Comments")
    i = 0
    for line in od.comments.splitlines():
        i += 1
        eds.set("Comments", f"Line{i}", line)
    eds.set("Comments", "Lines", i)

    eds.add_section("DummyUsage")
    for i in range(1, 8):
        key = f"Dummy{i:04d}"
        eds.set("DummyUsage", key, 1 if (key in od) else 0)

    def mandatory_indices(x):
        return x in {0x1000, 0x1001, 0x1018}

    def manufacturer_idices(x):
        return x in range(0x2000, 0x6000)

    def optional_indices(x):
        return all((
            x > 0x1001,
            not mandatory_indices(x),
            not manufacturer_idices(x),
        ))

    supported_mantatory_indices = list(filter(mandatory_indices, od))
    supported_optional_indices = list(filter(optional_indices, od))
    supported_manufacturer_indices = list(filter(manufacturer_idices, od))

    def add_list(section, list):
        eds.add_section(section)
        eds.set(section, "SupportedObjects", len(list))
        for i in range(0, len(list)):
            eds.set(section, (i + 1), f"0x{list[i]:04X}")
        for index in list:
            export_object(od[index], eds)

    add_list("MandatoryObjects", supported_mantatory_indices)
    add_list("OptionalObjects", supported_optional_indices)
    add_list("ManufacturerObjects", supported_manufacturer_indices)

    if not dest:
        import sys
        dest = sys.stdout

    eds.write(dest, False)
