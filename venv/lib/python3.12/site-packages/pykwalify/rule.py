# -*- coding: utf-8 -*-

""" pyKwalify - rule.py """

# python stdlib
import logging
import re

# pykwalify imports
from pykwalify.compat import basestring
from pykwalify.errors import SchemaConflict, RuleError
from pykwalify.types import (
    DEFAULT_TYPE,
    is_bool,
    is_builtin_type,
    is_collection_type,
    is_number,
    is_string,
    mapping_aliases,
    sequence_aliases,
    type_class,
)

log = logging.getLogger(__name__)


class Rule(object):
    """ Rule class that handles a rule constraint """

    def __init__(self, schema=None, parent=None, strict_rule_validation=False):
        self._allowempty_map = None
        self._assertion = None
        self._default = None
        self._desc = None
        self._enum = None
        self._example = None
        self._extensions = None
        self._format = None
        self._func = None
        self._ident = None
        self._include_name = None
        self._length = None
        self._map_regex_rule = None
        self._mapping = None
        # Possible values: [any, all, *]
        self._matching = "any"
        self._matching_rule = "any"
        self._name = None
        self._nullable = True
        self._parent = parent
        self._pattern = None
        self._pattern_regexp = None
        self._range = None
        self._regex_mappings = None
        self._required = False
        self._schema = schema
        self._schema_str = schema
        self._sequence = None
        self.strict_rule_validation = strict_rule_validation
        self._type = None
        self._type_class = None
        self._unique = None
        self._version = None

        if isinstance(schema, dict):
            self.init(schema, "")

    @property
    def allowempty_map(self):
        return self._allowempty_map

    @allowempty_map.setter
    def allowempty_map(self, value):
        self._allowempty_map = value

    @property
    def assertion(self):
        return self._assertion

    @assertion.setter
    def assertion(self, value):
        self._assertion = value

    @property
    def default(self):
        return self._default

    @default.setter
    def default(self, value):
        self._default = value

    @property
    def desc(self):
        return self._desc

    @desc.setter
    def desc(self, value):
        self._desc = value

    @property
    def enum(self):
        return self._enum

    @enum.setter
    def enum(self, value):
        self._enum = value

    @property
    def example(self):
        return self._example

    @example.setter
    def example(self, value):
        self._example = value

    @property
    def extensions(self):
        return self._extensions

    @extensions.setter
    def extensions(self, value):
        self._extensions = value

    @property
    def format(self):
        return self._format

    @format.setter
    def format(self, value):
        self._format = value

    @property
    def func(self):
        return self._func

    @func.setter
    def func(self, value):
        self._func = value

    @property
    def ident(self):
        return self._ident

    @ident.setter
    def ident(self, value):
        self._ident = value

    @property
    def include_name(self):
        return self._include_name

    @include_name.setter
    def include_name(self, value):
        self._include_name = value

    @property
    def length(self):
        return self._length

    @length.setter
    def length(self, value):
        self._length = value

    @property
    def map_regex_rule(self):
        return self._map_regex_rule

    @map_regex_rule.setter
    def map_regex_rule(self, value):
        self._map_regex_rule = value

    @property
    def mapping(self):
        return self._mapping

    @mapping.setter
    def mapping(self, value):
        self._mapping = value

    @property
    def matching(self):
        return self._matching

    @matching.setter
    def matching(self, value):
        self._matching = value

    @property
    def matching_rule(self):
        return self._matching_rule

    @matching_rule.setter
    def matching_rule(self, value):
        self._matching_rule = value

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def nullable(self):
        return self._nullable

    @nullable.setter
    def nullable(self, value):
        self._nullable = value

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, value):
        self._parent = value

    @property
    def pattern(self):
        return self._pattern

    @pattern.setter
    def pattern(self, value):
        self._pattern = value

    @property
    def pattern_regexp(self):
        return self._pattern_regexp

    @pattern_regexp.setter
    def pattern_regexp(self, value):
        self._pattern_regexp = value

    @property
    def range(self):
        return self._range

    @range.setter
    def range(self, value):
        self._range = value

    @property
    def regex_mappings(self):
        return self._regex_mappings

    @regex_mappings.setter
    def regex_mappings(self, value):
        self._regex_mappings = value

    @property
    def required(self):
        return self._required

    @required.setter
    def required(self, value):
        self._required = value

    @property
    def schema(self):
        return self._schema

    @schema.setter
    def schema(self, value):
        self._schema = value

    @property
    def schema_str(self):
        return self._schema_str

    @schema_str.setter
    def schema_str(self, value):
        self._schema_str = value

    @property
    def sequence(self):
        return self._sequence

    @sequence.setter
    def sequence(self, value):
        self._sequence = value

    @property
    def type(self):
        return self._type

    @type.setter
    def type(self, value):
        self._type = value

    @property
    def type_class(self):
        return self._type_class

    @type_class.setter
    def type_class(self, value):
        self._type_class = value

    @property
    def unique(self):
        return self._unique

    @unique.setter
    def unique(self, value):
        self._unique = value

    @property
    def version(self):
        return self._version

    @version.setter
    def version(self, value):
        self._version = value

    def __str__(self):
        return "Rule: {0}".format(str(self.schema_str))

    def keywords(self):
        """
        Returns a list of all keywords that this rule object has defined.
        A keyword is considered defined if the value it returns != None.
        """
        defined_keywords = [
            ('allowempty_map', 'allowempty_map'),
            ('assertion', 'assertion'),
            ('default', 'default'),
            ('class', 'class'),
            ('desc', 'desc'),
            ('enum', 'enum'),
            ('example', 'example'),
            ('extensions', 'extensions'),
            ('format', 'format'),
            ('func', 'func'),
            ('ident', 'ident'),
            ('include_name', 'include'),
            ('length', 'length'),
            ('map_regex_rule', 'map_regex_rule'),
            ('mapping', 'mapping'),
            ('matching', 'matching'),
            ('matching_rule', 'matching_rule'),
            ('name', 'name'),
            ('nullable', 'nullable'),
            ('parent', 'parent'),
            ('pattern', 'pattern'),
            ('pattern_regexp', 'pattern_regexp'),
            ('range', 'range'),
            ('regex_mappings', 'regex_mappings'),
            ('required', 'required'),
            ('schema', 'schema'),
            ('schema_str', 'schema_str'),
            ('sequence', 'sequence'),
            ('type', 'type'),
            ('type_class', 'type_class'),
            ('unique', 'unique'),
            ('version', 'version'),
        ]
        found_keywords = []

        for var_name, keyword_name in defined_keywords:
            if getattr(self, var_name, None):
                found_keywords.append(keyword_name)

        return found_keywords

    def init(self, schema, path):
        """
        """
        log.debug(u"Init schema: %s", schema)

        include = schema.get("include")

        # Check if this item is a include, overwrite schema with include schema and continue to parse
        if include:
            log.debug(u"Found include tag...")
            self.include_name = include
            return

        t = None
        rule = self

        if schema is not None:
            if "type" not in schema:
                # Mapping and sequence do not need explicit type defenitions
                if any(sa in schema for sa in sequence_aliases):
                    t = "seq"
                    self.init_type_value(t, rule, path)
                elif any(ma in schema for ma in mapping_aliases):
                    t = "map"
                    self.init_type_value(t, rule, path)
                else:
                    t = DEFAULT_TYPE
                    self.type = t
            else:
                if not is_string(schema["type"]):
                    raise RuleError(
                        msg=u"Key 'type' in schema rule is not a string type (found %s)" % type(schema["type"]).__name__,
                        error_key=u"type.not_string",
                        path=path,
                    )

                self.type = schema["type"]

        self.schema_str = schema

        if not t:
            t = schema["type"]
            self.init_type_value(t, rule, path)

        func_mapping = {
            "allowempty": self.init_allow_empty_map,
            "assert": self.init_assert_value,
            "class": lambda x, y, z: (),
            "default": self.init_default_value,
            "desc": self.init_desc_value,
            "enum": self.init_enum_value,
            "example": self.init_example,
            "extensions": self.init_extensions,
            "format": self.init_format_value,
            "func": self.init_func,
            "ident": self.init_ident_value,
            "length": self.init_length_value,
            "map": self.init_mapping_value,
            "mapping": self.init_mapping_value,
            "matching": self.init_matching,
            "matching-rule": self.init_matching_rule,
            "name": self.init_name_value,
            "nul": self.init_nullable_value,
            "nullable": self.init_nullable_value,
            "pattern": self.init_pattern_value,
            "range": self.init_range_value,
            "req": self.init_required_value,
            "required": self.init_required_value,
            "seq": self.init_sequence_value,
            "sequence": self.init_sequence_value,
            "type": lambda x, y, z: (),
            "unique": self.init_unique_value,
            "version": self.init_version,
        }

        for k, v in schema.items():
            if k in func_mapping:
                func_mapping[k](v, rule, path)
            elif k.startswith("schema;"):
                # Schema tag is only allowed on top level of data
                log.debug(u"Found schema tag...")
                raise RuleError(
                    msg=u"Schema is only allowed on top level of schema file",
                    error_key=u"schema.not.toplevel",
                    path=path,
                )
            else:
                raise RuleError(
                    msg=u"Unknown key: {0} found".format(k),
                    error_key=u"key.unknown",
                    path=path,
                )

        self.check_conflicts(schema, rule, path)

        self.check_type_keywords(schema, rule, path)

    def init_format_value(self, v, rule, path):
        log.debug(u"Init format value : %s", path)

        if is_string(v):
            self._format = [v]
        elif isinstance(v, list):
            valid = True
            for date_format in v:
                if not isinstance(date_format, basestring):
                    valid = False

            if valid:
                self._format = v
            else:
                raise RuleError(
                    msg=u"All values in format list must be strings",
                    error_key=u"format.not_string",
                    path=path,
                )
        else:
            raise RuleError(
                msg=u"Value of format keyword: '{}' must be a string or list or string values".format(v),
                error_key=u"format.not_string",
                path=path,
            )

        valid_types = ("date", )

        # Format is only supported when used with "type=date"
        if self._type not in valid_types:
            raise RuleError(
                msg="Keyword format is only allowed when used with the following types: {0}".format(valid_types),
                error_key=u"format.not_used_with_correct_type",
                path=path,
            )

    def init_version(self, v, rule, path):
        """
        """
        log.debug(u"Init version value : {0}".format(path))

        self._version = str(v)

    def init_example(self, v, rule, path):
        log.debug(u'Init example value : {0}'.format(path))

        if not is_string(v):
            raise RuleError(
                msg=u"Value: {0} for keyword example must be a string".format(v),
                error_key=u"example.not_string",
                path=path,
            )

        self.desc = v

    def init_length_value(self, v, rule, path):
        log.debug(u'Init length value : {0}'.format(path))

        supported_types = ["str", "int", "float", "number", "map", "seq"]

        if not isinstance(v, dict):
            raise RuleError(
                msg=u"Length value is not a dict type: '{0}'".format(v),
                error_key=u"length.not_map",
                path=path,
            )

        if self.type not in supported_types:
            raise RuleError(
                msg=u"Length value type: '{0}' is not a supported type".format(self.type),
                error_key=u"length.not_supported_type",
                path=path,
            )

        # dict that should contain min, max, min-ex, max-ex keys
        self.length = v

        # This should validate that only min, max, min-ex, max-ex exists in the dict
        for k, v in self.length.items():
            if k not in ["max", "min", "max-ex", "min-ex"]:
                raise RuleError(
                    msg=u"Unknown key: '{0}' found in length keyword".format(k),
                    error_key=u"length.unknown_key",
                    path=path,
                )

        if "max" in self.length and "max-ex" in self.length:
            raise RuleError(
                msg=u"'max' and 'max-ex' can't be used in the same length rule",
                error_key=u"length.max_duplicate_keywords",
                path=path,
            )

        if "min" in self.length and "min-ex" in self.length:
            raise RuleError(
                msg=u"'min' and 'min-ex' can't be used in the same length rule",
                error_key=u"length.min_duplicate_keywords",
                path=path,
            )

        max = self.length.get("max")
        min = self.length.get("min")
        max_ex = self.length.get("max-ex")
        min_ex = self.length.get("min-ex")

        if max is not None and not is_number(max) or is_bool(max):
            raise RuleError(
                msg=u"Value: '{0}' for 'max' keyword is not a number".format(v),
                error_key=u"length.max.not_number",
                path=path,
            )

        if min is not None and not is_number(min) or is_bool(min):
            raise RuleError(
                msg=u"Value: '{0}' for 'min' keyword is not a number".format(v),
                error_key=u"length.min.not_number",
                path=path,
            )

        if max_ex is not None and not is_number(max_ex) or is_bool(max_ex):
            raise RuleError(
                msg=u"Value: '{0}' for 'max-ex' keyword is not a number".format(v),
                error_key=u"length.max_ex.not_number",
                path=path,
            )

        if min_ex is not None and not is_number(min_ex) or is_bool(min_ex):
            raise RuleError(
                msg=u"Value: '{0}' for 'min-ex' keyword is not a number".format(v),
                error_key=u"length.min_ex.not_number",
                path=path,
            )

        # only numbers allow negative lengths
        # string, map and seq require non negative lengtsh
        if self.type not in ["int", "float", "number"]:
            if min is not None and min < 0:
                raise RuleError(
                    msg=u"Value for 'min' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"length.min_negative",
                    path=path,
                )
            elif min_ex is not None and min_ex < 0:
                raise RuleError(
                    msg=u"Value for 'min-ex' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"length.min-ex_negative",
                    path=path,
                )
            if max is not None and max < 0:
                raise RuleError(
                    msg=u"Value for 'max' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"length.max_negative",
                    path=path,
                )
            elif max_ex is not None and max_ex < 0:
                raise RuleError(
                    msg=u"Value for 'max-ex' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"length.max-ex_negative",
                    path=path,
                )

        if max is not None:
            if min is not None and max < min:
                raise RuleError(
                    msg=u"Value for 'max' can't be less then value for 'min'. {0} < {1}".format(max, min),
                    error_key=u"length.max_lt_min",
                    path=path,
                )
            elif min_ex is not None and max <= min_ex:
                raise RuleError(
                    msg=u"Value for 'max' can't be less then value for 'min-ex'. {0} <= {1}".format(max, min_ex),
                    error_key=u"length.max_le_min-ex",
                    path=path,
                )
        elif max_ex is not None:
            if min is not None and max_ex < min:
                raise RuleError(
                    msg=u"Value for 'max-ex' can't be less then value for 'min'. {0} < {1}".format(max_ex, min),
                    error_key=u"length.max-ex_le_min",
                    path=path,
                )
            elif min_ex is not None and max_ex <= min_ex:
                raise RuleError(
                    msg=u"Value for 'max-ex' can't be less then value for 'min-ex'. {0} <= {1}".format(max_ex, min_ex),
                    error_key=u"length.max-ex_le_min-ex",
                    path=path,
                )

    def init_func(self, v, rule, path):
        """
        """
        if not is_string(v):
            raise RuleError(
                msg=u"Value: {0} for func keyword must be a string".format(v),
                error_key=u"func.notstring",
                path=path,
            )

        self.func = v

    def init_extensions(self, v, rule, path):
        """
        """
        if not isinstance(v, list):
            raise RuleError(
                msg=u"Extension definition should be a list",
                error_key=u"extension.not_list",
                path=path,
            )

        # TODO: Add limitation that this keyword can only be used at the top level of the file

        self.extensions = v

    def init_matching_rule(self, v, rule, path):
        """
        """
        log.debug(u"Init matching-rule: %s", path)
        log.debug(u"%s %s", v, rule)

        # Verify that the provided rule is part of one of the allowed one
        allowed = ["any", "all"]
        # ["none", "one"] Is currently awaiting proper implementation
        if v not in allowed:
            raise RuleError(
                msg=u"Specified rule in key: {0} is not part of allowed rule set : {1}".format(v, allowed),
                error_key=u"matching_rule.not_allowed",
                path=path,
            )
        else:
            self.matching_rule = v

    def init_allow_empty_map(self, v, rule, path):
        """
        """
        log.debug(u"Init allow empty value: %s", path)
        log.debug(u"Type: %s : %s", v, rule)

        self.allowempty_map = v

    def init_type_value(self, v, rule, path):
        """
        """
        log.debug(u"Init type value : %s", path)
        log.debug(u"Type: %s %s", v, rule)

        if v is None:
            v = DEFAULT_TYPE

        self.type = v
        self.type_class = type_class(v)

        if not is_builtin_type(self.type):
            raise RuleError(
                msg=u"Type: {0} is not any of the known types".format(self.type),
                error_key=u"type.unknown",
                path=path,
            )

    def init_matching(self, v, rule, path):
        """
        """
        log.debug(u"Init matching rule : %s", path)

        valid_values = ["any", "all", "*"]

        if str(v) not in valid_values:
            raise RuleError(
                msg=u"matching value: {0} is not one of {1}".format(str(v), valid_values),
                error_key=u"matching_rule.invalid",
                path=path,
            )

        self.matching = str(v)

    def init_name_value(self, v, rule, path):
        """
        """
        log.debug(u"Init name value : %s", path)

        if not is_string(v):
            raise RuleError(
                msg=u"Value: {0} for keyword name must be a string".format(v),
                error_key=u"name.not_string",
                path=path,
            )

        self.name = v

    def init_nullable_value(self, v, rule, path):
        """
        """
        log.debug(u"Init nullable value : %s", path)

        if not isinstance(v, bool):
            raise RuleError(
                msg=u"Value: '{0}' for nullable keyword must be a boolean".format(v),
                error_key=u"nullable.not_bool",
                path=path,
            )

        self.nullable = v

    def init_desc_value(self, v, rule, path):
        """
        """
        log.debug(u"Init descr value : %s", path)

        if not is_string(v):
            raise RuleError(
                msg=u"Value: {0} for keyword desc must be a string".format(v),
                error_key=u"desc.not_string",
                path=path,
            )

        self.desc = v

    def init_required_value(self, v, rule, path):
        """
        """
        log.debug(u"Init required value : %s", path)

        if not is_bool(v):
            raise RuleError(
                msg=u"Value: '{0}' for required keyword must be a boolean".format(v),
                error_key=u"required.not_bool",
                path=path,
            )
        self.required = v

    def init_pattern_value(self, v, rule, path):
        """
        """
        log.debug(u"Init pattern value : %s", path)

        if not is_string(v):
            raise RuleError(
                msg=u"Value of pattern keyword: '{0}' is not a string".format(v),
                error_key=u"pattern.not_string",
                path=path,
            )

        self.pattern = v

        if self.schema_str["type"] == "map":
            raise RuleError(
                msg=u"Keyword pattern is not allowed inside map",
                error_key=u"pattern.not_allowed_in_map",
                path=path,
            )

        # TODO: Some form of validation of the regexp? it exists in the source

        try:
            self.pattern_regexp = re.compile(self.pattern)
        except Exception:
            raise RuleError(
                msg=u"Syntax error when compiling regex pattern: {0}".format(self.pattern_regexp),
                error_key=u"pattern.syntax_error",
                path=path,
            )

    def init_enum_value(self, v, rule, path):
        """
        """
        log.debug(u"Init enum value : %s", path)

        if not isinstance(v, list):
            raise RuleError(
                msg=u"Enum is not a sequence",
                error_key=u"enum.not_seq",
                path=path,
            )
        self.enum = v

        if is_collection_type(self.type):
            raise RuleError(
                msg=u"Enum is not a scalar",
                error_key=u"enum.not_scalar",
                path=path,
            )

        lookup = set()
        for item in v:
            if not isinstance(item, self.type_class):
                raise RuleError(
                    msg=u"Item: '{0}' in enum is not of correct class type: '{1}'".format(item, self.type_class),
                    error_key=u"enum.type.unmatch",
                    path=path,
                )

            if item in lookup:
                raise RuleError(
                    msg=u"Duplicate items: '{0}' found in enum".format(item),
                    error_key=u"enum.duplicate_items",
                    path=path,
                )

            lookup.add(item)

    def init_assert_value(self, v, rule, path):
        """
        """
        log.debug(u"Init assert value : %s", path)

        if not is_string(v):
            raise RuleError(
                msg=u"Value: '{0}' for keyword 'assert' is not a string".format(v),
                error_key=u"assert.not_str",
                path=path,
            )

        self.assertion = v

        if any(k in self.assertion for k in (';', 'import', '__import__')):
            raise RuleError(
                msg=u"Value: '{assertion}' contain invalid content that is not allowed to be present in assertion keyword".format(assertion=self.assertion),
                error_key=u"assert.unsupported_content",
                path=path,
            )

    def init_range_value(self, v, rule, path):
        """
        """
        log.debug(u"Init range value : %s", path)

        supported_types = ["str", "int", "float", "number", "map", "seq"]

        if not isinstance(v, dict):
            raise RuleError(
                msg=u"Range value is not a dict type: '{0}'".format(v),
                error_key=u"range.not_map",
                path=path,
            )

        if self.type not in supported_types:
            raise RuleError(
                msg=u"Range value type: '{0}' is not a supported type".format(self.type),
                error_key=u"range.not_supported_type",
                path=path,
            )

        # dict that should contain min, max, min-ex, max-ex keys
        self.range = v

        # This should validate that only min, max, min-ex, max-ex exists in the dict
        for k, v in self.range.items():
            if k not in ["max", "min", "max-ex", "min-ex"]:
                raise RuleError(
                    msg=u"Unknown key: '{0}' found in range keyword".format(k),
                    error_key=u"range.unknown_key",
                    path=path,
                )

        if "max" in self.range and "max-ex" in self.range:
            raise RuleError(
                msg=u"'max' and 'max-ex' can't be used in the same range rule",
                error_key=u"range.max_duplicate_keywords",
                path=path,
            )

        if "min" in self.range and "min-ex" in self.range:
            raise RuleError(
                msg=u"'min' and 'min-ex' can't be used in the same range rule",
                error_key=u"range.min_duplicate_keywords",
                path=path,
            )

        max = self.range.get("max")
        min = self.range.get("min")
        max_ex = self.range.get("max-ex")
        min_ex = self.range.get("min-ex")

        if max is not None and not is_number(max) or is_bool(max):
            raise RuleError(
                msg=u"Value: '{0}' for 'max' keyword is not a number".format(v),
                error_key=u"range.max.not_number",
                path=path,
            )

        if min is not None and not is_number(min) or is_bool(min):
            raise RuleError(
                msg=u"Value: '{0}' for 'min' keyword is not a number".format(v),
                error_key=u"range.min.not_number",
                path=path,
            )

        if max_ex is not None and not is_number(max_ex) or is_bool(max_ex):
            raise RuleError(
                msg=u"Value: '{0}' for 'max-ex' keyword is not a number".format(v),
                error_key=u"range.max_ex.not_number",
                path=path,
            )

        if min_ex is not None and not is_number(min_ex) or is_bool(min_ex):
            raise RuleError(
                msg=u"Value: '{0}' for 'min-ex' keyword is not a number".format(v),
                error_key=u"range.min_ex.not_number",
                path=path,
            )

        # only numbers allow negative ranges
        # string, map and seq require non negative ranges
        if self.type not in ["int", "float", "number"]:
            if min is not None and min < 0:
                raise RuleError(
                    msg=u"Value for 'min' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"range.min_negative",
                    path=path,
                )
            elif min_ex is not None and min_ex < 0:
                raise RuleError(
                    msg=u"Value for 'min-ex' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"range.min-ex_negative",
                    path=path,
                )
            if max is not None and max < 0:
                raise RuleError(
                    msg=u"Value for 'max' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"range.max_negative",
                    path=path,
                )
            elif max_ex is not None and max_ex < 0:
                raise RuleError(
                    msg=u"Value for 'max-ex' can't be negative in case of type {0}.".format(self.type),
                    error_key=u"range.max-ex_negative",
                    path=path,
                )

        if max is not None:
            if min is not None and max < min:
                raise RuleError(
                    msg=u"Value for 'max' can't be less then value for 'min'. {0} < {1}".format(max, min),
                    error_key=u"range.max_lt_min",
                    path=path,
                )
            elif min_ex is not None and max <= min_ex:
                raise RuleError(
                    msg=u"Value for 'max' can't be less then value for 'min-ex'. {0} <= {1}".format(max, min_ex),
                    error_key=u"range.max_le_min-ex",
                    path=path,
                )
        elif max_ex is not None:
            if min is not None and max_ex < min:
                raise RuleError(
                    msg=u"Value for 'max-ex' can't be less then value for 'min'. {0} < {1}".format(max_ex, min),
                    error_key=u"range.max-ex_le_min",
                    path=path,
                )
            elif min_ex is not None and max_ex <= min_ex:
                raise RuleError(
                    msg=u"Value for 'max-ex' can't be less then value for 'min-ex'. {0} <= {1}".format(max_ex, min_ex),
                    error_key=u"range.max-ex_le_min-ex",
                    path=path,
                )

    def init_ident_value(self, v, rule, path):
        """
        """
        log.debug(u"Init ident value : %s", path)

        if v is None or not is_bool(v):
            raise RuleError(
                msg=u"Value: '{0}' of 'ident' is not a boolean value".format(v),
                error_key=u"ident.not_bool",
                path=path,
            )

        self.ident = bool(v)
        self.required = True

        if is_collection_type(self.type):
            raise RuleError(
                msg=u"Value: '{0}' of 'ident' is not a scalar value".format(v),
                error_key=u"ident.not_scalar",
                path=path,
            )

        if path == "":
            raise RuleError(
                msg=u"Keyword 'ident' can't be on root level of schema",
                error_key=u"ident.not_on_root_level",
                path=path,
            )

        if self.parent is None or not self.parent.type == "map":
            raise RuleError(
                msg=u"Keword 'ident' can't be inside 'map'",
                error_key=u"ident.not_in_map",
                path=path,
            )

    def init_unique_value(self, v, rule, path):
        """
        """
        log.debug(u"Init unique value : %s", path)

        if not is_bool(v):
            raise RuleError(
                msg=u"Value: '{0}' for 'unique' keyword is not boolean".format(v),
                error_key=u"unique.not_bool",
                path=path,
            )

        self.unique = v

        if is_collection_type(self.type):
            raise RuleError(
                msg=u"Type of the value: '{0}' for 'unique' keyword is not a scalar type".format(self.type),
                error_key=u"unique.not_scalar",
                path=path,
            )
        if path == "":
            raise RuleError(
                msg=u"Keyword 'unique' can't be on root level of schema",
                error_key=u"unique.not_on_root_level",
                path=path,
            )

    def init_sequence_value(self, v, rule, path):
        """
        """
        log.debug(u"Init sequence value : %s", path)

        if v is not None and not isinstance(v, list):
            raise RuleError(
                msg=u"Sequence keyword is not a list",
                error_key=u"sequence.not_seq",
                path=path,
            )

        self.sequence = v

        if self.sequence is None or len(self.sequence) == 0:
            raise RuleError(
                msg=u"Sequence contains 0 elements",
                error_key=u"sequence.no_elements",
                path=path,
            )

        tmp_seq = []

        for i, e in enumerate(self.sequence):
            elem = e or {}

            rule = Rule(None, self)
            rule.init(elem, u"{0}/sequence/{1}".format(path, i))

            tmp_seq.append(rule)

        self.sequence = tmp_seq

        return rule

    def init_mapping_value(self, v, rule, path):
        """
        """
        # Check for duplicate use of 'map' and 'mapping'
        if self.mapping:
            raise RuleError(
                msg=u"Keywords 'map' and 'mapping' can't be used on the same level",
                error_key=u"mapping.duplicate_keywords",
                path=path,
            )

        log.debug(u"Init mapping value : %s", path)

        if v is not None and not isinstance(v, dict):
            raise RuleError(
                msg=u"Value for keyword 'map/mapping' is not a dict",
                error_key=u"mapping.not_dict",
                path=path,
            )

        if v is None or len(v) == 0:
            raise RuleError(
                msg=u"Mapping do not contain any elements",
                error_key=u"mapping.no_elements",
                path=path,
            )

        self.mapping = {}
        self.regex_mappings = []

        for k, v in v.items():
            if v is None:
                v = {}

            # Check if this is a regex rule. Handle specially
            if str(k).startswith("regex;") or str(k).startswith("re;"):
                log.debug(u"Found regex map rule")
                regex = k.split(";", 1)
                if len(regex) != 2:
                    raise RuleError(
                        msg=u"Value: '{0}' for keyword regex is malformed".format(k),
                        error_key=u"mapping.regex.malformed",
                        path=path,
                    )

                elif not regex[1].startswith('(') or not regex[1].endswith(')'):
                    raise RuleError(
                        msg=u"Regex '{0}' should start and end with parentheses".format(regex[1]),
                        error_key=u"mapping.regex.missing_parentheses",
                        path=path,
                    )

                else:
                    regex = regex[1]
                    try:
                        re.compile(regex)
                    except Exception as e:
                        log.debug(e)
                        raise RuleError(
                            msg=u"Unable to compile regex '{0}'".format(regex),
                            error_key=u"mapping.regex.compile_error",
                            path=path,
                        )

                    regex_rule = Rule(None, self)
                    regex_rule.init(v, u"{0}/mapping;regex/{1}".format(path, regex[1:-1]))
                    regex_rule.map_regex_rule = regex[1:-1]
                    self.regex_mappings.append(regex_rule)
                    self.mapping[k] = regex_rule
            else:
                rule = Rule(None, self)
                rule.init(v, u"{0}/mapping/{1}".format(path, k))
                self.mapping[k] = rule

        return rule

    def init_default_value(self, v, rule, path):
        """
        """
        log.debug(u"Init default value : %s", path)
        self.default = v

        if is_collection_type(self.type):
            raise RuleError(
                msg=u"Value: {0} for keyword 'default' is not a scalar type".format(v),
                error_key=u"default.not_scalar",
                path=path,
            )

        if self.type == "map" or self.type == "seq":
            raise RuleError(
                msg=u"Value: {0} for keyword 'default' is not a scalar type".format(v),
                error_key=u"default.not_scalar",
                path=path,
            )

        if not isinstance(v, self.type_class):
            raise RuleError(
                msg=u"Types do not match: '{0}' --> '{1}'".format(v, self.type_class),
                error_key=u"default.type.unmatch",
                path=path,
            )

    def check_type_keywords(self, schema, rule, path):
        """
        All supported keywords:
         - allowempty_map
         - assertion
         - class
         - date
         - default
         - desc
         - enum
         - example
         - extensions
         - func
         - ident
         - include_name
         - map_regex_rule
         - mapping
         - matching
         - matching_rule
         - name
         - nullable
         - pattern
         - pattern_regexp
         - range
         - regex_mappings
         - required
         - schema
         - sequence
         - type
         - type_class
         - unique
         - version
        """
        if not self.strict_rule_validation:
            return

        global_keywords = ['type', 'desc', 'example', 'extensions', 'name', 'nullable', 'version', 'func', 'include']
        all_allowed_keywords = {
            'str': global_keywords + ['default', 'pattern', 'range', 'enum', 'required', 'unique', 'req'],
            'int': global_keywords + ['default', 'range', 'enum', 'required', 'unique'],
            'float': global_keywords + ['default', 'enum', 'range', 'required'],
            'number': global_keywords + ['default', 'enum'],
            'bool': global_keywords + ['default', 'enum'],
            'map': global_keywords + ['allowempty_map', 'mapping', 'map', 'allowempty', 'required', 'matching-rule', 'range', 'class'],
            'seq': global_keywords + ['sequence', 'seq', 'required', 'range', 'matching'],
            'sequence': global_keywords + ['sequence', 'seq', 'required'],
            'mapping': global_keywords + ['mapping', 'seq', 'required'],
            'timestamp': global_keywords + ['default', 'enum'],
            'date': global_keywords + ['default', 'enum'],
            'symbol': global_keywords + ['default', 'enum'],
            'scalar': global_keywords + ['default', 'enum'],
            'text': global_keywords + ['default', 'enum', 'pattern'],
            'any': global_keywords + ['default', 'enum'],
            'enum': global_keywords + ['default', 'enum'],
            'none': global_keywords + ['default', 'enum', 'required'],
        }
        rule_type = schema.get('type')
        if not rule_type:
            # Special cases for the "shortcut methods"
            if 'sequence' in schema or 'seq' in schema:
                rule_type = 'sequence'
            elif 'mapping' in schema or 'map' in schema:
                rule_type = 'mapping'

        allowed_keywords = all_allowed_keywords.get(rule_type)
        if not allowed_keywords and 'sequence' not in schema and 'mapping' not in schema and 'seq' not in schema and 'map' not in schema:
            raise RuleError('No allowed keywords found for type: {0}'.format(rule_type))

        for k, v in schema.items():
            if k not in allowed_keywords:
                raise RuleError('Keyword "{0}" is not supported for type: "{1}" '.format(k, rule_type))

    def check_conflicts(self, schema, rule, path):
        """
        """
        log.debug(u"Checking for conflicts : %s", path)

        if self.type == "seq":
            if all(sa not in schema for sa in sequence_aliases):
                raise SchemaConflict(
                    msg="Type is sequence but no sequence alias found on same level",
                    error_key=u"seq.no_sequence",
                    path=path,
                )

            if self.enum is not None:
                raise SchemaConflict(
                    msg="Sequence and enum can't be on the same level in the schema",
                    error_key=u"seq.conflict.enum",
                    path=path,
                )

            if self.pattern is not None:
                raise SchemaConflict(
                    msg="Sequence and pattern can't be on the same level in the schema",
                    error_key=u"seq.conflict.pattern",
                    path=path,
                )

            if self.mapping is not None:
                raise SchemaConflict(
                    msg="Sequence and mapping can't be on the same level in the schema",
                    error_key=u"seq.conflict.mapping",
                    path=path,
                )
        elif self.type == "map":
            if all(ma not in schema for ma in mapping_aliases) and not self.allowempty_map:
                raise SchemaConflict(
                    msg="Type is mapping but no mapping alias found on same level",
                    error_key=u"map.no_mapping",
                    path=path,
                )

            if self.enum is not None:
                raise SchemaConflict(
                    msg="Mapping and enum can't be on the same level in the schema",
                    error_key=u"map.conflict.enum",
                    path=path,
                )

            if self.sequence is not None:
                raise SchemaConflict(
                    msg="Mapping and sequence can't be on the same level in the schema",
                    error_key=u"map.conflict.sequence",
                    path=path,
                )
        else:
            if self.sequence is not None:
                raise SchemaConflict(
                    msg="Scalar and sequence can't be on the same level in the schema",
                    error_key=u"scalar.conflict.sequence",
                    path=path,
                )

            if self.mapping is not None:
                raise SchemaConflict(
                    msg="Scalar and mapping can't be on the same level in the schema",
                    error_key=u"scalar.conflict.mapping",
                    path=path,
                )

            if self.enum is not None and self.range is not None:
                raise SchemaConflict(
                    msg="Enum and range can't be on the same level in the schema",
                    error_key=u"enum.conflict.range",
                    path=path,
                )
