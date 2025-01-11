# Copyright (c) 2018, 2019, 2020 Nordic Semiconductor ASA
# Copyright 2018, 2019 Foundries.io Ltd
#
# SPDX-License-Identifier: Apache-2.0

'''
Parser and abstract data types for west manifests.
'''

from collections import deque
import enum
import errno
import logging
import os
from pathlib import PurePosixPath, Path
import re
import shlex
import subprocess
import sys
from typing import Any, Callable, Dict, Iterable, List, NoReturn, \
    NamedTuple, Optional, Set, TYPE_CHECKING, Union

from packaging.version import parse as parse_version
import pykwalify.core
import yaml

from west import util
from west.util import PathType
from west.configuration import Configuration, ConfigFile, MalformedConfig

#
# Public constants
#

#: Index in a Manifest.projects attribute where the `ManifestProject`
#: instance for the workspace is stored.
MANIFEST_PROJECT_INDEX = 0

#: A git revision which points to the most recent `Project` update.
MANIFEST_REV_BRANCH = 'manifest-rev'

#: A fully qualified reference to `MANIFEST_REV_BRANCH`.
QUAL_MANIFEST_REV_BRANCH = 'refs/heads/' + MANIFEST_REV_BRANCH

#: Git ref space used by west for internal purposes.
QUAL_REFS_WEST = 'refs/west/'

#: The latest manifest schema version supported by this west program.
#:
#: This value will change whenever a new version of west includes new
#: manifest file features not supported by earlier versions of west.
#: (Its value changed to 1.0 following the release of west versions
#: v1.0.x, so that users can say "I want schema version 1" instead of
#: having to keep using '0.13', which was the previous version this
#: changed.)
SCHEMA_VERSION = '1.2'
# MAINTAINERS:
#
# - Make sure to update _VALID_SCHEMA_VERS if you change this.
#
# - When changing this, make sure that it matches the major.minor
#   version of west itself. E.g. if you add a new manifest feature for
#   west 1.6, then SCHEMA_VERSION above should be 1.6, and west's
#   release version numbers should be 1.6.x.

#
# Internal helpers
#

# The value of a west-commands as passed around during manifest
# resolution. It can become a list due to resolving imports, even
# though it's just a str in each individual file right now.
WestCommandsType = Union[str, List[str]]

# Type for the importer callback passed to the manifest constructor.
# (ImportedContentType is just an alias for what it gives back.)
ImportedContentType = Optional[Union[str, List[str]]]
ImporterType = Callable[['Project', str], ImportedContentType]

# Type for an import map filter function, which takes a Project and
# returns a bool. The various allowlists and blocklists are used to
# create these filter functions. A None value is treated as a function
# which always returns True.
ImapFilterFnType = Optional[Callable[['Project'], bool]]

# A list of group names to enable and disable, like ['+foo', '-bar'].
GroupFilterType = List[str]

# A list of group names belonging to a project, like ['foo', 'bar']
GroupsType = List[str]

class PFR(enum.Enum):
    # "Project filter result": internal type for expressing whether a
    # project has been explicitly made active or inactive via
    # manifest.project-filter.

    ACTIVE = 1
    INACTIVE = 2
    NONE = 3

# Type for an individual element of a project filter.
class ProjectFilterElt(NamedTuple):
    # The regular expression to match against project names
    # when applying the filter. This must match the entire project
    # name in order for this filter to apply.
    pattern: re.Pattern

    # If True, projects whose names match 'pattern' are explicitly
    # made active. If False, projects whose names match the regular
    # expression are made inactive.
    make_active: bool

# The internal representation for a 'manifest.project-filter'
# configuration option's value.
#
# If an individual list element matches, it makes the project active
# or inactive accordingly. The "activate/inactivate result" from the
# last match in the list "wins". If there are no matches, the
# project's active/inactive status is not changed by the filter.
#
# For example, "-hal_.*,+hal_my_vendor' would make all projects
# whose names start with 'hal_' inactive, except a project named exactly
# 'hal_my_vendor'. We would represent that like this:
#
#   [ProjectFilterElt(re.compile('hal_.*'), False),
#    ProjectFilterElt(re.compile('hal_my_vendor'), True)]
#
# The regular expression must match the entire project name.
ProjectFilterType = List[ProjectFilterElt]

def _update_project_filter(project_filter: ProjectFilterType,
                           option_value: Optional[str]) -> None:
    # Validate a 'manifest.project-filter' configuration option's
    # value. The 'option_value' argument is the raw configuration
    # option. If 'option_value' is invalid, error out. Otherwise,
    # destructively modify 'project_filter' to reflect the option's
    # value.

    if option_value is None:
        return

    def _err(message):
        raise MalformedConfig(
            f'invalid "manifest.project-filter" option value '
            f'"{option_value}": {message}')

    for elt in option_value.split(','):
        elt = elt.strip()
        if not elt:
            continue
        elif not elt.startswith(('-', '+')):
            _err(f'element "{elt}" does not start with "+" or "-"')
        if len(elt) == 1:
            _err('a bare "+" or "-" contains no regular expression')

        make_active = elt.startswith('+')
        regexp = elt[1:]

        try:
            pattern = re.compile(regexp)
        except re.error as e:
            _err(f'invalid regular expression "{regexp}": {str(e)}')

        project_filter.append(ProjectFilterElt(pattern=pattern,
                                               make_active=make_active))

# The parsed contents of a manifest YAML file as returned by _load(),
# after sanitychecking with validate().
ManifestDataType = Union[str, Dict]

# Logging

_logger = logging.getLogger(__name__)

# Type for the submodule value passed through the manifest file.
class Submodule(NamedTuple):
    '''Represents a Git submodule within a project.'''

    path: str
    name: Optional[str] = None

# Submodules may be a list of values or a bool.
SubmodulesType = Union[List[Submodule], bool]

# Manifest locating, parsing, loading, etc.

class _defaults(NamedTuple):
    remote: Optional[str]
    revision: str

_DEFAULT_REV = 'master'
_WEST_YML = 'west.yml'
_SCHEMA_PATH = os.path.join(os.path.dirname(__file__), "manifest-schema.yml")
_SCHEMA_VER = parse_version(SCHEMA_VERSION)
_EARLIEST_VER_STR = '0.6.99'  # we introduced the version feature after 0.6
_VALID_SCHEMA_VERS = [
    _EARLIEST_VER_STR,
    '0.7', '0.8', '0.9', '0.10', '0.12', '0.13', '1.0',
    SCHEMA_VERSION
]

def _is_yml(path: PathType) -> bool:
    return Path(path).suffix in ['.yml', '.yaml']

def _load(data: str) -> Any:
    try:
        return yaml.safe_load(data)
    except yaml.scanner.ScannerError as e:
        raise MalformedManifest(data) from e

def _west_commands_list(west_commands: Optional[WestCommandsType]) -> \
        List[str]:
    # Convert the raw data from a manifest file to a list of
    # west_commands locations. (If it's already a list, make a
    # defensive copy.)

    if west_commands is None:
        return []
    elif isinstance(west_commands, str):
        return [west_commands]
    else:
        return list(west_commands)

def _west_commands_maybe_delist(west_commands: List[str]) -> WestCommandsType:
    # Convert a west_commands list to a string if there's
    # just one element, otherwise return the list itself.

    if len(west_commands) == 1:
        return west_commands[0]
    else:
        return west_commands

def _west_commands_merge(wc1: List[str], wc2: List[str]) -> List[str]:
    # Merge two west_commands lists, filtering out duplicates.

    if wc1 and wc2:
        return wc1 + [wc for wc in wc2 if wc not in wc1]
    else:
        return wc1 or wc2

# Manifest import handling

def _default_importer(project: 'Project', file: str) -> NoReturn:
    raise ManifestImportFailed(project, file)

def _manifest_content_at(project: 'Project', path: PathType, mf_encoding: str,
                         rev: str = QUAL_MANIFEST_REV_BRANCH) \
                                -> ImportedContentType:
    # Get a list of manifest data from project at path
    #
    # The data are loaded from Git at ref QUAL_MANIFEST_REV_BRANCH,
    # *NOT* the file system.
    #
    # If path is a tree at that ref, the contents of the YAML files
    # inside path are returned, as strings. If it's a file at that
    # ref, it's a string with its contents.
    #
    # Though this module and the "west update" implementation share
    # this code, it's an implementation detail, not API.

    path = os.fspath(path)
    _logger.debug(f'{project.name}: looking up path {path} type at {rev}')

    # Returns 'blob', 'tree', etc. for path at revision, if it exists.
    out = project.git(['ls-tree', rev, path], capture_stdout=True,
                      capture_stderr=True).stdout

    if not out:
        # It's a bit inaccurate to raise FileNotFoundError for
        # something that isn't actually file, but this is internal
        # API, and git is a content addressable file system, so close
        # enough!
        raise OSError(errno.ENOENT, os.strerror(errno.ENOENT), path)

    # TODO: does git always output utf-8? Test git config 'core.precomposeunicode' etc.
    git_filenames_encoding = 'utf-8'
    ptype = out.decode(git_filenames_encoding).split()[1]

    if ptype == 'blob':
        # Importing a file: just return its content.
        return project.read_at(path, rev=rev).decode(mf_encoding)
    elif ptype == 'tree':
        # Importing a tree: return the content of the YAML files inside it.
        ret = []
        # Use a PurePosixPath because that's the form git seems to
        # store internally, even on Windows.
        pathobj = PurePosixPath(path)
        for f in filter(_is_yml, project.listdir_at(path, rev=rev,
                                                    encoding=git_filenames_encoding)):
            ret.append(project.read_at(pathobj / f, rev=rev).decode(mf_encoding))
        return ret
    else:
        raise MalformedManifest(f"can't decipher project {project.name} "
                                f'path {path} revision {rev} '
                                f'(git type: {ptype})')

class _import_map(NamedTuple):
    file: str
    name_allowlist: List[str]
    path_allowlist: List[str]
    name_blocklist: List[str]
    path_blocklist: List[str]
    path_prefix: str

def _is_imap_list(value: Any) -> bool:
    # Return True if the value is a valid import map 'blocklist' or
    # 'allowlist'. Empty strings and lists are OK, and list nothing.

    return (isinstance(value, str) or
            (isinstance(value, list) and
             all(isinstance(item, str) for item in value)))

def _imap_filter(imap: _import_map) -> ImapFilterFnType:
    # Returns either None (if no filter is necessary) or a
    # filter function for the given import map.

    if any([imap.name_allowlist, imap.path_allowlist,
            imap.name_blocklist, imap.path_blocklist]):
        return lambda project: _is_imap_ok(imap, project)
    else:
        return None

def _ensure_list(item: Union[str, List[str]]) -> List[str]:
    # Converts item to a list containing it if item is a string, or
    # returns item.

    if isinstance(item, str):
        return [item]
    return item

def _is_imap_ok(imap: _import_map, project: 'Project') -> bool:
    # Return True if a project passes an import map's filters,
    # and False otherwise.

    nwl, pwl, nbl, pbl = [_ensure_list(lst) for lst in
                          (imap.name_allowlist, imap.path_allowlist,
                           imap.name_blocklist, imap.path_blocklist)]
    name = project.name
    path = Path(project.path)
    blocked = (name in nbl) or any(path.match(p) for p in pbl)
    allowed = (name in nwl) or any(path.match(p) for p in pwl)
    no_allowlists = not (nwl or pwl)

    if blocked:
        return allowed
    else:
        return allowed or no_allowlists

class _import_ctx(NamedTuple):
    # Holds shared state that we want to pass around as we
    # resolve imports.

    # The current map from already-defined project names to Projects.
    #
    # This is shared, mutable state between Manifest() constructor
    # calls that happen during resolution. We mutate this directly
    # when handling 'manifest: projects:' lists. Manifests which are
    # imported earlier get higher precedence: if a 'projects:' list
    # contains a name which is already present here, we ignore that
    # element.
    projects: Dict[str, 'Project']

    # The project filters we should apply while resolving imports. We
    # try to load this only once from the 'manifest.project-filter'
    # configuration option.
    project_filter: ProjectFilterType

    # Deque of group filters from every manifest we import.
    # Manifests that are imported *earlier* appear *later*
    # in the queue. That means that iterating over the queue
    # is the same thing as iterating over group filters in
    # the import tree in precedence order.
    group_filter_q: deque

    # The list of west command names provided by the manifest
    # repository itself. This is mutable state in the same way
    # 'projects' is. Manifests which are imported earlier get
    # higher precedence here as usual.
    manifest_west_commands: List[str]

    # The current restrictions on which projects the importing
    # manifest is interested in.
    #
    # These accumulate as we pick up additional allowlists and
    # blocklists in 'import: <map>' values.
    imap_filter: ImapFilterFnType

    # The current prefix which should be added to any project paths
    # as defined by all the importing manifests up to this point.
    # These accumulate as we pick up 'import: path-prefix: ...' values.
    path_prefix: Path

    # The absolute path to the manifest file we are currently loading.
    # One of the following things:
    #
    # - the top-level manifest
    # - a submanifest file, if loading via "self: import:"
    # - None, if loading from data (either explicitly or recursively
    #   via an import from a "projects:" element)
    current_abspath: Optional[Path]

    # The relative path to the manifest file we are currently loading.
    # This is relative to the topdir in the absence of symlinks, but
    # symlinks are a pain, so we stash the actual values we read from
    # configuration values like manifest.path/manifest.file
    # to make things simpler.
    current_relpath: Optional[Path]

    # The actual manifest data we are currently loading
    current_data: Any

    # The absolute path to the git repository we are currently loading
    # current_abspath from, or None.
    current_repo_abspath: Optional[Path]

    # Callable to use when importing from "manifest: projects:".
    project_importer: ImporterType

    # Bit vector of flags that modify import behavior.
    import_flags: 'ImportFlag'

def _imap_filter_allows(imap_filter: ImapFilterFnType,
                        project: 'Project') -> bool:
    # imap_filter(project) if imap_filter is not None; True otherwise.

    return (imap_filter is None) or imap_filter(project)

def _compose_imap_filters(imap_filter1: ImapFilterFnType,
                          imap_filter2: ImapFilterFnType) -> ImapFilterFnType:
    # Return an import map filter which gives back the logical AND of
    # what the two argument filter functions would return.

    if imap_filter1 and imap_filter2:
        # These type annotated versions silence mypy warnings.
        fn1: Callable[['Project'], bool] = imap_filter1
        fn2: Callable[['Project'], bool] = imap_filter2
        return lambda project: (fn1(project) and fn2(project))
    else:
        return imap_filter1 or imap_filter2

# It's an error if a group name matches this pattern.
_RESERVED_GROUP_RE = re.compile(r'(^[+-]|[\s,:])')

# _INVALID_PROJECT_NAME_RE: pattern for project names that
#                           are errors
# _RESERVED_PROJECT_NAME_RE: pattern for names that cause warnings by
#                            default for now, but will be errors later
#                            (west 2.0 or later)
_INVALID_PROJECT_NAME_RE = re.compile(r'([/\\])')
_RESERVED_PROJECT_NAME_RE = re.compile(r'[\s,]')

def _update_disabled_groups(disabled_groups: Set[str],
                            group_filter: GroupFilterType):
    # Update a set of disabled groups in place based on
    # 'group_filter'.

    for item in group_filter:
        if item.startswith('-'):
            disabled_groups.add(item[1:])
        elif item.startswith('+'):
            group = item[1:]
            if group in disabled_groups:
                disabled_groups.remove(group)
        else:
            # We should never get here. This private helper is only
            # meant to be invoked on valid data.
            assert False, \
                (f"Unexpected group filter item {item}. "
                 "This is a west bug. Please report it to the developers "
                 "along with as much information as you can, such as the "
                 "stack trace that preceded this message.")

def _is_submodule_dict_ok(subm: Any) -> bool:
    # Check whether subm is a dict that contains the expected
    # submodule fields of proper types.

    class _failed(Exception):
        pass

    def _assert(cond):
        if not cond:
            raise _failed()

    try:
        _assert(isinstance(subm, dict))
        # Required key
        _assert('path' in subm)
        # Allowed keys
        for k in subm:
            _assert(k in ['path', 'name'])
            _assert(isinstance(subm[k], str))

    except _failed:
        return False

    return True

#
# Public functions
#

def manifest_path() -> str:
    '''Absolute path of the manifest file in the current workspace.

    Exceptions raised:

        - `west.util.WestNotFound` if called from outside of a west
          workspace

        - `MalformedConfig` if the configuration file has no
          ``manifest.path`` key

        - ``FileNotFoundError`` if no manifest file exists as determined by
          ``manifest.path`` and ``manifest.file``
    '''
    topdir = Path(util.west_topdir(start=Path.cwd(), fall_back=True))
    config = Configuration(topdir=topdir)
    manifest_path = config.get('manifest.path', configfile=ConfigFile.LOCAL)
    if manifest_path is None:
        raise MalformedConfig('no local manifest.path option')
    manifest_file: str = config.get('manifest.file', _WEST_YML)  # type: ignore
    ret_path = topdir / manifest_path / manifest_file
    # It's kind of annoying to manually instantiate a FileNotFoundError.
    # This seems to be the best way.
    if not ret_path.is_file():
        raise OSError(errno.ENOENT, os.strerror(errno.ENOENT),
                      os.fspath(ret_path))
    return os.fspath(ret_path)

def validate(data: Any) -> Dict[str, Any]:
    '''Validate manifest data

    Raises an exception if the manifest data is not valid for loading
    by this version of west. (Actually attempting to load the data may
    still fail if the it contains imports which cannot be resolved.)

    Returns the validated YAML dictionary, which may be convenient if
    the argument was a str and further loading is required.

    :param data: YAML manifest data as a string or object
    '''
    if isinstance(data, str):
        as_str = data
        as_dict = _load(data)
        if not isinstance(as_dict, dict):
            raise MalformedManifest(f'{as_str} is not a YAML dictionary')
    elif isinstance(data, dict):
        as_dict = data
    else:
        raise TypeError(f'{data} has type {type(data)}, '
                        'expected raw str data or a loaded YAML dict')

    if 'manifest' not in as_dict:
        raise MalformedManifest('manifest data contains no "manifest" key')

    data = as_dict['manifest']

    # Make sure this version of west can load this manifest data.
    # This has to happen before the schema check -- later schemas
    # may incompatibly extend this one.
    if 'version' in data:
        # As a convenience for the user, convert floats to strings.
        # This avoids forcing them to write:
        #
        #  version: "0.8"
        #
        # by explicitly allowing:
        #
        #  version: 0.8
        if not isinstance(data['version'], str):
            min_version_str = str(data['version'])
            casted_to_str = True
        else:
            min_version_str = data['version']
            casted_to_str = False

        min_version = parse_version(min_version_str)
        if min_version > _SCHEMA_VER:
            raise ManifestVersionError(min_version_str)
        if min_version_str not in _VALID_SCHEMA_VERS:
            msg = (f'invalid version {min_version_str}; must be one of: ' +
                   ', '.join(_VALID_SCHEMA_VERS))
            if casted_to_str:
                msg += ('. Do you need to quote the value '
                        '(e.g. "0.10" instead of 0.10)?')
            raise MalformedManifest(msg)

    try:
        pykwalify.core.Core(source_data=data,
                            schema_files=[_SCHEMA_PATH]).validate()
    except pykwalify.errors.SchemaError as se:
        raise MalformedManifest(se.msg) from se

    return as_dict

# A 'raw' element in a project 'groups:' or manifest 'group-filter:' list,
# as it is parsed from YAML, before conversion to string.
RawGroupType = Union[str, int, float]

def is_group(raw_group: RawGroupType) -> bool:
    '''Is a 'raw' project group value 'raw_group' valid?

    Valid groups are strings that don't contain whitespace, commas
    (","), or colons (":"), and do not start with "-" or "+".

    As a special case, groups may also be nonnegative numbers, to
    avoid forcing users to quote these values in YAML files.

    :param raw_group: the group value to check
    '''
    # Implementation notes:
    #
    #     - not starting with "-" because "-foo" means "disable group
    #       foo", and not starting with "+" because "+foo" means
    #       "enable group foo".
    #
    #     - no commas because that's a separator character in
    #       manifest.group-filter and 'west update --group-filter'
    #
    #     - no whitespace mostly to guarantee that printing
    #       comma-separated lists of groups won't cause 'word' breaks
    #       in 'west list' pipelines to cut(1) or similar
    #
    #     - no colons to reserve some namespace for potential future
    #       use; we might want to do something like
    #       "--group-filter=path-prefix:foo" to create additional logical
    #       groups based on the workspace layout or other metadata

    return ((raw_group >= 0) if isinstance(raw_group, (float, int)) else
            bool(raw_group and not _RESERVED_GROUP_RE.search(raw_group)))

#
# Exception types
#

class MalformedManifest(Exception):
    '''Manifest parsing failed due to invalid data.
    '''

class ManifestImportFailed(Exception):
    '''An operation required to resolve a manifest failed.

    Attributes:

    - ``project``: the Project instance with the missing manifest data;
      None if it's from the manifest via "manifest: self: import:"
    - ``imp``: the parsed YAML data whose import was requested
    '''

    def __init__(self, project: Optional['Project'], imp: Any):
        super().__init__()
        self.project = project
        self.imp = imp

    def __str__(self):
        if self.project is None:
            # This happens when imports fail in the manifest repository
            return (f'cannot import {self.imp}; is it present '
                    'in your manifest repository?')
        else:
            return (f'project {self.project.name_and_path}: '
                    f'cannot import contents of {self.imp}; '
                    'do you need to run "west update"?')


class ManifestVersionError(Exception):
    '''The manifest required a version of west more recent than the
    current version.
    '''

    def __init__(self, version: str, file: Optional[PathType] = None):
        super().__init__(version, file)
        self.version = version
        '''The minimum version of west that was required.'''

        self.file = os.fspath(file) if file else None
        '''The file that required this version of west, if any.'''

class _ManifestImportDepth(ManifestImportFailed):
    # A hack to signal to main.py what happened.
    pass

#
# The main Manifest class and its public helper types, like Project
# and ImportFlag.
#

class ImportFlag(enum.IntFlag):
    '''Bit flags for handling imports when resolving a manifest.

    Note that any "path-prefix:" values set in an "import:" still take
    effect for the project itself even when IGNORE or IGNORE_PROJECTS are
    given. For example, in this manifest::

       manifest:
         projects:
         - name: foo
           import:
             path-prefix: bar

    Project 'foo' has path 'bar/foo' regardless of whether IGNORE or
    IGNORE_PROJECTS is given. This ensures the Project has the same path
    attribute as it normally would if imported projects weren't being
    ignored.
    '''

    #: The default value, 0, reads the file system to resolve
    #: "self: import:", and runs git to resolve a "projects:" import.
    DEFAULT = 0

    #: Ignore projects added via "import:" in "self:" and "projects:"
    IGNORE = 1

    #: Always invoke importer callback for "projects:" imports
    FORCE_PROJECTS = 2

    #: Ignore projects added via "import:" : in "projects:" only;
    #: including any projects added via "import:" : in "self:"
    IGNORE_PROJECTS = 4

def _flags_ok(flags: ImportFlag) -> bool:
    # Sanity-check the combination of flags.
    F_I = ImportFlag.IGNORE
    F_FP = ImportFlag.FORCE_PROJECTS
    F_IP = ImportFlag.IGNORE_PROJECTS

    if (flags & F_I) or (flags & F_IP):
        return not flags & F_FP
    elif flags & (F_FP | F_IP):
        return bool((flags & F_FP) ^ (flags & F_IP))
    else:
        return True

class _MLS(str):
    ''' Multi-Line String.
        This class is only used to represent multi-line strings that will be
        then dumped into YAML using block style literals ("|").
    '''
    pass

class Project:
    '''Represents a project defined in a west manifest.

    Attributes:

    - ``name``: project's unique name
    - ``description``: project's description
    - ``url``: project fetch URL
    - ``revision``: revision to fetch from ``url`` when the
      project is updated
    - ``path``: relative path to the project within the workspace
      (i.e. from ``topdir`` if that is set)
    - ``abspath``: absolute path to the project in the native path name
      format (or ``None`` if ``topdir`` is)
    - ``posixpath``: like ``abspath``, but with slashes (``/``) as
      path separators
    - ``clone_depth``: clone depth to fetch when first cloning the
      project, or ``None`` (the revision should not be a SHA
      if this is used)
    - ``west_commands``: list of YAML files where extension commands in
      the project are declared
    - ``topdir``: the top level directory of the west workspace
      the project is part of, or ``None``
    - ``remote_name``: the name of the remote which should be set up
      when the project is being cloned (default: 'origin')
    - ``groups``: the project's groups (as a list) as given in the manifest.
      If the manifest data contains no groups for the project, this is
      an empty list.
    - ``submodules``: the project's submodules configuration; either
      a list of Submodule objects, or a boolean.
    - ``userdata``: the parsed 'userdata' field in the manifest, or None
    '''

    def __eq__(self, other):
        return NotImplemented

    def __repr__(self):
        return (f'Project("{self.name}", "{self.url}", '
                f'revision="{self.revision}", path={repr(self.path)}, '
                f'clone_depth={self.clone_depth}, '
                f'west_commands={self.west_commands}, '
                f'topdir={repr(self.topdir)}, '
                f'groups={repr(self.groups)}, '
                f'userdata={repr(self.userdata)})')

    def __str__(self):
        path_repr = repr(self.abspath or self.path)
        return f'<Project {self.name} ({path_repr}) at {self.revision}>'

    def __init__(self, name: str, url: str,
                 description: Optional[str] = None,
                 revision: Optional[str] = None,
                 path: Optional[PathType] = None,
                 submodules: SubmodulesType = False,
                 clone_depth: Optional[int] = None,
                 west_commands: Optional[WestCommandsType] = None,
                 topdir: Optional[PathType] = None,
                 remote_name: Optional[str] = None,
                 groups: Optional[GroupsType] = None,
                 userdata: Optional[Any] = None):
        '''Project constructor.

        If *topdir* is ``None``, then absolute path attributes
        (``abspath`` and ``posixpath``) will also be ``None``.

        :param name: project's ``name:`` attribute in the manifest
        :param description: project's description or None
        :param url: fetch URL
        :param revision: fetch revision
        :param path: path (relative to topdir), or None for *name*
        :param submodules: submodules to pull within the project
        :param clone_depth: depth to use for initial clone
        :param west_commands: path to a west commands specification YAML
            file in the project, relative to its base directory,
            or list of these
        :param topdir: the west workspace's top level directory
        :param remote_name: the name of the remote which should be
            set up if the project is being cloned (default: 'origin')
        :param groups: a list of groups found in the manifest data for
            the project, after conversion to str and validation.
        '''

        self.name = name
        self.description = description
        self.url = url
        self.submodules = submodules
        self.revision = revision or _DEFAULT_REV
        self.clone_depth = clone_depth
        self.path = os.fspath(path or name)
        self.west_commands = _west_commands_list(west_commands)
        self.topdir = os.fspath(topdir) if topdir else None
        self.remote_name = remote_name or 'origin'
        self.groups: GroupsType = groups or []
        self.userdata: Any = userdata

    @property
    def path(self) -> str:
        return self._path

    @path.setter
    def path(self, path: PathType) -> None:
        self._path: str = os.fspath(path)

        # Invalidate the absolute path attributes. They'll get
        # computed again next time they're accessed.
        self._abspath: Optional[str] = None
        self._posixpath: Optional[str] = None

    @property
    def abspath(self) -> Optional[str]:
        if self._abspath is None and self.topdir:
            self._abspath = os.path.abspath(Path(self.topdir) /
                                            self.path)
        return self._abspath

    @property
    def posixpath(self) -> Optional[str]:
        if self._posixpath is None and self.abspath is not None:
            self._posixpath = Path(self.abspath).as_posix()
        return self._posixpath

    @property
    def name_and_path(self) -> str:
        return f'{self.name} ({self.path})'

    def as_dict(self) -> Dict:
        '''Return a representation of this object as a dict, as it
        would be parsed from an equivalent YAML manifest.
        '''
        ret: Dict = {}
        ret['name'] = self.name
        if self.description:
            ret['description'] = _MLS(self.description)
        ret['url'] = self.url
        ret['revision'] = self.revision
        if self.path != self.name:
            ret['path'] = self.path
        if self.clone_depth:
            ret['clone-depth'] = self.clone_depth
        if self.west_commands:
            ret['west-commands'] = \
                _west_commands_maybe_delist(self.west_commands)
        if self.groups:
            ret['groups'] = self.groups
        if isinstance(self.submodules, bool) and self.submodules:
            ret['submodules'] = True
        elif isinstance(self.submodules, list):
            ret['submodules'] = []
            for s in self.submodules:
                obj: Dict = {'path': s.path}
                if s.name:
                    obj['name'] = s.name
                ret['submodules'].append(obj)
        if self.userdata:
            ret['userdata'] = self.userdata

        return ret

    #
    # Git helpers
    #

    def git(self, cmd: Union[str, List[str]],
            extra_args: Iterable[str] = (),
            capture_stdout: bool = False,
            capture_stderr: bool = False,
            check: bool = True,
            cwd: Optional[PathType] = None) -> subprocess.CompletedProcess:
        '''Run a git command in the project repository.

        :param cmd: git command as a string (or list of strings)
        :param extra_args: sequence of additional arguments to pass to
            the git command (useful mostly if *cmd* is a string).
        :param capture_stdout: if True, git's standard output is
            captured in the ``CompletedProcess`` instead of being
            printed.
        :param capture_stderr: Like *capture_stdout*, but for standard
            error. Use with caution: this may prevent error messages
            from being shown to the user.
        :param check: if given, ``subprocess.CalledProcessError`` is
            raised if git finishes with a non-zero return code
        :param cwd: directory to run git in (default: ``self.abspath``)
        '''
        if isinstance(cmd, str):
            cmd_list = shlex.split(cmd)
        else:
            cmd_list = list(cmd)

        extra_args = list(extra_args)

        if cwd is None:
            if self.abspath is not None:
                cwd = self.abspath
            else:
                raise ValueError('no abspath; cwd must be given')
        elif sys.version_info < (3, 6, 1) and not isinstance(cwd, str):
            # Popen didn't accept a PathLike cwd on Windows until
            # python v3.7; this was backported onto cpython v3.6.1,
            # though. West currently supports "python 3.6", though, so
            # in the unlikely event someone is running 3.6.0 on
            # Windows, do the right thing.
            cwd = os.fspath(cwd)

        args = ['git'] + cmd_list + extra_args
        cmd_str = util.quote_sh_list(args)

        _logger.debug(f"running '{cmd_str}' in {cwd}")
        popen = subprocess.Popen(
            args, cwd=cwd,
            stdout=subprocess.PIPE if capture_stdout else None,
            stderr=subprocess.PIPE if capture_stderr else None)

        stdout, stderr = popen.communicate()

        # We use logger style % formatting here to avoid the
        # potentially expensive overhead of formatting long
        # stdout/stderr strings if the current log level isn't DEBUG,
        # which is the usual case.
        _logger.debug('"%s" exit code: %d stdout: %r stderr: %r',
                      cmd_str, popen.returncode, stdout, stderr)

        if check and popen.returncode:
            raise subprocess.CalledProcessError(popen.returncode, cmd_list,
                                                output=stdout, stderr=stderr)
        else:
            return subprocess.CompletedProcess(popen.args, popen.returncode,
                                               stdout, stderr)

    def sha(self, rev: str, cwd: Optional[PathType] = None) -> str:
        '''Get the SHA for a project revision.

        :param rev: git revision (HEAD, v2.0.0, etc.) as a string
        :param cwd: directory to run command in (default:
            self.abspath)
        '''
        # Though we capture stderr, it will be available as the stderr
        # attribute in the CalledProcessError raised by git() in
        # Python 3.5 and above if this call fails.
        cp = self.git(['rev-parse', f'{rev}^{{commit}}'], capture_stdout=True,
                      cwd=cwd, capture_stderr=True)
        # Assumption: SHAs are hex values and thus safe to decode in ASCII.
        # It'll be fun when we find out that was wrong and how...
        return cp.stdout.decode('ascii').strip()

    def is_ancestor_of(self, rev1: str, rev2: str,
                       cwd: Optional[PathType] = None) -> bool:
        '''Check if 'rev1' is an ancestor of 'rev2' in this project.

        Returns True if rev1 is an ancestor commit of rev2 in the
        given project; rev1 and rev2 can be anything that resolves to
        a commit. (If rev1 and rev2 refer to the same commit, the
        return value is True, i.e. a commit is considered an ancestor
        of itself.) Returns False otherwise.

        :param rev1: commit that could be the ancestor of *rev2*
        :param rev2: commit that could be a descendant or *rev1*
        :param cwd: directory to run command in (default:
            ``self.abspath``)
        '''
        rc = self.git(f'merge-base --is-ancestor {rev1} {rev2}',
                      check=False, cwd=cwd).returncode

        if rc == 0:
            return True
        elif rc == 1:
            return False
        else:
            raise RuntimeError(f'unexpected git merge-base result {rc}')

    def is_up_to_date_with(self, rev: str,
                           cwd: Optional[PathType] = None) -> bool:
        '''Check if the project is up to date with *rev*, returning
        ``True`` if so.

        This is equivalent to ``is_ancestor_of(rev, 'HEAD',
        cwd=cwd)``.

        :param rev: base revision to check if project is up to date
            with.
        :param cwd: directory to run command in (default:
            ``self.abspath``)
        '''
        return self.is_ancestor_of(rev, 'HEAD', cwd=cwd)

    def is_up_to_date(self, cwd: Optional[PathType] = None) -> bool:
        '''Check if the project HEAD is up to date with the manifest.

        This is equivalent to ``is_up_to_date_with(self.revision,
        cwd=cwd)``.

        :param cwd: directory to run command in (default:
            ``self.abspath``)
        '''
        return self.is_up_to_date_with(self.revision, cwd=cwd)

    def is_cloned(self, cwd: Optional[PathType] = None) -> bool:
        '''Returns ``True`` if ``self.abspath`` looks like a git
        repository's top-level directory, and ``False`` otherwise.

        :param cwd: directory to run command in (default:
            ``self.abspath``)
        '''
        if not self.abspath or not os.path.isdir(self.abspath):
            return False

        # --is-inside-work-tree doesn't require that the directory is
        # the top-level directory of a Git repository. Use --show-cdup
        # instead, which prints an empty string (i.e., just a newline,
        # which we strip) for the top-level directory.
        _logger.debug(f'{self.name}: checking if cloned')
        res = self.git(['rev-parse', '--show-cdup'], check=False, cwd=cwd,
                       capture_stderr=True, capture_stdout=True)

        return not (res.returncode or res.stdout.strip())

    def read_at(self, path: PathType, rev: Optional[str] = None,
                cwd: Optional[PathType] = None) -> bytes:
        '''Read file contents in the project at a specific revision.

        :param path: relative path to file in this project
        :param rev: revision to read *path* from (default: ``self.revision``)
        :param cwd:  directory to run command in (default: ``self.abspath``)
        '''
        if rev is None:
            rev = self.revision
        cp = self.git(['show', f'{rev}:{os.fspath(path)}'],
                      capture_stdout=True, capture_stderr=True, cwd=cwd)
        return cp.stdout

    def listdir_at(self, path: PathType, rev: Optional[str] = None,
                   cwd: Optional[PathType] = None,
                   encoding: Optional[str] = None) -> List[str]:
        '''List of directory contents in the project at a specific revision.

        The return value is the directory contents as a list of files and
        subdirectories.

        :param path: relative path to file in this project
        :param rev: revision to read *path* from (default: ``self.revision``)
        :param cwd: directory to run command in (default: ``self.abspath``)
        :param encoding: directory contents encoding (default: 'utf-8')
        '''
        if rev is None:
            rev = self.revision
        if encoding is None:
            encoding = 'utf-8'

        # git-ls-tree -z means we get NUL-separated output with no quoting
        # of the file names. Using 'git-show' or 'git-cat-file -p'
        # wouldn't work for files with special characters in their names.
        out = self.git(['ls-tree', '-z', f'{rev}:{os.fspath(path)}'], cwd=cwd,
                       capture_stdout=True, capture_stderr=True).stdout

        # A tab character separates the SHA from the file name in each
        # NUL-separated entry.
        return [f.decode(encoding).split('\t', 1)[1]
                for f in out.split(b'\x00') if f]

# FIXME: this whole class should just go away. See #327.
class ManifestProject(Project):
    '''Represents the manifest repository as a `Project`.

    Meaningful attributes:

    - ``name``: the string ``"manifest"``
    - ``topdir``: the top level directory of the west workspace
      the manifest project controls, or ``None``
    - ``path``: relative path to the manifest repository within the
      workspace, or ``None`` (i.e. from ``topdir`` if that is set)
    - ``abspath``: absolute path to the manifest repository in the
      native path name format (or ``None`` if ``topdir`` is)
    - ``posixpath``: like ``abspath``, but with slashes (``/``) as
      path separators
    - ``west_commands``:``west_commands:`` key in the manifest's
      ``self:`` map. This may be a list of such if the self
      section imports multiple additional files with west commands.
    - ``userdata``: the parsed 'userdata' field under self in the manifest file

    Other readable attributes included for Project compatibility:

    - ``url``: the empty string; the west manifest is not
      version-controlled by west itself, even though 'west init'
      can fetch a manifest repository from a Git remote
    - ``revision``: ``"HEAD"``
    - ``clone_depth``: ``None``, because there's no URL
    - ``groups``: the empty list
    '''

    def __repr__(self):
        return (f'ManifestProject(path={repr(self.path)}, '
                f'west_commands={self.west_commands}, '
                f'topdir={repr(self.topdir)}, '
                f'userdata={repr(self.userdata)})')

    def __init__(self, path: Optional[PathType] = None,
                 west_commands: Optional[WestCommandsType] = None,
                 topdir: Optional[PathType] = None,
                 userdata: Optional[Any] = None):
        '''
        :param path: Relative path to the manifest repository in the
            west workspace, if known.
        :param west_commands: path to a west commands specification YAML
            file in the project, relative to its base directory,
            or list of these
        :param topdir: Root of the west workspace the manifest
            project is inside. If not given, all absolute path
            attributes (abspath and posixpath) will be None.
        :param userdata: User data specified under self in manifest file
        '''
        self.name: str = 'manifest'

        # Pretending that this is a Project, even though it's not (#327)
        self.description: Optional[str] = None
        self.url: str = ''
        self.submodules: SubmodulesType = False
        self.revision: str = 'HEAD'
        self.remote_name: str = ''
        self.clone_depth: Optional[int] = None
        self.groups = []
        self.userdata: Optional[Any] = userdata

        # The following type: ignore is necessary since every Project
        # actually has a non-None _path attribute, so the parent class
        # defines its type as 'str', where here we need it to be
        # an Optional[str].
        self._path = os.fspath(path) if path else None  # type: ignore

        # Path related attributes
        self.topdir: Optional[str] = os.fspath(topdir) if topdir else None
        self._abspath: Optional[str] = None
        self._posixpath: Optional[str] = None

        # Extension commands.
        self.west_commands = _west_commands_list(west_commands)

    @property
    def abspath(self) -> Optional[str]:
        if self._abspath is None and self.topdir and self.path:
            self._abspath = os.path.abspath(os.path.join(self.topdir,
                                                         self.path))
        return self._abspath

    def as_dict(self) -> Dict:
        '''Return a representation of this object as a dict, as it would be
        parsed from an equivalent YAML manifest.'''
        ret: Dict = {}
        if self.path:
            ret['path'] = self.path
        if self.west_commands:
            ret['west-commands'] = \
                _west_commands_maybe_delist(self.west_commands)
        if self.userdata:
            ret['userdata'] = self.userdata
        return ret

class Manifest:
    '''The parsed contents of a west manifest file.
    '''

    # TODO: make this a new west config: 'manifest.encoding'
    encoding: str = 'utf-8'

    @staticmethod
    def from_topdir(topdir: Optional[PathType] = None,
                    config: Optional[Configuration] = None,
                    importer: Optional[ImporterType] = None,
                    import_flags: ImportFlag = ImportFlag.DEFAULT
                    ) -> 'Manifest':
        '''Manifest object factory given a workspace topdir.

        The default behavior if *topdir* is not given is to find the
        current west workspace's manifest file starting from the
        current working directory.

        :param topdir: workspace top-level directory
        :param config: passed to Manifest()
        :param importer: passed to Manifest()
        :param import_flags: passed to Manifest()
        '''
        if topdir is None:
            topdir = Path(util.west_topdir(start=Path.cwd(),
                                           fall_back=False)).resolve()
        return Manifest(topdir=topdir, config=config,
                        importer=importer, import_flags=import_flags)

    @staticmethod
    def from_file(source_file: Optional[PathType] = None,
                  importer: Optional[ImporterType] = None,
                  import_flags: ImportFlag = ImportFlag.DEFAULT
                  ) -> 'Manifest':
        '''Manifest object factory given a source YAML file.

        The default behavior if *source_file* is not given is to find
        the current west workspace's manifest file and resolve it
        starting from the current working directory. This matches the
        from_topdir() behavior.

        With *source_file*, the *topdir* is found starting there. As a
        special case, this factory allows you to load a Manifest
        from an arbitrary file in an arbitrary git repository in the
        workspace. The ``manifest.path`` and ``manifest.file`` configuration
        values do not have to refer to *source_file* in this case.

        This can be useful to load an alternative manifest file within
        an existing workspace for purposes of comparing two manifests,
        for example.

        Exceptions raised:

            - `west.util.WestNotFound` if no *topdir* can be found
              starting from *source_file* or the current working directory

            - CalledProcessError if the git repository containing
              *source_file* cannot be found

            - Other exceptions from the Manifest constructor

        :param source_file: source file to load
        :param importer: passed to Manifest()
        :param import_flags: passed to Manifest()
        '''
        if source_file is None:
            start = Path.cwd()
            fall_back = True
        else:
            source_file = Path(source_file).resolve()
            start = source_file.parent
            fall_back = False

        # Find the workspace topdir.
        topdir = Path(util.west_topdir(start=start,
                                       fall_back=fall_back)).resolve()

        # Load a Configuration.
        if source_file is None:
            config = Configuration(topdir=topdir)
        else:
            # Find the git repository we will treat as the
            # manifest.path, chopping off the newline, performing a
            # hopefully-safe decode, and resolving symlinks.
            manifest_abspath = Path(subprocess.check_output(
                ['git', 'rev-parse', '--show-toplevel'],
                cwd=start)[:-1].decode('utf-8')).resolve()
            manifest_path = manifest_abspath.relative_to(topdir)
            patch_dict = {
                'manifest.path': manifest_path,
                'manifest.file': source_file.relative_to(manifest_abspath)
            }
            # Patch the Configuration object to behave as though
            # source_file was the manifest file, as a special case.
            config = _PatchedConfiguration(patch_dict, topdir=topdir)

        return Manifest(topdir=topdir, config=config,
                        importer=importer, import_flags=import_flags)

    @staticmethod
    def from_data(source_data: ManifestDataType,
                  importer: Optional[ImporterType] = None,
                  import_flags: ImportFlag = ImportFlag.DEFAULT
                  ) -> 'Manifest':
        '''Manifest object factory given parsed YAML data.

        This factory does not read any configuration files.

        Let the return value be ``m``. Relative project paths in ``m``
        (like ``m.projects[1].path``) are taken from *source_data*.
        Absolute project paths are all None.

        May raise the same exceptions as the Manifest constructor.

        :param source_data: parsed YAML data as a Python object, or a
            string with unparsed YAML data
        :param importer: passed to Manifest
        :param import_flags: passed to Manifest
        '''
        if not source_data:
            raise MalformedManifest('manifest contains no data')
        return Manifest(source_data=source_data, importer=importer,
                        import_flags=import_flags)

    def __init__(self, *,  # All arguments are keyword-only.
                 source_data: Optional[ManifestDataType] = None,
                 topdir: Optional[PathType] = None,
                 config: Optional[Configuration] = None,
                 importer: Optional[ImporterType] = None,
                 import_flags: ImportFlag = ImportFlag.DEFAULT,
                 internal_import_ctx: Optional[_import_ctx] = None):
        '''Using one of the factory methods may be easier than direct
        instantiation.

        Instance attributes:

            - ``abspath``: absolute path to the manifest file, or None

            - ``posixpath``: like ``abspath``, but with slashes (``/``)
              as separators

            - ``relative_path``: path to the manifest file relative to
              the workspace topdir (i.e. the combined manifest.path
              and manifest.file configuration options), or None

            - ``yaml_path``: the value of the "self: path:" field in
              the manifest YAML, or None

            - ``repo_path``: relative filesystem path to the manifest
              repository from the workspace topdir as a string, or None

            - ``repo_abspath``: the absolute filesystem path to the manifest
              repository as a string with symlinks resolved, or None

            - ``repo_posixpath``: like ``repo_abspath``, but with slashes
              as separators

            - ``topdir``: the workspace top level directory as a string
               with symlinks resolved, or None

            - ``projects``: sequence of `Project` instances

            - ``has_imports``: bool, True if the manifest contains
              an "import:" attribute in "self:" or "projects:" that were
              not ignored due to *import_flags* or manifest.project-filter;
              False otherwise

            - ``group_filter``: a group filter value equivalent to
              the resolved manifest's "group-filter:", along with any
              values from imported manifests. This value may be simpler
              than the actual input data.

        You must give exactly one of the *topdir* and *source_data* kwargs:

            - Use *topdir* to load a manifest from a workspace

            - Use *source_data* to load data without any workspace

        If *topdir* is given:

            - You may pass *config* if you already have the desired
              workspace configuration loaded. This is used to
              find the manifest file's location from the manifest.path
              and manifest.file options. It's your responsibility
              to make sure *config* is properly loaded from *topdir*
              in this case.

              If you don't pass *config*, the constructor will
              instantiate the correct Configuration object itself.

            - The return value's absolute paths will be relative to
              *topdir*. If *topdir* is not an absolute path, it will be
              resolved first (this resolves symlinks too).
              If it is absolute, it will not be resolved.

        If *source_data* is given:

            - You cannot pass *config*.

            - Manifest imports will fail unless you pass *importer*
              or ignore them with *import_flags*.

            - All absolute paths (like ``projects[i].abspath``)
              in the results will be ``None``.

        The *importer* kwarg, if given, is a callable. It is used
        as a callback by the constructor when it must import manifest
        data that aren't found locally on the file system.

        The *importer* callback will be called as:

            ``importer(project, file)``

        where ``project`` is a `west.manifest.Project` and ``file``
        is the missing manifest file. The file's contents at
        refs/heads/manifest-rev should usually be returned by the callback,
        potentially after fetching the project's revision from its remote URL
        and updating that ref.

        The *importer* callback's return value should be a string containing
        manifest data, or a list of strings if ``file`` is a directory
        containing YAML files. A return value of None will cause the import
        to be ignored.

        Exceptions raised:

            - `MalformedManifest`: if the manifest data is invalid

            - `ManifestImportFailed`: if an import failed

            - `ManifestVersionError`: if this version of west is too
              old to parse the manifest (based on its schema version)

            - ``OSError``: or subclasses, when files cannot be opened

            - ``ValueError``: for other invalid arguments

        :param source_data: parsed YAML data as a Python object, or a
            string containing unparsed YAML data
        :param topdir: west workspace top level directory
        :param config: optional pre-loaded configuration from topdir
        :param importer: provides missing manifest import data
        :param import_flags: bit mask, controls import resolution
        :param internal_import_ctx: for internal use only; do not use
        '''

        # Initialize public state; these are overwritten later as
        # needed.

        self.abspath: Optional[str] = None
        self.relative_path: Optional[str] = None
        self.yaml_path: Optional[str] = None
        self.repo_path: Optional[str] = None
        self.repo_abspath: Optional[str] = None
        self.topdir: Optional[str] = None
        if topdir:
            topdir = Path(topdir)
            if topdir.is_absolute():
                topdir_abspath: Optional[Path] = topdir
            else:
                topdir_abspath = Path(topdir).resolve()
            if TYPE_CHECKING:
                assert topdir_abspath is not None
            self.topdir = os.fspath(topdir_abspath)
        else:
            topdir_abspath = None
        self.has_imports: bool = False
        # The final, effective group filter, with simplifications
        # applied so that e.g. ['-foo', '+foo', '-bar'] becomes
        # ['-bar'], with filters imported from other manifests applied.
        self.group_filter: GroupFilterType = []

        # Initialize private state, some of which is also overwritten
        # later as needed.

        # This backs the projects() property.
        self._projects: List[Project] = []
        # The final set of groups which are explicitly disabled in
        # this manifest data, after resolving imports. This is used
        # as an optimization in is_active().
        self._disabled_groups: Set[str] = set()
        # The "raw" (unparsed) manifest.group-filter configuration
        # option in the local configuration file. See
        # _config_group_filter(); only initialized if self._top_level
        # is True and if we're loading from a file in a workspace.
        self._raw_config_group_filter: Optional[str] = None
        # A helper attribute we use for schema version v0.9 compatibility.
        self._top_level_group_filter: GroupFilterType = []
        # The manifest.path configuration option in the local
        # configuration file, as a Path.
        self._config_path: Optional[Path] = None
        # These back the relevant properties.
        self._posixpath: Optional[str] = None
        self._repo_posixpath: Optional[str] = None
        # This backs the userdata property
        self.userdata: Any = None
        # Load context needed for import resolution. Do top-level
        # argument validation and storage if self._top_level is True,
        # but otherwise just get self._ctx from the caller.
        if internal_import_ctx is None:
            self._top_level: bool = True
            self._ctx = self._top_level_init(source_data,
                                             topdir,
                                             topdir_abspath,
                                             config,
                                             importer or _default_importer,
                                             import_flags)
        else:
            self._top_level = False
            self._recursive_init(internal_import_ctx)
            self._ctx = internal_import_ctx

        # Validate the current data against the schema. Wrap a couple
        # of the exceptions with extra context about the problematic
        # file in case of errors, to help debugging.

        try:
            self._ctx = self._ctx._replace(
                current_data=validate(self._ctx.current_data))
        except ManifestVersionError as mv:
            raise ManifestVersionError(mv.version,
                                       file=self._ctx.current_abspath) from mv
        except MalformedManifest as mm:
            self._malformed(mm.args[0], parent=mm)
        except TypeError as te:
            self._malformed(te.args[0], parent=te)

        # Finish loading the validated data.

        self._load_validated()

    def get_projects(self,
                     # any str name is also a PathType
                     project_ids: Iterable[PathType],
                     allow_paths: bool = True,
                     only_cloned: bool = False) -> List[Project]:
        '''Get a list of `Project` objects in the manifest from
        *project_ids*.

        If *project_ids* is empty, a copy of ``self.projects``
        attribute is returned as a list. Otherwise, the returned list
        has projects in the same order as *project_ids*.

        ``ValueError`` is raised if:

            - *project_ids* contains unknown project IDs

            - (with *only_cloned*) an uncloned project was found

        The ``ValueError`` *args* attribute is a 2-tuple with a list
        of unknown *project_ids* at index 0, and a list of uncloned
        `Project` objects at index 1.

        :param project_ids: a sequence of projects, identified by name
            or (absolute or relative) path. Names are matched first; path
            checking can be disabled with *allow_paths*.
        :param allow_paths: if false, *project_ids* is assumed to contain
            names only, not paths
        :param only_cloned: raise an exception for uncloned projects
        '''
        projects = list(self.projects)
        unknown: List[PathType] = []  # project_ids with no Projects
        uncloned: List[Project] = []  # if only_cloned, the uncloned Projects
        ret: List[Project] = []  # result list of resolved Projects

        # If no project_ids are specified, use all projects.
        if not project_ids:
            if only_cloned:
                uncloned = [p for p in projects if not p.is_cloned()]
                if uncloned:
                    raise ValueError(unknown, uncloned)
            return projects

        # Otherwise, resolve each of the project_ids to a project,
        # returning the result or raising ValueError.
        for pid in project_ids:
            project: Optional[Project] = None

            if isinstance(pid, str):
                project = self._projects_by_name.get(pid)

            if project is None and allow_paths:
                project = self._projects_by_rpath.get(Path(pid).resolve())

            if project is None:
                unknown.append(pid)
                continue

            ret.append(project)

            if only_cloned and not project.is_cloned():
                uncloned.append(project)

        if unknown or (only_cloned and uncloned):
            raise ValueError(unknown, uncloned)
        return ret

    def _as_dict_helper(
            self, pdict: Optional[Callable[[Project], Dict]] = None) \
            -> Dict:
        # pdict: returns a Project's dict representation.
        #        By default, it's Project.as_dict.
        if pdict is None:
            pdict = Project.as_dict

        projects = list(self.projects)
        del projects[MANIFEST_PROJECT_INDEX]
        project_dicts = [pdict(p) for p in projects]

        # This relies on insertion-ordered dictionaries for
        # predictability, which is a CPython 3.6 implementation detail
        # and Python 3.7+ guarantee.
        r: Dict[str, Any] = {}
        r['manifest'] = {}
        if self.group_filter:
            r['manifest']['group-filter'] = self.group_filter
        r['manifest']['projects'] = project_dicts
        r['manifest']['self'] = self.projects[MANIFEST_PROJECT_INDEX].as_dict()

        return r

    def as_dict(self) -> Dict:
        '''Returns a dict representing self, fully resolved.

        The value is "resolved" in that the result is as if all
        projects had been defined in a single manifest without any
        import attributes.
        '''
        return self._as_dict_helper()

    def as_frozen_dict(self) -> Dict:
        '''Returns a dict representing self, but frozen.

        The value is "frozen" in that all project revisions are the
        full SHAs pointed to by `QUAL_MANIFEST_REV_BRANCH` references.

        Raises ``RuntimeError`` if a project SHA can't be resolved.
        '''
        def pdict(p):
            if not p.is_cloned():
                raise RuntimeError(f'cannot freeze; project {p.name} '
                                   'is uncloned')
            try:
                sha = p.sha(QUAL_MANIFEST_REV_BRANCH)
            except subprocess.CalledProcessError as e:
                raise RuntimeError(f'cannot freeze; project {p.name} '
                                   f'ref {QUAL_MANIFEST_REV_BRANCH} '
                                   'cannot be resolved to a SHA') from e
            d = p.as_dict()
            d['revision'] = sha
            return d

        return self._as_dict_helper(pdict=pdict)

    def _dump_yaml(self, to_dump: Dict, **kwargs) -> str:
        ''' Dumps dictionary to YAML using the multi-line string representer.

        :param dict: dictionary to be dumped
        :param kwargs: passed to yaml.safe_dump()
        '''

        def mls_representer(dumper, data):
            if '\n' in data:
                tag = u'tag:yaml.org,2002:str'
                return dumper.represent_scalar(tag, data, style="|")
            else:
                return dumper.represent_str(data)

        yaml.add_representer(_MLS, mls_representer, Dumper=yaml.SafeDumper)
        return yaml.safe_dump(to_dump, **kwargs)

    def as_yaml(self, **kwargs) -> str:
        '''Returns a YAML representation for self, fully resolved.

        The value is "resolved" in that the result is as if all
        projects had been defined in a single manifest without any
        import attributes.

        :param kwargs: passed to yaml.safe_dump()
        '''
        return self._dump_yaml(self.as_dict(), **kwargs)

    def as_frozen_yaml(self, **kwargs) -> str:
        '''Returns a YAML representation for self, but frozen.

        The value is "frozen" in that all project revisions are the
        full SHAs pointed to by `QUAL_MANIFEST_REV_BRANCH` references.

        Raises ``RuntimeError`` if a project SHA can't be resolved.

        :param kwargs: passed to yaml.safe_dump()
        '''
        return self._dump_yaml(self.as_frozen_dict(), **kwargs)

    @property
    def projects(self) -> List[Project]:
        '''Sequence of `Project` objects representing manifest
        projects.

        Index 0 (`MANIFEST_PROJECT_INDEX`) contains a
        `ManifestProject` representing the manifest repository. The
        rest of the sequence contains projects in manifest file order
        (or resolution order if the manifest contains imports).
        '''
        return self._projects

    def is_active(self, project: Project,
                  extra_filter: Optional[Iterable[str]] = None) -> bool:
        '''Is a project active?

        If the manifest.project-filter configuration option is set,
        the return value determined by the option's value:

        - The elements of the manifest.project-filter value are checked
          against the project's name. If the regular expression in the
          element matches the project's name, then the project is active
          or inactive depending on if the element begins with + or -
          respectively.

        - If multiple elements have regular expressions matching the
          project's name, the last element which has a match determines
          the result.

        - This function returns True or False if the project is active or
          inactive according to these rules.

        The manifest.project-filter value that was set at the time
        this Manifest object was constructed is used.

        Otherwise, the return value depends on whether the project has
        any groups, and if so, whether they are enabled:

        - Projects with empty 'project.groups' lists are always active,
          and this function returns True for such projects.

        - If 'project.groups' is not empty, and any group in it is
          enabled by this manifest's 'group-filter:' list (and the
          'manifest.group-filter' local configuration option, if we
          have a workspace), this returns True.

        - Otherwise, i.e. if all of the project's groups are disabled,
          this returns False.

        "Inactive" projects should generally be considered absent from
        the workspace for purposes like updating, listing, resolving
        imports, etc.

        :param project: project to check
        :param extra_filter: an optional additional group filter
        '''

        if not isinstance(project, ManifestProject):
            pfr = self._pfr(project)
            if pfr == PFR.ACTIVE:
                return True
            elif pfr == PFR.INACTIVE:
                return False

        if not project.groups:
            # Projects without any groups are always active, so just
            # exit early. Note that this happens to treat the
            # ManifestProject as though it's always active. This is
            # important for keeping it in the 'west list' output for
            # now.
            return True

        # Parse manifest.group-filter from the configuration file if we
        # haven't already.
        cfg_gf = self._config_group_filter

        # Figure out what the disabled groups are. Skip reallocation
        # if possible.
        if cfg_gf or extra_filter is not None:
            disabled_groups = set(self._disabled_groups)
            if cfg_gf:
                _update_disabled_groups(disabled_groups, cfg_gf)
            if extra_filter is not None:
                extra_filter = self._validated_group_filter(None,
                                                            list(extra_filter))
                _update_disabled_groups(disabled_groups, extra_filter)
        else:
            disabled_groups = self._disabled_groups

        return any(group not in disabled_groups for group in project.groups)

    def _pfr(self, project: Project) -> PFR:
        # Internal helper for checking if a project has been
        # made explicitly active or inactive.

        name = project.name
        ret = PFR.NONE
        for elt in self._ctx.project_filter:
            if not elt.pattern.fullmatch(name):
                continue
            if elt.make_active:
                ret = PFR.ACTIVE
            else:
                ret = PFR.INACTIVE
        return ret

    @property
    def path(self) -> Optional[str]:  # for compatibility
        '''Deprecated. Use abspath instead.'''
        return self.abspath

    @property
    def posixpath(self):
        if self._posixpath is None and self.abspath is not None:
            self._posixpath = Path(self.abspath).as_posix()
        return self._posixpath

    @property
    def repo_posixpath(self):
        if self._repo_posixpath is None and self.repo_abspath is not None:
            self._repo_posixpath = Path(self.repo_abspath).as_posix()
        return self._repo_posixpath

    @property
    def _config_group_filter(self) -> GroupFilterType:
        # Private property for loading the manifest.group-filter value
        # in the local configuration file. Used by is_active.

        if not hasattr(self, '_cfg_gf'):
            self._cfg_gf = self._load_config_group_filter()
        return self._cfg_gf

    def _load_config_group_filter(self) -> GroupFilterType:
        # Parse the raw manifest.group-filter (converted to a list
        # of strings) from the local configuration file if there is
        # one.
        #
        # Returns [] if manifest.group-filter is not set, is empty,
        # and when there was no workspace at __init__() time.

        if not self._raw_config_group_filter:
            return []

        # Be forgiving: allow empty strings and values with
        # whitespace, and ignore (but emit warnings for) invalid
        # values.
        #
        # Whitespace in between groups, like "foo ,bar", is removed,
        # resulting in valid group names ['foo', 'bar'].
        ret: GroupFilterType = []
        for item in self._raw_config_group_filter.split(','):
            stripped = item.strip()
            if not stripped:
                # Don't emit a warning here.
                continue
            if not stripped[0].startswith(('-', '+')):
                _logger.warning(
                    f'ignoring invalid manifest.group-filter item {item}; '
                    'this must start with "-" or "+"')
                continue
            if not is_group(stripped[1:]):
                _logger.warning(
                    f'ignoring invalid manifest.group-filter item {item}; '
                    f'"{stripped[1:]}" is not a group name')
                continue
            ret.append(stripped)

        return ret

    def _malformed(self, complaint: str,
                   parent: Optional[Exception] = None) -> NoReturn:
        context = (f'file: {self._ctx.current_abspath} '
                   if self._ctx.current_abspath else 'data')
        args = [f'Malformed manifest {context}',
                f'Schema file: {_SCHEMA_PATH}']
        if complaint:
            args.append('Hint: ' + complaint)
        exc = MalformedManifest(*args)
        if parent:
            raise exc from parent
        else:
            raise exc

    def _top_level_init(self, source_data, topdir, topdir_abspath,
                        config, project_importer,
                        import_flags) -> _import_ctx:
        # Validate the top-level arguments, perform some
        # top-level-only early initialization, and set up the initial
        # _import_ctx, returning it.

        if not (topdir or source_data):
            raise ValueError('neither topdir nor source_data were given')
        if topdir and source_data:
            raise ValueError('both topdir and source_data were given')
        if source_data and config:
            raise ValueError('both source_data and config were given')
        if not _flags_ok(import_flags):
            raise ValueError(f'bad import_flags {import_flags:x}')

        current_abspath = None
        current_relpath = None
        current_data = source_data
        current_repo_abspath = None
        project_filter: ProjectFilterType = []

        if topdir_abspath:
            config = config or Configuration(topdir=topdir_abspath)

            def get_option(option, default=None):
                # Gets a ConfigFile.LOCAL option from 'config'.
                return config.get(option, default=default,
                                  configfile=ConfigFile.LOCAL)

            manifest_path_option = get_option('manifest.path')

            if manifest_path_option is None:
                raise MalformedConfig(
                    'no local "manifest.path" config option is set')

            manifest_path = Path(manifest_path_option)

            if manifest_path.is_absolute():
                _logger.warning('"manifest.path" should not be absolute: %s',
                                manifest_path_option)

            manifest_file = get_option('manifest.file', _WEST_YML)

            current_relpath = manifest_path / manifest_file
            current_abspath = topdir_abspath / current_relpath
            try:
                current_data = current_abspath.read_text(encoding=Manifest.encoding)
            except FileNotFoundError:
                raise MalformedConfig(
                    f'file not found: manifest file {current_abspath} '
                    '(from configuration options '
                    f'manifest.path="{manifest_path_option}", '
                    f'manifest.file="{manifest_file}")')

            current_repo_abspath = topdir_abspath / manifest_path

            self.abspath = os.fspath(current_abspath)
            self.relative_path = os.fspath(current_relpath)
            self.repo_path = os.fspath(manifest_path)
            self.repo_abspath = os.fspath(current_repo_abspath)
            self._raw_config_group_filter = get_option('manifest.group-filter')
            self._config_path = manifest_path
            _update_project_filter(project_filter,
                                   config.get('manifest.project-filter'))

        return _import_ctx(projects={},
                           project_filter=project_filter,
                           group_filter_q=deque(),
                           manifest_west_commands=[],
                           imap_filter=None,
                           path_prefix=Path('.'),
                           current_abspath=current_abspath,
                           current_relpath=current_relpath,
                           current_data=current_data,
                           current_repo_abspath=current_repo_abspath,
                           project_importer=project_importer,
                           import_flags=import_flags)

    def _recursive_init(self, ctx: _import_ctx):
        # Set up any required state that we need while recursively
        # resolving a manifest due to an import.

        if ctx.current_repo_abspath:
            self.repo_abspath = os.fspath(ctx.current_repo_abspath)

    def _load_validated(self) -> None:
        # Finish initializing this instance by loading data from YAML
        # that's been validated against the schema.

        if self._ctx.current_abspath:
            loading_what: PathType = self._ctx.current_abspath
        else:
            loading_what = 'data (no file)'

        _logger.debug('loading %s', loading_what)

        manifest_data = self._ctx.current_data['manifest']

        schema_version = str(manifest_data.get('version', SCHEMA_VERSION))

        # We want to make an ordered map from project names to
        # corresponding Project instances. Insertion order into this
        # map should reflect the final project order including
        # manifest import resolution, which is:
        #
        # 1. Imported projects from "manifest: self: import:"
        # 2. "manifest: projects:"
        # 3. Imported projects from "manifest: projects: ... import:"

        # Load data from "self:", including resolving any self imports.
        self._load_self(manifest_data)

        # Load data from "group-filter:".
        self._load_group_filter(manifest_data)

        # Add this manifest's projects to the map, and handle imported
        # projects and group-filter values.
        url_bases = {r['name']: r['url-base'] for r in
                     manifest_data.get('remotes', [])}
        defaults = self._load_defaults(manifest_data.get('defaults', {}),
                                       url_bases)
        self._load_projects(manifest_data, url_bases, defaults)

        # The manifest is resolved; perform post-resolution validation.
        self._check_paths_are_unique()

        # Set up top-level state which relies on the resolved and
        # validated manifest.
        if self._top_level:
            # Create the ManifestProject instance.
            mp = ManifestProject(
                path=self._config_path if self.topdir else self.yaml_path,
                west_commands=self._ctx.manifest_west_commands,
                topdir=self.topdir, userdata=self.userdata)

            # Save the resulting projects and initialize lookup tables
            # that rely on the ManifestProject existing.
            self._projects = list(self._ctx.projects.values())
            self._projects.insert(MANIFEST_PROJECT_INDEX, mp)
            self._projects_by_name: Dict[str, Project] = {'manifest': mp}
            self._projects_by_name.update(self._ctx.projects)
            self._projects_by_rpath: Dict[Path, Project] = {}  # resolved paths
            if self.topdir:
                for p in self.projects:
                    if TYPE_CHECKING:
                        # The typing module can't tell that self.topdir
                        # being truthy guarantees p.abspath is a str, not None.
                        assert p.abspath

                    self._projects_by_rpath[Path(p.abspath).resolve()] = p

            # Update self.group_filter
            #
            # For schema version 0.10 or later, there's no point in
            # overwriting these attributes for anything except the top
            # level manifest: all the other ones we've loaded above
            # during import resolution are already garbage.
            #
            # For schema version 0.9, we only want to warn once, at the
            # top level, if the distinction actually matters.
            self.group_filter = self._final_group_filter(schema_version)

        _logger.debug(f'loaded {loading_what}')

    def _load_group_filter(self, manifest_data: Dict[str, Any]) -> None:
        # Update self._ctx.group_filter_q from manifest_data.

        if 'group-filter' not in manifest_data:
            _logger.debug('group-filter: unset')
            return

        raw_filter: List[RawGroupType] = manifest_data['group-filter']
        if not raw_filter:
            self._malformed('"manifest: group-filter:" may not be empty')

        group_filter = self._validated_group_filter('manifest', raw_filter)
        _logger.debug('group-filter: %s', group_filter)

        self._ctx.group_filter_q.appendleft(group_filter)

        if self._top_level:
            self._top_level_group_filter = group_filter

    def _validated_group_filter(
            self, source: Optional[str], raw_filter: List[RawGroupType]
    ) -> GroupFilterType:
        # Helper function for cleaning up nonempty manifest:
        # group-filter: and manifest.group-filter values.

        if source is not None:
            source += ' '
        else:
            source = ''

        ret: GroupFilterType = []
        for item in raw_filter:
            if not isinstance(item, str):
                item = str(item)

            if (not item) or (item[0] not in ('+', '-')):
                self._malformed(
                    f'{source}group filter contains invalid item "{item}"; '
                    'this must begin with "+" or "-"')

            group = item[1:]
            if not is_group(group):
                self._malformed(
                    f'{source}group filter contains invalid item "{item}"; '
                    f'"{group}" is an invalid group name')

            ret.append(item)

        return ret

    def _load_self(self, manifest_data: Dict[str, Any]) -> None:
        # Handle the "self:" section in the manifest data, including
        # import resolution and extension commands.

        slf: Optional[Dict[str, Any]] = manifest_data.get('self')

        if not slf:
            return None

        yaml_path = slf.get('path')
        if 'path' in slf and not yaml_path:
            self._malformed(f'self: path: is {yaml_path}; this value '
                            'must be nonempty if present')
        self.yaml_path = yaml_path

        imp = slf.get('import')
        if imp is not None:
            _logger.debug(f'resolving self import {imp}')
            self._import_from_self(imp)
            _logger.debug('resolved self import')

        userdata = slf.get('userdata')
        self.userdata = userdata

        # The current manifest data's west-comands comes first because
        # we treat imports from self as if they are defined "before"
        # the contents in the higher level manifest, so any west
        # commands imported from self have higher precedence.
        west_commands = slf.get('west-commands')
        if west_commands:
            assert isinstance(west_commands, str)
            west_commands = [west_commands]
        else:
            west_commands = []
        self._ctx.manifest_west_commands[:] = _west_commands_merge(
            self._ctx.manifest_west_commands, west_commands)

    def _assert_imports_ok(self) -> None:
        # Sanity check that we aren't calling code that does importing
        # if the flags tell us not to.
        #
        # Could be deleted if this feature stabilizes and we never hit
        # this assertion.

        assert not self._ctx.import_flags & ImportFlag.IGNORE

    def _import_from_self(self, imp: Any) -> None:
        # Recursive helper to import projects from the manifest repository.
        #
        # The 'imp' argument is the loaded value of "foo" in "self:
        # import: foo".
        #
        # All data is read from the file system. Requests to read
        # files which don't exist or aren't ordinary files/directories
        # raise MalformedManifest.
        #
        # This is unlike importing from projects -- for projects, data
        # are read from Git (treating it as a content-addressable file
        # system) with a fallback on self._ctx.project_importer.

        imptype = type(imp)
        if imptype == bool:
            self._malformed(f'got "self: import: {imp}" of boolean')
        elif self._ctx.import_flags & ImportFlag.IGNORE:
            # If we're ignoring imports altogether, this is fine.
            _logger.debug('ignored self import')
            return
        elif self.topdir is None:
            # The import is well-formed and we aren't ignoring
            # imports, but we have no topdir, and therefore have no
            # good way to import this data within the existing API.
            # Just fail instead.
            #
            # (We cannot pass a ManifestProject to the importer
            # because that class is basically legacy code now, and
            # we don't want to add any more users of it to the
            # public west.manifest API. If someone complains about
            # the missing feature and has a suggestion for how to
            # generalize this that doesn't involve using
            # ManifestProject here, we can think about it then.)
            raise ManifestImportFailed(None, imp)

        self._assert_imports_ok()
        self.has_imports = True

        if imptype == str:
            self._import_path_from_self(imp)
        elif imptype == list:
            for subimp in imp:
                self._import_from_self(subimp)
        elif imptype == dict:
            self._import_map_from_self(imp)
        else:
            self._malformed(
                f'"self: import: {imp}" has invalid type {imptype}')

    def _import_path_from_self(self, imp: str) -> None:
        pathobj = Path(imp)
        if pathobj.is_absolute():
            self._malformed(f'{imp} is an absolute path')
        if TYPE_CHECKING:
            assert self.repo_abspath is not None
        pathobj_abs = (self.repo_abspath / pathobj).resolve()

        if pathobj_abs.is_file():
            _logger.debug('found submanifest file: %s', imp)
            self._import_pathobj_from_self(pathobj_abs, pathobj)
        elif pathobj_abs.is_dir():
            _logger.debug('found submanifest directory: %s', imp)
            for yml in filter(_is_yml, sorted(pathobj_abs.iterdir())):
                _logger.debug('found submanifest file: %s', yml)
                self._import_pathobj_from_self(yml, pathobj / yml.name)
        else:
            if pathobj_abs.exists():
                hint = 'this is neither a file nor a directory'
            else:
                hint = 'file not found'
            self._malformed(f'"self: import: {imp}": {hint}')

    def _import_pathobj_from_self(self, pathobj_abs: Path,
                                  pathobj: Path) -> None:
        # - pathobj_abs: the resolved path to the manifest file
        # - pathobj: same, but relative to self.repo_abspath as obtained
        #   from the import data

        # Destructively merge imported content into self._ctx. The
        # intermediate manifest is thrown away; we're just
        # using __init__ as a function here.
        child_ctx = self._ctx._replace(
            current_abspath=pathobj_abs,
            current_relpath=pathobj,
            current_data=pathobj_abs.read_text(encoding=Manifest.encoding)
        )
        try:
            Manifest(topdir=self.topdir, internal_import_ctx=child_ctx)
        except RecursionError as e:
            raise _ManifestImportDepth(None, pathobj) from e

    def _import_map_from_self(self, imp: Dict) -> None:
        # imap may introduce additional constraints on self._ctx, such
        # as a stricter imap_filter or a longer path_prefix.
        #
        # We therefore need to compose them during the recursive import.

        imap = self._load_imap(imp, f'manifest file {self.abspath}')
        imap_filter = _compose_imap_filters(self._ctx.imap_filter,
                                            _imap_filter(imap))
        path_prefix = self._ctx.path_prefix / imap.path_prefix

        pathobj = Path(imap.file)
        if pathobj.is_absolute():
            self._malformed(f'"self: import: {imp}" contains an absolute path')

        if TYPE_CHECKING:
            assert self.repo_abspath is not None
        pathobj_abs = self.repo_abspath / pathobj
        if pathobj_abs.is_dir():
            # Use an iterator in case there are a lot of files in there.
            to_import: Any = (f for f in sorted(pathobj_abs.iterdir())
                              if _is_yml(f))
        else:
            to_import = iter([pathobj_abs])

        for import_abs in to_import:
            child_ctx = self._ctx._replace(
                imap_filter=imap_filter,
                path_prefix=path_prefix,
                current_abspath=import_abs,
                current_relpath=pathobj / import_abs.name,
                current_data=import_abs.read_text(encoding=Manifest.encoding)
            )
            try:
                Manifest(topdir=self.topdir, internal_import_ctx=child_ctx)
            except RecursionError as e:
                raise _ManifestImportDepth(None, import_abs) from e

    def _load_defaults(self, defaults: Dict[str, Any],
                       url_bases: Dict[str, str]) -> _defaults:
        # md = manifest defaults (dictionary with values parsed from
        # the manifest)
        mdrem: Optional[str] = defaults.get('remote')
        if mdrem:
            # The default remote name, if provided, must refer to a
            # well-defined remote.
            if mdrem not in url_bases:
                self._malformed(f'default remote {mdrem} is not defined')
        return _defaults(mdrem, defaults.get('revision', _DEFAULT_REV))

    def _load_projects(self, manifest: Dict[str, Any],
                       url_bases: Dict[str, str],
                       defaults: _defaults) -> None:
        # Load projects and add them to self._ctx.projects.

        if 'projects' not in manifest:
            return

        have_imports = []
        names = set()
        for pd in manifest['projects']:
            project = self._load_project(pd, url_bases, defaults)
            name = project.name

            if not _imap_filter_allows(self._ctx.imap_filter, project):
                _logger.debug(f'project {name} in file {self.abspath} ' +
                              'ignored: an importing manifest blocked or '
                              'did not allow it')
                continue

            if name in names:
                # Project names must be unique within a manifest.
                self._malformed(f'project name {name} used twice in ' +
                                (self.abspath or 'the same manifest'))
            names.add(name)

            # Add the project to the map if it's new.
            added = self._add_project(project)
            if added:
                # Track project imports unless we are ignoring those.
                imp = pd.get('import')
                if imp:
                    if self._ctx.import_flags & (ImportFlag.IGNORE |
                                                 ImportFlag.IGNORE_PROJECTS):
                        _logger.debug(
                            f'project {project}: ignored import ({imp})')
                    else:
                        have_imports.append((project, imp))

        # Handle imports from new projects in our "projects:" section.
        for project, imp in have_imports:
            self._import_from_project(project, imp)

    def _load_project(self, pd: Dict, url_bases: Dict[str, str],
                      defaults: _defaults) -> Project:
        # pd = project data (dictionary with values parsed from the
        # manifest)

        name = pd['name']

        # The name "manifest" cannot be used as a project name; it
        # is reserved to refer to the manifest repository itself
        # (e.g. from "west list"). Note that this has not always
        # been enforced, but it is part of the documentation.
        if name == 'manifest':
            self._malformed('no project can be named "manifest"')

        # Figure out the project's fetch URL:
        #
        # - url is tested first (and can't be used with remote or repo-path)
        # - remote is tested next (and must be defined if present)
        # - default remote is tested last, if there is one
        url = pd.get('url')
        remote = pd.get('remote')
        repo_path = pd.get('repo-path')
        if remote and url:
            self._malformed(f'project {name} has both "remote: {remote}" '
                            f'and "url: {url}"')
        if defaults.remote and not (remote or url):
            remote = defaults.remote

        if url:
            if repo_path:
                self._malformed(f'project {name} has "repo_path: {repo_path}" '
                                f'and "url: {url}"')
        elif remote:
            if remote not in url_bases:
                self._malformed(f'project {name} remote {remote} '
                                'is not defined')
            url = url_bases[remote] + '/' + (repo_path or name)
        else:
            self._malformed(
                f'project {name} '
                'has no remote or url and no default remote is set')

        # The project's path needs to respect any import: path-prefix,
        # regardless of self._ctx.import_flags. The 'ignore' type flags
        # just mean ignore the imported data. The path-prefix in this
        # manifest affects the project no matter what.
        imp = pd.get('import', None)
        if isinstance(imp, dict):
            pfx = self._load_imap(imp, f'project {name}').path_prefix
        else:
            pfx = ''

        # Historically, path attributes came directly from the manifest data
        # itself and were passed along to the Project constructor unmodified.
        # When we added path-prefix support, we needed to introduce pathlib
        # wrappers around the pd['path'] value as is done here.
        #
        # Since west is a git wrapper and git prefers to work with
        # POSIX paths in general, we've decided for now to force paths
        # to POSIX style in all circumstances. If this breaks
        # anything, we can always revisit, maybe adding a 'nativepath'
        # attribute or something like that.
        path = (self._ctx.path_prefix / pfx / pd.get('path', name)).as_posix()

        raw_groups = pd.get('groups')
        if raw_groups:
            self._validate_project_groups(name, raw_groups)
            groups: GroupsType = [str(group) for group in raw_groups]
        else:
            groups = []

        if imp and groups:
            # Maybe there is a sensible way to combine the two of these.
            # but it's not clear what it is. Let's avoid weird edge cases
            # like "what do I do about a project whose group is disabled
            # that I need to import data from?".
            self._malformed(
                f'project {name}: "groups" cannot be combined with "import"')

        userdata = pd.get('userdata')

        ret = Project(name, url, description=pd.get('description'),
                      revision=pd.get('revision', defaults.revision),
                      path=path,
                      submodules=self._load_submodules(pd.get('submodules'),
                                                       f'project {name}'),
                      clone_depth=pd.get('clone-depth'),
                      west_commands=pd.get('west-commands'),
                      topdir=self.topdir, remote_name=remote,
                      groups=groups,
                      userdata=userdata)

        # Make sure the return Project's path does not escape the
        # workspace. We can't use escapes_directory() as that
        # resolves paths, which has proven to break some existing
        # users who use symlinks to existing project repositories
        # outside the workspace as a cache.
        #
        # Instead, normalize the path and make sure it's neither
        # absolute nor starts with a '..'. This is intended to be
        # a purely lexical operation which should therefore ignore
        # symbolic links.
        ret_norm = os.path.normpath(ret.path)

        # To be "really" absolute, a Windows path must include the "drive" like C:\\, D:\\.
        # But here we also want to block drive-less "half-breeds".
        # Note this question has confused Python which changed the `.isabs()` behavior in 3.13
        if ret_norm[0] in '/\\' or os.path.isabs(ret_norm):
            self._malformed(f'project "{ret.name}" has absolute path '
                            f'{ret.path}; this must be relative to the '
                            f'workspace topdir' +
                            (f' ({self.topdir})' if self.topdir else ''))

        if ret_norm.startswith('..'):
            self._malformed(f'project "{name}" path {ret.path} '
                            f'normalizes to {ret_norm}, which escapes '
                            f'the workspace topdir')

        if Path(ret_norm).parts[0] == util.WEST_DIR:
            self._malformed(f'project "{name}" path {ret.path} '
                            f'is in the {util.WEST_DIR} directory')

        return ret

    def _validate_project_groups(self, project_name: str,
                                 raw_groups: List[RawGroupType]):
        for raw_group in raw_groups:
            if not is_group(raw_group):
                self._malformed(f'project {project_name}: '
                                f'invalid group "{raw_group}"')

    def _load_submodules(self, submodules: Any, src: str) -> SubmodulesType:
        # Gets a list of Submodules objects or boolean from the manifest
        # *submodules* value.
        #
        # If submodules is a list[dict], checks the format of elements
        # and converts the list to a List[Submodule].
        #
        # If submodules is a bool, returns its value (True means that
        # all project submodules should be considered and False means
        # all submodules should be ignored).
        #
        # If submodules is None, returns False.
        #
        # All errors raise MalformedManifest.
        #
        # :param submodules: content of the manifest submodules value.
        # :param src: human readable source of the submodules data

        # A missing 'submodules' is the same thing as False.
        if submodules is None:
            return False

        # A bool should be returned as-is.
        if isinstance(submodules, bool):
            return submodules

        # Convert lists[dict] to list[Submodules].
        if isinstance(submodules, list):
            ret = []
            for index, value in enumerate(submodules):
                if _is_submodule_dict_ok(value):
                    ret.append(Submodule(**value))
                else:
                    self._malformed(f'{src}: invalid submodule element '
                                    f'{value} at index {index}')
            return ret

        self._malformed(f'{src}: invalid submodules: {submodules} '
                        f'has type {type(submodules)}; '
                        'expected a list or boolean')

    def _import_from_project(self, project: Project, imp: Any):
        # Recursively resolve a manifest import from 'project'.
        #
        # - project: Project instance to import from
        # - imp: the parsed value of project's import key (string, list, etc.)

        pfr = self._pfr(project)
        if pfr == PFR.INACTIVE:
            _logger.debug(f'project {project.name} is inactive due to '
                          'manifest.project-filter; ignoring its "import:"')
            return

        self._assert_imports_ok()
        self.has_imports = True

        imptype = type(imp)
        if imptype == bool:
            # We should not have been called unless the import was truthy.
            assert imp
            self._import_path_from_project(project, _WEST_YML)
        elif imptype == str:
            self._import_path_from_project(project, imp)
        elif imptype == list:
            for subimp in imp:
                self._import_from_project(project, subimp)
        elif imptype == dict:
            self._import_map_from_project(project, imp)
        else:
            self._malformed(f'{project.name_and_path}: invalid import {imp} '
                            f'type: {imptype}')

    def _import_path_from_project(self, project: Project, path: str) -> None:
        # Import data from git at the given path at revision manifest-rev.
        # Fall back on self._ctx.project_importer if that fails.

        _logger.debug(f'resolving import {path} for {project}')
        imported = self._import_content_from_project(project, path)
        if imported is None:
            # This can happen if self._ctx.project_importer returns None.
            # It means there's nothing to do.
            return

        for data in imported:
            self._import_data_from_project(project, data, None)

        _logger.debug(f'done resolving import {path} for {project}')

    def _import_map_from_project(self, project: Project,
                                 imp: Dict) -> None:
        imap = self._load_imap(imp, f'project {project.name}')

        _logger.debug(f'resolving import {imap} for {project}')

        imported = self._import_content_from_project(project, imap.file)
        if imported is None:
            return

        for data in imported:
            self._import_data_from_project(project, data, imap)

        _logger.debug(f'done resolving import {imap} for {project}')

    def _import_data_from_project(self, project: Project, data: Any,
                                  imap: Optional[_import_map]) -> None:
        # Destructively add the imported data into our 'projects' map.

        if imap is not None:
            imap_filter = _compose_imap_filters(self._ctx.imap_filter,
                                                _imap_filter(imap))
            imap_path_prefix = imap.path_prefix
        else:
            imap_filter = self._ctx.imap_filter
            imap_path_prefix = '.'

        child_ctx = self._ctx._replace(
            imap_filter=imap_filter,
            path_prefix=self._ctx.path_prefix / imap_path_prefix,
            current_abspath=None,
            current_relpath=None,
            current_data=data,
            current_repo_abspath=(Path(project.abspath) if project.abspath
                                  else None),
            # If the manifest data we imported from the project has
            # west commands, they logically belong to 'project'.
            # We therefore use a separate list for tracking them
            # from our current list.
            manifest_west_commands=[]
        )
        try:
            submanifest = Manifest(topdir=self.topdir,
                                   internal_import_ctx=child_ctx)
        except RecursionError as e:
            raise _ManifestImportDepth(None, imap.file if imap else None) \
                from e

        # Patch up any extension commands in the imported data
        # by allocating them to the project.
        project.west_commands = _west_commands_merge(
            project.west_commands,
            submanifest._ctx.manifest_west_commands)

    def _import_content_from_project(self, project: Project,
                                     path: str) -> ImportedContentType:
        if not (self._ctx.import_flags & ImportFlag.FORCE_PROJECTS) and \
           project.is_cloned():
            try:
                content = _manifest_content_at(project, path, Manifest.encoding)
            except MalformedManifest as mm:
                self._malformed(mm.args[0])
            except FileNotFoundError:
                # We may need to fetch a new manifest-rev, e.g. if
                # revision is a branch that didn't used to have a
                # manifest, but now does.
                content = self._ctx.project_importer(project, path)
            except subprocess.CalledProcessError:
                # We may need a new manifest-rev, e.g. if revision is
                # a SHA we don't have yet.
                content = self._ctx.project_importer(project, path)
        else:
            # We need to clone this project, or we were specifically
            # asked to use the importer.
            content = self._ctx.project_importer(project, path)

        if isinstance(content, str):
            content = [content]

        return content

    def _load_imap(self, imp: Dict, src: str) -> _import_map:
        # Convert a parsed self or project import value from YAML into
        # an _import_map namedtuple.

        # Work on a copy in case the caller needs the full value.
        copy = dict(imp)
        # Preserve deprecated whitelist/blacklist terms
        name_allowlist = copy.pop(
            'name-allowlist', copy.pop('name-whitelist', [])
        )
        path_allowlist = copy.pop(
            'path-allowlist', copy.pop('path-whitelist', [])
        )
        name_blocklist = copy.pop(
            'name-blocklist', copy.pop('name-blacklist', [])
        )
        path_blocklist = copy.pop(
            'path-blocklist', copy.pop('path-blacklist', [])
        )

        ret = _import_map(copy.pop('file', _WEST_YML),
                          name_allowlist,
                          path_allowlist,
                          name_blocklist,
                          path_blocklist,
                          copy.pop('path-prefix', ''))

        # Check that the value is OK.
        #
        # If you modify the error handling here, be sure to
        # update test_import_map_error_handling() as needed.
        if copy:
            # We popped out all of the valid keys already.
            self._malformed(f'{src}: invalid import contents: {copy}')
        elif not _is_imap_list(ret.name_allowlist):
            self._malformed(f'{src}: bad import name-allowlist '
                            f'{ret.name_allowlist}')
        elif not _is_imap_list(ret.path_allowlist):
            self._malformed(f'{src}: bad import path-allowlist '
                            f'{ret.path_allowlist}')
        elif not _is_imap_list(ret.name_blocklist):
            self._malformed(f'{src}: bad import name-blocklist '
                            f'{ret.name_blocklist}')
        elif not _is_imap_list(ret.path_blocklist):
            self._malformed(f'{src}: bad import path-blocklist '
                            f'{ret.path_blocklist}')
        elif not isinstance(ret.path_prefix, str):
            self._malformed(f'{src}: bad import path-prefix '
                            f'{ret.path_prefix}; expected str, not '
                            f'{type(ret.path_prefix)}')

        return ret

    def _add_project(self, project: Project) -> bool:
        # Add the project to our map if we don't already know about it.
        # Return the result.

        if project.name not in self._ctx.projects:
            self._check_project_name(project)
            self._ctx.projects[project.name] = project
            _logger.debug('added project %s path %s revision %s%s%s',
                          project.name, project.path, project.revision,
                          (f' from {self.abspath}' if self.abspath else ''),
                          (f' groups {project.groups}' if project.groups
                           else ''))
            return True
        else:
            return False

    def _check_project_name(self, project):
        name = project.name
        if _INVALID_PROJECT_NAME_RE.search(name):
            self._malformed(f'Invalid project name: {name}')
        if _RESERVED_PROJECT_NAME_RE.search(name):
            if self._ctx.current_abspath:
                context = f' (defined in {self._ctx.current_abspath})'
            elif self._ctx.current_repo_abspath:
                context = f' (loaded from {self._ctx.current_repo_abspath})'
            if self._ctx.project_filter:
                raise MalformedConfig(
                    f'project "{name}"{context} contains comma (",") or '
                    'whitespace; this is incompatible with use of the '
                    'manifest.project-filter option. '
                    '(These characters will be forbidden entirely in a '
                    'future version of west.)')
            else:
                _logger.warning(
                    f'project "{name}"{context} contains comma (",") or '
                    'whitespace; this will be forbidden in a future version '
                    'of west')

    def _check_paths_are_unique(self) -> None:
        ppaths: Dict[Path, Project] = {}
        for name, project in self._ctx.projects.items():
            pp = Path(project.path)
            if self._top_level and pp == self._config_path:
                self._malformed(f'project {name} path "{project.path}" '
                                'is taken by the manifest repository')
            other = ppaths.get(pp)
            if other:
                self._malformed(f'project {name} path "{project.path}" '
                                f'is taken by project {other.name}')
            ppaths[pp] = project

    def _final_group_filter(self, schema_version: str):
        # Update self.group_filter based on the schema version.

        if schema_version == '0.9':
            # If the user requested v0.9.x group-filter semantics,
            # provide them, but emit a warning that can't be silenced
            # if group filters were used anywhere.
            #
            # Hopefully no users ever actually see this warning.

            if any(self._ctx.group_filter_q):
                _logger.warning(
                    "providing deprecated group-filter semantics "
                    "due to explicit 'manifest: version: 0.9'; "
                    "for the new semantics, use "
                    "'manifest: version: \"0.10\"' or later")

                # Set attribute for white-box testing the above warning.
                self._legacy_group_filter_warned = True

            return self._top_level_group_filter

        else:
            _logger.debug(
                'group filters in precedence order (later is higher): %s',
                self._ctx.group_filter_q)
            for group_filter in self._ctx.group_filter_q:
                _update_disabled_groups(self._disabled_groups, group_filter)
            ret = [f'-{g}' for g in self._disabled_groups]
            _logger.debug('final top level group-filter: %s', ret)
            return ret

class _PatchedConfiguration(Configuration):
    # Internal helper class that fakes out manifest.path and manifest.file
    # to point at another location. Used by Manifest.from_file().

    def __init__(self, patch_dict, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__patch_dict = patch_dict

    def get(self, option, **kwargs):
        if option in self.__patch_dict:
            return self.__patch_dict[option]
        return super().get(option, **kwargs)
