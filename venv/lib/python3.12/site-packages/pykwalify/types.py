# -*- coding: utf-8 -*-

""" pyKwalify - types.py """

# python stdlib
import re
import datetime
import re
from pykwalify.compat import basestring, bytes

DEFAULT_TYPE = "str"


class TextMeta(type):
    def __instancecheck__(self, instance):
        return is_text(instance)


class text(object):
    __metaclass__ = TextMeta


_types = {
    "str": str,
    "int": int,
    "float": float,
    "number": None,
    "bool": bool,
    "map": dict,
    "seq": list,
    "timestamp": datetime.datetime,
    "date": datetime.date,
    "symbol": str,
    "scalar": None,
    "text": text,
    "any": object,
    "enum": str,
    "none": None,
    "email": str,
    "url": str,
}


sequence_aliases = ["sequence", "seq"]
mapping_aliases = ["map", "mapping"]


def type_class(type):
    return _types[type]


def is_builtin_type(type):
    return type in _types


def is_collection_type(type):
    return type.lower().strip() == "map" or type.lower().strip() == "seq"


def is_scalar_type(type):
    return not is_collection_type(type)


def is_collection(obj):
    return isinstance(obj, dict) or isinstance(obj, list)


def is_scalar(obj):
    return not is_collection(obj) and obj is not None


def is_correct_type(obj, type):
    return isinstance(obj, type)


def is_string(obj):
    return isinstance(obj, basestring) or isinstance(obj, bytes)


def is_int(obj):
    """
    True & False is not considered valid integers even if python considers them 1 & 0 in some versions
    """
    return isinstance(obj, int) and not isinstance(obj, bool)


def is_bool(obj):
    return isinstance(obj, bool)


def is_float(obj):
    """
    Valid types are:
     - objects of float type
     - Strings that can be converted to float. For example '1e-06'
    """
    is_f = isinstance(obj, float)
    if not is_f:
        try:
            float(obj)
            is_f = True
        except (ValueError, TypeError):
            is_f = False
    return is_f and not is_bool(obj)


def is_number(obj):
    return is_int(obj) or is_float(obj)


def is_text(obj):
    return (is_string(obj) or is_number(obj)) and is_bool(obj) is False


def is_any(obj):
    return True


def is_enum(obj):
    return isinstance(obj, basestring)


def is_none(obj):
    return obj is None


def is_sequence_alias(alias):
    return alias in sequence_aliases


def is_mapping_alias(alias):
    return alias in mapping_aliases


def is_timestamp(obj):
    """
    Yaml either have automatically converted it to a datetime object
    or it is a string that will be validated later.
    """
    return isinstance(obj, datetime.datetime) or is_string(obj) or is_int(obj) or is_float(obj)


def is_date(obj):
    """
    :param obj: Object that is to be validated
    :return: True/False if obj is valid date object
    """
    return isinstance(obj, basestring) or isinstance(obj, datetime.date)


def is_email(obj):
    """
    """
    return re.match(r"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)", obj)


def is_url(obj):
    """
    :param obj: Object that is to be validated
    :return: True/False if obj is valid 
    """
    return re.match(r'http[s]?://(?:[a-zA-Z]|[0-9]|[$-_@.&+]|[!*(),]|(?:%[0-9a-fA-F][0-9a-fA-F]))+', obj)


tt = {
    "str": is_string,
    "int": is_int,
    "bool": is_bool,
    "float": is_float,
    "number": is_number,
    "text": is_text,
    "any": is_any,
    "enum": is_enum,
    "none": is_none,
    "timestamp": is_timestamp,
    "scalar": is_scalar,
    "date": is_date,
    "email": is_email,
    "url": is_url,
}
