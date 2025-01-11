# -*- coding: utf-8 -*-

""" pyKwalify - core.py """

# python std lib
import datetime
import json
import logging
import os
import re
import sys
import traceback
import time
from io import open
from importlib.machinery import SourceFileLoader

# pyKwalify imports
import pykwalify
from pykwalify.compat import unicode, nativestr, basestring
from pykwalify.errors import CoreError, SchemaError, NotMappingError, NotSequenceError
from pykwalify.rule import Rule
from pykwalify.types import is_scalar, is_string, tt

# 3rd party imports
from dateutil.parser import parse
from pykwalify.compat import yml
from ruamel.yaml.constructor import Constructor

log = logging.getLogger(__name__)


class Core(object):
    """ Core class of pyKwalify """

    def __init__(self, source_file=None, schema_files=None, source_data=None, schema_data=None, extensions=None, strict_rule_validation=False,
                 fix_ruby_style_regex=False, allow_assertions=False, file_encoding=None, schema_file_obj=None, data_file_obj=None):
        """
        :param extensions:
            List of paths to python files that should be imported and available via 'func' keywork.
            This list of extensions can be set manually or they should be provided by the `--extension`
            flag from the cli. This list should not contain files specified by the `extensions` list keyword
            that can be defined at the top level of the schema.
        """
        if schema_files is None:
            schema_files = []
        if extensions is None:
            extensions = []

        log.debug(u"source_file: %s", source_file)
        log.debug(u"schema_file: %s", schema_files)
        log.debug(u"source_data: %s", source_data)
        log.debug(u"schema_data: %s", schema_data)
        log.debug(u"extension files: %s", extensions)

        self.source = None
        self.schema = None
        self.validation_errors = None
        self.validation_errors_exceptions = None
        self.root_rule = None
        self.extensions = extensions
        self.errors = []
        self.strict_rule_validation = strict_rule_validation
        self.fix_ruby_style_regex = fix_ruby_style_regex
        self.allow_assertions = allow_assertions

        # Patch in all the normal python types into the yaml load instance so we can use all the
        # internal python types in the yaml loading.
        yml.constructor.add_constructor('tag:yaml.org,2002:python/bool', Constructor.construct_yaml_bool)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/complex', Constructor.construct_python_complex)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/dict', Constructor.construct_yaml_map)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/float', Constructor.construct_yaml_float)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/int', Constructor.construct_yaml_int)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/list', Constructor.construct_yaml_seq)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/long', Constructor.construct_python_long)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/none', Constructor.construct_yaml_null)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/str', Constructor.construct_python_str)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/tuple', Constructor.construct_python_tuple)
        yml.constructor.add_constructor('tag:yaml.org,2002:python/unicode', Constructor.construct_python_unicode)

        if data_file_obj:
            try:
                self.source = yml.load(data_file_obj.read())
            except Exception as e:
                raise CoreError("Unable to load data_file_obj input")

        if schema_file_obj:
            try:
                self.schema = yml.load(schema_file_obj.read())
            except Exception as e:
                raise CoreError("Unable to load schema_file_obj")

        if source_file is not None:
            if not os.path.exists(source_file):
                raise CoreError(u"Provided source_file do not exists on disk: {0}".format(source_file))

            with open(source_file, "r", encoding=file_encoding) as stream:
                if source_file.endswith(".json"):
                    self.source = json.load(stream)
                elif source_file.endswith(".yaml") or source_file.endswith('.yml'):
                    self.source = yml.load(stream)
                else:
                    raise CoreError(u"Unable to load source_file. Unknown file format of specified file path: {0}".format(source_file))

        if not isinstance(schema_files, list):
            raise CoreError(u"schema_files must be of list type")

        # Merge all schema files into one single file for easy parsing
        if len(schema_files) > 0:
            schema_data = {}
            for f in schema_files:
                if not os.path.exists(f):
                    raise CoreError(u"Provided source_file do not exists on disk : {0}".format(f))

                with open(f, "r", encoding=file_encoding) as stream:
                    if f.endswith(".json"):
                        data = json.load(stream)
                    elif f.endswith(".yaml") or f.endswith(".yml"):
                        data = yml.load(stream)
                        if not data:
                            raise CoreError(u"No data loaded from file : {0}".format(f))
                    else:
                        raise CoreError(u"Unable to load file : {0} : Unknown file format. Supported file endings is [.json, .yaml, .yml]")

                    for key in data.keys():
                        if key in schema_data.keys():
                            raise CoreError(u"Parsed key : {0} : two times in schema files...".format(key))

                    schema_data = dict(schema_data, **data)

            self.schema = schema_data

        # Nothing was loaded so try the source_data variable
        if self.source is None:
            log.debug(u"No source file loaded, trying source data variable")
            self.source = source_data
        if self.schema is None:
            log.debug(u"No schema file loaded, trying schema data variable")
            self.schema = schema_data

        # Test if anything was loaded
        if self.source is None:
            raise CoreError(u"No source file/data was loaded")
        if self.schema is None:
            raise CoreError(u"No schema file/data was loaded")

        # Merge any extensions defined in the schema with the provided list of extensions from the cli
        for f in self.schema.get('extensions', []):
            self.extensions.append(f)

        if not isinstance(self.extensions, list) and all(isinstance(e, str) for e in self.extensions):
            raise CoreError(u"Specified extensions must be a list of file paths")

        self._load_extensions()

        if self.strict_rule_validation:
            log.info("Using strict rule keywords validation...")

    def _load_extensions(self):
        """
        Load all extension files into the namespace pykwalify.ext
        """
        log.debug(u"loading all extensions : %s", self.extensions)

        self.loaded_extensions = []

        for f in self.extensions:
            if not os.path.isabs(f):
                f = os.path.abspath(f)

            if not os.path.exists(f):
                raise CoreError(u"Extension file: {0} not found on disk".format(f))

            self.loaded_extensions.append(SourceFileLoader("", f).load_module())

        log.debug(self.loaded_extensions)
        log.debug([dir(m) for m in self.loaded_extensions])

    def validate(self, raise_exception=True):
        """
        """
        log.debug(u"starting core")

        self._start_validate(self.source)
        self.validation_errors = [unicode(error) for error in self.errors]
        self.validation_errors_exceptions = self.errors

        if self.errors is None or len(self.errors) == 0:
            log.info(u"validation.valid")
        else:
            log.error(u"validation.invalid")
            log.error(u" --- All found errors ---")
            log.error(self.validation_errors)
            if raise_exception:
                raise SchemaError(u"Schema validation failed:\n - {error_msg}.".format(
                    error_msg=u'.\n - '.join(self.validation_errors)))
            else:
                log.error(u"Errors found but will not raise exception...")

        # Return validated data
        return self.source

    def _start_validate(self, value=None):
        """
        """
        path = ""
        self.errors = []
        done = []

        s = {}

        # Look for schema; tags so they can be parsed before the root rule is parsed
        for k, v in self.schema.items():
            if k.startswith("schema;"):
                log.debug(u"Found partial schema; : %s", v)
                r = Rule(schema=v)
                log.debug(u" Partial schema : %s", r)
                pykwalify.partial_schemas[k.split(";", 1)[1]] = r
            else:
                # readd all items that is not schema; so they can be parsed
                s[k] = v

        self.schema = s

        log.debug(u"Building root rule object")
        root_rule = Rule(schema=self.schema)
        self.root_rule = root_rule
        log.debug(u"Done building root rule")
        log.debug(u"Root rule: %s", self.root_rule)

        self._validate(value, root_rule, path, done)

    def _validate(self, value, rule, path, done):
        """
        """
        log.debug(u"Core validate")
        log.debug(u" Root validate : Rule: %s", rule)
        log.debug(u" Root validate : Rule_type: %s", rule.type)
        log.debug(u" Root validate : Seq: %s", rule.sequence)
        log.debug(u" Root validate : Map: %s", rule.mapping)
        log.debug(u" Root validate : Done: %s", done)

        if rule.required and value is None and not rule.type == 'none':
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"required.novalue : '{path}'",
                path=path,
                value=value.encode('unicode_escape') if value else value,
            ))
            return

        if not rule.nullable and value is None and not rule.type == 'none':
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"nullable.novalue : '{path}'",
                path=path,
                value=value.encode('unicode_escape') if value else value,
            ))
            return

        log.debug(u" ? ValidateRule: %s", rule)
        if rule.include_name is not None:
            self._validate_include(value, rule, path, done=None)
        elif rule.sequence is not None:
            self._validate_sequence(value, rule, path, done=None)
        elif rule.mapping is not None or rule.allowempty_map:
            self._validate_mapping(value, rule, path, done=None)
        else:
            self._validate_scalar(value, rule, path, done=None)

    def _handle_func(self, value, rule, path, done=None):
        """
        Helper function that should check if func is specified for this rule and
        then handle it for all cases in a generic way.
        """
        func = rule.func

        # func keyword is not defined so nothing to do
        if not func:
            return

        found_method = False

        for extension in self.loaded_extensions:
            method = getattr(extension, func, None)
            if method:
                found_method = True

                # No exception will should be caught. If one is raised it should bubble up all the way.
                ret = method(value, rule, path)
                if ret is not True and ret is not None:
                    msg = '%s. Path: {path}' % unicode(ret)
                    self.errors.append(SchemaError.SchemaErrorEntry(
                                    msg=msg,
                                    path=path,
                                    value=None))

                # If False or None or some other object that is interpreted as False
                if not ret:
                    raise CoreError(u"Error when running extension function : {0}".format(func))

                # Only run the first matched function. Sinc loading order is determined
                # it should be easy to determine which file is used before others
                break

        if not found_method:
            raise CoreError(u"Did not find method '{0}' in any loaded extension file".format(func))

    def _validate_include(self, value, rule, path, done=None):
        """
        """
        # TODO: It is difficult to get a good test case to trigger this if case
        if rule.include_name is None:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u'Include name not valid',
                path=path,
                value=value.encode('unicode_escape')))
            return
        include_name = rule.include_name
        partial_schema_rule = pykwalify.partial_schemas.get(include_name)
        if not partial_schema_rule:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Cannot find partial schema with name '{include_name}'. Existing partial schemas: '{existing_schemas}'. Path: '{path}'",
                path=path,
                value=value,
                include_name=include_name,
                existing_schemas=", ".join(sorted(pykwalify.partial_schemas.keys()))))
            return

        self._validate(value, partial_schema_rule, path, done)

    def _validate_sequence(self, value, rule, path, done=None):
        """
        """
        log.debug(u"Core Validate sequence")
        log.debug(u" Sequence : Data: %s", value)
        log.debug(u" Sequence : Rule: %s", rule)
        log.debug(u" Sequence : RuleType: %s", rule.type)
        log.debug(u" Sequence : Path: %s", path)
        log.debug(u" Sequence : Seq: %s", rule.sequence)
        log.debug(u" Sequence : Map: %s", rule.mapping)

        if len(rule.sequence) <= 0:
            raise CoreError(u"Sequence must contains atleast one item : {0}".format(path))

        if value is None:
            log.debug(u" * Core seq: sequence data is None")
            return

        if not isinstance(value, list):
            if isinstance(value, str):
                value = value.encode('unicode_escape')
            self.errors.append(SchemaError.SchemaErrorEntry(
                u"Value '{value}' is not a list. Value path: '{path}'",
                path,
                value,
            ))
            return

        # Handle 'func' argument on this sequence
        self._handle_func(value, rule, path, done)

        ok_values = []
        error_tracker = []

        unique_errors = {}
        map_unique_errors = {}

        for i, item in enumerate(value):
            processed = []

            for r in rule.sequence:
                tmp_errors = []

                try:
                    # Create a sub core object to enable error tracking that do not
                    #  collide with this Core objects errors
                    tmp_core = Core(source_data={}, schema_data={})
                    tmp_core.fix_ruby_style_regex = self.fix_ruby_style_regex
                    tmp_core.allow_assertions = self.allow_assertions
                    tmp_core.strict_rule_validation = self.strict_rule_validation
                    tmp_core.loaded_extensions = self.loaded_extensions
                    tmp_core._validate(item, r, "{0}/{1}".format(path, i), done)
                    tmp_errors = tmp_core.errors
                except NotMappingError:
                    # For example: If one type was specified as 'map' but data
                    # was 'str' a exception will be thrown but we should ignore it
                    pass
                except NotSequenceError:
                    # For example: If one type was specified as 'seq' but data
                    # was 'str' a exception will be thrown but we shold ignore it
                    pass

                processed.append(tmp_errors)

                if r.type == "map":
                    log.debug(u" * Found map inside sequence")
                    unique_keys = []

                    if r.mapping is None:
                        log.debug(u" + No rule to apply, prolly because of allowempty: True")
                        return

                    for k, _rule in r.mapping.items():
                        log.debug(u" * Key: %s", k)
                        log.debug(u" * Rule: %s", _rule)

                        if _rule.unique or _rule.ident:
                            unique_keys.append(k)

                    if len(unique_keys) > 0:
                        for v in unique_keys:
                            table = {}
                            for j, V in enumerate(value):
                                # If key do not exists it should be ignored by unique because that is not a broken constraint
                                val = V.get(v, None)

                                if val is None:
                                    continue

                                if val in table:
                                    curr_path = "{0}/{1}/{2}".format(path, j, v)
                                    prev_path = "{0}/{1}/{2}".format(path, table[val], v)
                                    s = SchemaError.SchemaErrorEntry(
                                        msg=u"Value '{duplicate}' is not unique. Previous path: '{prev_path}'. Path: '{path}'",
                                        path=curr_path,
                                        value=value,
                                        duplicate=val,
                                        prev_path=prev_path,
                                    )
                                    map_unique_errors[s.__repr__()] = s
                                else:
                                    table[val] = j
                elif r.unique:
                    log.debug(u" * Found unique value in sequence")
                    table = {}

                    for j, val in enumerate(value):
                        if val is None:
                            continue

                        if val in table:
                            curr_path = "{0}/{1}".format(path, j)
                            prev_path = "{0}/{1}".format(path, table[val])
                            s = SchemaError.SchemaErrorEntry(
                                msg=u"Value '{duplicate}' is not unique. Previous path: '{prev_path}'. Path: '{path}'",
                                path=curr_path,
                                value=value,
                                duplicate=val,
                                prev_path=prev_path,
                            )
                            unique_errors[s.__repr__()] = s
                        else:
                            table[val] = j

            error_tracker.append(processed)
            no_errors = []
            for _errors in processed:
                no_errors.append(len(_errors) == 0)

            if rule.matching == "any":
                log.debug(u" * any rule %s", True in no_errors)
                ok_values.append(True in no_errors)
            elif rule.matching == "all":
                log.debug(u" * all rule".format(all(no_errors)))
                ok_values.append(all(no_errors))
            elif rule.matching == "*":
                log.debug(u" * star rule", "...")
                ok_values.append(True)

        for _error in unique_errors:
            self.errors.append(_error)

        for _error in map_unique_errors:
            self.errors.append(_error)

        log.debug(u" * ok : %s", ok_values)

        # All values must pass the validation, otherwise add the parsed errors
        # to the global error list and throw up some error.
        if not all(ok_values):
            # Ignore checking for '*' type because it should allways go through
            if rule.matching == "any":
                log.debug(u" * Value: %s did not validate against one or more sequence schemas", value)
            elif rule.matching == "all":
                log.debug(u" * Value: %s did not validate against all possible sequence schemas", value)

            for i, is_ok in enumerate(ok_values):
                if not is_ok:
                    for error in error_tracker[i]:
                        for e in error:
                            self.errors.append(e)

        log.debug(u" * Core seq: validation recursivley done...")

        if rule.range is not None:
            rr = rule.range

            self._validate_range(
                rr.get("max"),
                rr.get("min"),
                rr.get("max-ex"),
                rr.get("min-ex"),
                len(value),
                path,
                "seq",
            )

    def _validate_mapping(self, value, rule, path, done=None):
        """
        """
        log.debug(u"Validate mapping")
        log.debug(u" Mapping : Data: %s", value)
        log.debug(u" Mapping : Rule: %s", rule)
        log.debug(u" Mapping : RuleType: %s", rule.type)
        log.debug(u" Mapping : Path: %s", path)
        log.debug(u" Mapping : Seq: %s", rule.sequence)
        log.debug(u" Mapping : Map: %s", rule.mapping)

        if not isinstance(value, dict):
            self.errors.append(SchemaError.SchemaErrorEntry(
                u"Value '{value}' is not a dict. Value path: '{path}'",
                path,
                value,
            ))
            return

        if rule.mapping is None:
            log.debug(u" + No rule to apply, prolly because of allowempty: True")
            return

        # Handle 'func' argument on this mapping
        self._handle_func(value, rule, path, done)

        m = rule.mapping
        log.debug(u"   Mapping: Rule-Mapping: %s", m)

        if rule.range is not None:
            r = rule.range

            self._validate_range(
                r.get("max"),
                r.get("min"),
                r.get("max-ex"),
                r.get("min-ex"),
                len(value),
                path,
                "map",
            )

        for k, rr in m.items():
            # Handle if the value of the key contains a include keyword
            if rr.include_name is not None:
                include_name = rr.include_name
                partial_schema_rule = pykwalify.partial_schemas.get(include_name)

                if not partial_schema_rule:
                    self.errors.append(SchemaError.SchemaErrorEntry(
                        msg=u"Cannot find partial schema with name '{include_name}'. Existing partial schemas: '{existing_schemas}'. Path: '{path}'",
                        path=path,
                        value=value,
                        include_name=include_name,
                        existing_schemas=", ".join(sorted(pykwalify.partial_schemas.keys()))))
                    return

                rr = partial_schema_rule

            # Find out if this is a regex rule
            is_regex_rule = False
            required_regex = ""
            for regex_rule in rule.regex_mappings:
                if k == "regex;({})".format(regex_rule.map_regex_rule) or k == "re;({})".format(regex_rule.map_regex_rule):
                    is_regex_rule = True
                    required_regex = regex_rule.map_regex_rule

            # Check for the presense of the required key
            is_present = False
            if not is_regex_rule:
                is_present = k in value
            else:
                is_present = any([re.search(required_regex, str(v)) for v in value])

            # Specifying =: as key is considered the "default" if no other keys match
            if rr.required and not is_present and k != "=":
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Cannot find required key '{key}'. Path: '{path}'",
                    path=path,
                    value=value,
                    key=k))
            if k not in value and rr.default is not None:
                value[k] = rr.default

        for k, v in value.items():
            # If no other case was a match, check if a default mapping is valid/present and use
            # that one instead
            r = m.get(k, m.get('='))
            log.debug(u"  Mapping-value : %s", m)
            log.debug(u"  Mapping-value : %s %s", k, v)
            log.debug(u"  Mapping-value : %s", r)

            regex_mappings = [(regex_rule, re.search(regex_rule.map_regex_rule, str(k))) for regex_rule in rule.regex_mappings]
            log.debug(u"  Mapping-value: Mapping Regex matches: %s", regex_mappings)

            if r is not None:
                # validate recursively
                log.debug(u"  Mapping-value: Core Map: validate recursively: %s", r)
                self._validate(v, r, u"{0}/{1}".format(path, k), done)
            elif any(regex_mappings):
                sub_regex_result = []

                # Found at least one that matches a mapping regex
                for mm in regex_mappings:
                    if mm[1]:
                        log.debug(u"  Mapping-value: Matching regex patter: %s", mm[0])
                        self._validate(v, mm[0], "{0}/{1}".format(path, k), done)
                        sub_regex_result.append(True)
                    else:
                        sub_regex_result.append(False)

                if rule.matching_rule == "any":
                    if any(sub_regex_result):
                        log.debug(u"  Mapping-value: Matched at least one regex")
                    else:
                        log.debug(u"  Mapping-value: No regex matched")
                        self.errors.append(SchemaError.SchemaErrorEntry(
                            msg=u"Key '{key}' does not match any regex '{regex}'. Path: '{path}'",
                            path=path,
                            value=value,
                            key=k,
                            regex="' or '".join(sorted([mm[0].map_regex_rule for mm in regex_mappings]))))
                elif rule.matching_rule == "all":
                    if all(sub_regex_result):
                        log.debug(u"  Mapping-value: Matched all regex rules")
                    else:
                        log.debug(u"  Mapping-value: Did not match all regex rules")
                        self.errors.append(SchemaError.SchemaErrorEntry(
                            msg=u"Key '{key}' does not match all regex '{regex}'. Path: '{path}'",
                            path=path,
                            value=value,
                            key=k,
                            regex="' and '".join(sorted([mm[0].map_regex_rule for mm in regex_mappings]))))
                else:
                    log.debug(u"  Mapping-value: No mapping rule defined")
            else:
                if not rule.allowempty_map:
                    self.errors.append(SchemaError.SchemaErrorEntry(
                        msg=u"Key '{key}' was not defined. Path: '{path}'",
                        path=path,
                        value=value,
                        key=k))

    def _validate_scalar(self, value, rule, path, done=None):
        """
        """
        log.debug(u"Validate scalar")
        log.debug(u" Scalar : Value : %s", value)
        log.debug(u" Scalar : Rule :  %s", rule)
        log.debug(u" Scalar : RuleType : %s", rule.type)
        log.debug(u" Scalar : Path %s", path)

        # Handle 'func' argument on this scalar
        self._handle_func(value, rule, path, done)

        if rule.assertion is not None:
            self._validate_assert(rule, value, path)

        if value is None:
            return True

        if rule.enum is not None and value not in rule.enum:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Enum '{value}' does not exist. Path: '{path}' Enum: {enum_values}",
                path=path,
                value=nativestr(value) if tt['str'](value) else value,
                enum_values=rule.enum,
            ))

        # Set default value
        if rule.default and value is None:
            value = rule.default

        if not self._validate_scalar_type(value, rule.type, path):
            return

        if value is None:
            return

        if rule.pattern is not None:
            #
            # Try to trim away the surrounding slashes around ruby style /<regex>/ if they are defined.
            # This is a quirk from ruby that they define regex patterns with surrounding slashes.
            # Docs on how ruby regex works can be found here: https://ruby-doc.org/core-2.4.0/Regexp.html
            # The original ruby implementation uses this code to validate patterns
            #   unless value.to_s =~ rule.regexp
            # Becuase python do not work with surrounding slashes we have to trim them away in order to make the regex work
            #
            if rule.pattern.startswith('/') and rule.pattern.endswith('/') and self.fix_ruby_style_regex:
                rule.pattern = rule.pattern[1:-1]
                log.debug("Trimming slashes around ruby style regex. New pattern value: '{0}'".format(rule.pattern))

            try:
                log.debug("Matching pattern '{0}' to regex '{1}".format(rule.pattern, value))
                res = re.match(rule.pattern, value, re.UNICODE)
            except TypeError:
                res = None

            if res is None:  # Not matching
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Value '{value}' does not match pattern '{pattern}'. Path: '{path}'",
                    path=path,
                    value=nativestr(str(value)),
                    pattern=rule._pattern))
            else:
                log.debug("Pattern matched...")

        if rule.range is not None:
            if not is_scalar(value):
                raise CoreError(u"value is not a valid scalar")

            r = rule.range

            try:
                v = len(value)
                value = v
            except Exception:
                pass

            self._validate_range(
                r.get("max"),
                r.get("min"),
                r.get("max-ex"),
                r.get("min-ex"),
                value,
                path,
                "scalar",
            )

        if rule.length is not None:
            self._validate_length(
                rule.length,
                value,
                path,
                'scalar',
            )

        # Validate timestamp
        if rule.type == "timestamp":
            self._validate_scalar_timestamp(value, path)

        if rule.type == "date":
            if not is_scalar(value):
                raise CoreError(u'value is not a valid scalar')
            date_format = rule.format
            self._validate_scalar_date(value, date_format, path)

    def _validate_scalar_timestamp(self, timestamp_value, path):
        """
        """
        def _check_int_timestamp_boundaries(timestamp):
            """
            """
            if timestamp < 1:
                # Timestamp integers can't be negative
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Integer value of timestamp can't be below 0",
                    path=path,
                    value=timestamp,
                    timestamp=str(timestamp),
                ))
            if timestamp > 2147483647:
                # Timestamp integers can't be above the upper limit of
                # 32 bit integers
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Integer value of timestamp can't be above 2147483647",
                    path=path,
                    value=timestamp,
                    timestamp=str(timestamp),
                ))

        if isinstance(timestamp_value, (int, float)):
            _check_int_timestamp_boundaries(timestamp_value)
        elif isinstance(timestamp_value, datetime.datetime):
            # Datetime objects currently have nothing to validate.
            # In the future, more options will be added to datetime validation
            pass
        elif isinstance(timestamp_value, basestring):
            v = timestamp_value.strip()

            # parse("") will give a valid date but it should not be
            # considered a valid timestamp
            if v == "":
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Timestamp value is empty. Path: '{path}'",
                    path=path,
                    value=nativestr(timestamp_value),
                    timestamp=nativestr(timestamp_value)))
            else:
                # A string can contain a valid unit timestamp integer. Check if it is valid and validate it
                try:
                    int_v = int(v)
                    _check_int_timestamp_boundaries(int_v)
                except ValueError:
                    # Just continue to parse it as a timestamp
                    try:
                        parse(timestamp_value)
                        # If it can be parsed then it is valid
                    except Exception:
                        self.errors.append(SchemaError.SchemaErrorEntry(
                            msg=u"Timestamp: '{timestamp}'' is invalid. Path: '{path}'",
                            path=path,
                            value=nativestr(timestamp_value),
                            timestamp=nativestr(timestamp_value)))
        else:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Not a valid timestamp",
                path=path,
                value=timestamp_value,
                timestamp=timestamp_value,
            ))

    def _validate_scalar_date(self, date_value, date_formats, path):
        log.debug(u"Validate date : %(value)s : %(format)s : %(path)s" % {
            'value': date_value,
            'format': date_formats,
            'path': path,
        })

        if isinstance(date_value, str):
            # If a date_format is specefied then use strptime on all formats
            # If no date_format is specefied then use dateutils.parse() to test the value
            log.debug(date_formats)

            if date_formats:
                # Run through all date_formats and it is valid if atleast one of them passed time.strptime() parsing
                valid = False
                for date_format in date_formats:
                    try:
                        time.strptime(date_value, date_format)
                        valid = True
                    except ValueError:
                        pass

                if not valid:
                    self.errors.append(SchemaError.SchemaErrorEntry(
                        msg=u"Not a valid date: {value} format: {format}. Path: '{path}'",
                        path=path,
                        value=date_value,
                        format=date_format,
                    ))
                    return
            else:
                try:
                    parse(date_value)
                except ValueError:
                    self.errors.append(SchemaError.SchemaErrorEntry(
                        msg=u"Not a valid date: {value} Path: '{path}'",
                        path=path,
                        value=date_value,
                    ))
        elif isinstance(date_value, (datetime.date, datetime.datetime)):
            # If the object already is a datetime or date object it passes validation
            pass
        else:
            # If value is any other type then raise error
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Not a valid date: {value} date must be a string or a datetime.date not a '{type}'",
                path=path,
                value=date_value,
                type=type(date_value).__name__,
            ))

    def _validate_length(self, rule, value, path, prefix):
        if not is_string(value):
            raise CoreError("Value: '{0}' must be a 'str' type for length check to work".format(value))

        value_length = len(str(value))
        max_, min_, max_ex, min_ex = rule.get('max'), rule.get('min'), rule.get('max-ex'), rule.get('min-ex')

        log.debug(
            u"Validate length : %s : %s : %s : %s : %s : %s",
            max, min, max_ex, min_ex, value, path,
        )

        if max_ is not None and max_ < value_length:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Value: '{value_str}' has length of '{value}', greater than max limit '{max_}'. Path: '{path}'",
                value_str=value,
                path=path,
                value=len(value),
                prefix=prefix,
                max_=max_))

        if min_ is not None and min_ > value_length:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Value: '{value_str}' has length of '{value}', greater than min limit '{min_}'. Path: '{path}'",
                value_str=value,
                path=path,
                value=len(value),
                prefix=prefix,
                min_=min_))

        if max_ex is not None and max_ex <= value_length:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Value: '{value_str}' has length of '{value}', greater than max_ex limit '{max_ex}'. Path: '{path}'",
                value_str=value,
                path=path,
                value=len(value),
                prefix=prefix,
                max_ex=max_ex))

        if min_ex is not None and min_ex >= value_length:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Value: '{value_str}' has length of '{value}', greater than min_ex limit '{min_ex}'. Path: '{path}'",
                value_str=value,
                path=path,
                value=len(value),
                prefix=prefix,
                min_ex=min_ex))

    def _validate_assert(self, rule, value, path):
        if not self.allow_assertions:
            raise CoreError('To allow usage of keyword "assert" you must use cli flag "--allow-assertions" or set the keyword "allow_assert" in Core class')

        # Small hack to make strings work as a value.
        if isinstance(value, str):
            assert_value_str = '"{0}"'.format(value)
        else:
            assert_value_str = '{0}'.format(value)

        assertion_string = "val = {0}; assert {1}".format(assert_value_str, rule.assertion)
        try:
            exec(assertion_string, {}, {})
        except AssertionError:
            self.errors.append(SchemaError.SchemaErrorEntry(
                msg=u"Value: '{0}' assertion expression failed ({1})".format(value, rule.assertion),
                path=path,
                value=value,
            ))
            return
        except Exception as err:
            error_class = err.__class__.__name__
            detail = err.args[0]
            cl, exc, tb = sys.exc_info()
            line_number = traceback.extract_tb(tb)[-1][1]
            raise Exception("Unknown error during assertion\n{0}\n{1}\n{2}\n{3}\n{4}\n{5}".format(
                error_class, detail, cl, exc, tb, line_number,
            ))

    def _validate_range(self, max_, min_, max_ex, min_ex, value, path, prefix):
        """
        Validate that value is within range values.
        """
        if not isinstance(value, int) and not isinstance(value, float):
            raise CoreError("Value must be a integer type")

        log.debug(
            u"Validate range : %s : %s : %s : %s : %s : %s",
            max_,
            min_,
            max_ex,
            min_ex,
            value,
            path,
        )

        if max_ is not None and max_ < value:
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Type '{prefix}' has size of '{value}', greater than max limit '{max_}'. Path: '{path}'",
                    path=path,
                    value=nativestr(value) if tt['str'](value) else value,
                    prefix=prefix,
                    max_=max_))

        if min_ is not None and min_ > value:
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Type '{prefix}' has size of '{value}', less than min limit '{min_}'. Path: '{path}'",
                    path=path,
                    value=nativestr(value) if tt['str'](value) else value,
                    prefix=prefix,
                    min_=min_))

        if max_ex is not None and max_ex <= value:
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Type '{prefix}' has size of '{value}', greater than or equals to max limit(exclusive) '{max_ex}'. Path: '{path}'",
                    path=path,
                    value=nativestr(value) if tt['str'](value) else value,
                    prefix=prefix,
                    max_ex=max_ex))

        if min_ex is not None and min_ex >= value:
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Type '{prefix}' has size of '{value}', less than or equals to min limit(exclusive) '{min_ex}'. Path: '{path}'",
                    path=path,
                    value=nativestr(value) if tt['str'](value) else value,
                    prefix=prefix,
                    min_ex=min_ex))

    def _validate_scalar_type(self, value, t, path):
        """
        """
        log.debug(u" # Core scalar: validating scalar type : %s", t)
        log.debug(u" # Core scalar: scalar type: %s", type(value))

        try:
            if not tt[t](value):
                self.errors.append(SchemaError.SchemaErrorEntry(
                    msg=u"Value '{value}' is not of type '{scalar_type}'. Path: '{path}'",
                    path=path,
                    value=unicode(value) if tt['str'](value) else value,
                    scalar_type=t))
                return False
            return True
        except KeyError as e:
            # Type not found in valid types mapping
            log.debug(e)
            raise CoreError(u"Unknown type check: {0!s} : {1!s} : {2!s}".format(path, value, t))
