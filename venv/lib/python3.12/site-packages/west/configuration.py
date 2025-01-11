# Copyright (c) 2018, 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''West configuration file handling.

West follows Git-like conventions for configuration file locations.
There are three types of configuration file: system-wide files apply
to all users on the current machine, global files apply to the current
user, and local files apply to the current west workspace.

System files:

- Linux: ``/etc/westconfig``
- macOS: ``/usr/local/etc/westconfig``
- Windows: ``%PROGRAMDATA%\\west\\config``

Global files:

- Linux: ``~/.westconfig`` or (if ``$XDG_CONFIG_HOME`` is set)
  ``$XDG_CONFIG_HOME/west/config``
- macOS: ``~/.westconfig``
- Windows: ``.westconfig`` in the user's home directory, as determined
  by os.path.expanduser.

Local files:

- Linux, macOS, Windows: ``<workspace-topdir>/.west/config``

You can override these files' locations with the ``WEST_CONFIG_SYSTEM``,
``WEST_CONFIG_GLOBAL``, and ``WEST_CONFIG_LOCAL`` environment variables.

Configuration values from later configuration files override configuration
from earlier ones. Local values have highest precedence, and system values
lowest.
'''

import configparser
import os
from pathlib import PureWindowsPath, Path
import platform
from enum import Enum
from typing import Any, Dict, Iterable, List, Optional, Tuple, TYPE_CHECKING
import warnings

from west.util import WEST_DIR, west_dir, WestNotFound, PathType

class MalformedConfig(Exception):
    '''The west configuration was malformed.
    '''

def _configparser():            # for internal use
    return configparser.ConfigParser(allow_no_value=True)

class _InternalCF:
    # For internal use only; convenience interface for reading and
    # writing INI-style [section] key = value configuration files,
    # but presenting a west-style section.key = value style API.

    @staticmethod
    def parse_key(dotted_name: str):
        section_child = dotted_name.split('.', 1)
        if len(section_child) != 2:
            raise ValueError(f"Invalid key name: '{dotted_name}'")
        return section_child

    @staticmethod
    def from_path(path: Optional[Path]) -> Optional['_InternalCF']:
        return _InternalCF(path) if path and path.exists() else None

    def __init__(self, path: Path):
        self.path = path
        self.cp = _configparser()
        read_files = self.cp.read(path, encoding='utf-8')
        if len(read_files) != 1:
            raise FileNotFoundError(path)

    def __contains__(self, option: str) -> bool:
        section, key = _InternalCF.parse_key(option)

        return section in self.cp and key in self.cp[section]

    def get(self, option: str):
        return self._get(option, self.cp.get)

    def getboolean(self, option: str):
        return self._get(option, self.cp.getboolean)

    def getint(self, option: str):
        return self._get(option, self.cp.getint)

    def getfloat(self, option: str):
        return self._get(option, self.cp.getfloat)

    def _get(self, option, getter):
        section, key = _InternalCF.parse_key(option)

        try:
            return getter(section, key)
        except (configparser.NoOptionError, configparser.NoSectionError):
            raise KeyError(option)

    def set(self, option: str, value: Any):
        section, key = _InternalCF.parse_key(option)

        if section not in self.cp:
            self.cp[section] = {}

        self.cp[section][key] = value

        with open(self.path, 'w', encoding='utf-8') as f:
            self.cp.write(f)

    def delete(self, option: str):
        section, key = _InternalCF.parse_key(option)

        if section not in self.cp:
            raise KeyError(option)

        del self.cp[section][key]
        if not self.cp[section].items():
            del self.cp[section]

        with open(self.path, 'w', encoding='utf-8') as f:
            self.cp.write(f)

class ConfigFile(Enum):
    '''Types of west configuration file.

    Enumeration members:

    - SYSTEM: system level configuration shared by all users
    - GLOBAL: global or user-wide configuration
    - LOCAL: per-workspace configuration
    - ALL: all three of the above, where applicable
    '''
    ALL = 1
    SYSTEM = 2
    GLOBAL = 3
    LOCAL = 4

class Configuration:
    '''Represents the available configuration options and their values.

    Allows getting, setting, and deleting configuration options
    in the system, global, and local files.

    Setting values affects files immediately and is not protected against
    concurrent reads. The caller is responsible for any necessary
    mutual exclusion.

    WEST_CONFIG_* environment variables take effect when and only when
    a Configuration object is created. This can be used to point
    different objects at different files.

    If no topdir argument is passed to the constructor and WEST_CONFIG_LOCAL
    is not defined then the object does not point to any local file.

    '''

    def __init__(self, topdir: Optional[PathType] = None):
        '''Load the system, global, and workspace configurations and
        make them available for the user.

        :param topdir: workspace location; may be None
        '''

        local_path = _location(ConfigFile.LOCAL, topdir=topdir,
                               find_local=False) or None

        self._system_path = Path(_location(ConfigFile.SYSTEM))
        self._global_path = Path(_location(ConfigFile.GLOBAL))
        self._local_path = Path(local_path) if local_path is not None else None

        self._system = _InternalCF.from_path(self._system_path)
        self._global = _InternalCF.from_path(self._global_path)
        self._local = _InternalCF.from_path(self._local_path)

    def get(self, option: str,
            default: Optional[str] = None,
            configfile: ConfigFile = ConfigFile.ALL) -> Optional[str]:
        '''Get a configuration option's value as a string.

        :param option: option to get, in 'foo.bar' form
        :param default: default value to return if option is missing
        :param configfile: type of config file look for the value in
        '''
        return self._get(lambda cf: cf.get(option), default, configfile)

    def getboolean(self, option: str,
                   default: bool = False,
                   configfile: ConfigFile = ConfigFile.ALL) -> bool:
        '''Get a configuration option's value as a bool.

        The configparser module's conversion to boolean is applied
        to any value discovered. Invalid values raise ValueError.

        :param option: option to get, in 'foo.bar' form
        :param default: default value to return if option is missing
        :param configfile: type of config file to look for the value in
        '''
        return self._get(lambda cf: cf.getboolean(option), default, configfile)

    def getint(self, option: str,
               default: Optional[int] = None,
               configfile: ConfigFile = ConfigFile.ALL) -> Optional[int]:
        '''Get a configuration option's value as an int.

        :param option: option to get, in 'foo.bar' form
        :param default: default value to return if option is missing
        :param configfile: type of config file to look for the value in
        '''
        return self._get(lambda cf: cf.getint(option), default, configfile)

    def getfloat(self, option: str,
                 default: Optional[float] = None,
                 configfile: ConfigFile = ConfigFile.ALL) -> Optional[float]:
        '''Get a configuration option's value as a float.

        :param option: option to get, in 'foo.bar' form
        :param default: default value to return if option is missing
        :param configfile: type of config file to look for the value in
        '''
        return self._get(lambda cf: cf.getfloat(option), default, configfile)

    def _get(self, getter, default, configfile):
        for cf in self._whence(configfile):
            if cf is None:
                continue
            try:
                return getter(cf)
            except KeyError:
                pass

        return default

    def _whence(self, configfile):
        if configfile == ConfigFile.ALL:
            if self._local is not None:
                return [self._local, self._global, self._system]
            return [self._global, self._system]
        elif configfile == ConfigFile.SYSTEM:
            return [self._system]
        elif configfile == ConfigFile.GLOBAL:
            return [self._global]
        elif configfile == ConfigFile.LOCAL:
            if self._local is None:
                raise MalformedConfig('local configuration file not found')
            return [self._local]
        else:
            raise ValueError(configfile)

    def set(self, option: str, value: Any,
            configfile: ConfigFile = ConfigFile.LOCAL) -> None:
        '''Set a configuration option's value.

        The write to the configuration file takes effect
        immediately. No concurrency protection is performed against
        concurrent access from the time that this Configuration object
        was created. If the file may have been modified since that
        time, either create a new Configuration object before using
        this method or lose the intervening modifications.

        :param option: option to set, in 'foo.bar' form
        :param value: value to set option to
        :param configfile: type of config file to set the value in
        '''

        if configfile == ConfigFile.ALL:
            # We need a real configuration file; ALL doesn't make sense here.
            raise ValueError(configfile)
        elif configfile == ConfigFile.LOCAL:
            if self._local_path is None:
                raise ValueError(f'{configfile}: file not found; retry in a '
                                 'workspace or set WEST_CONFIG_LOCAL')
            if not self._local_path.exists():
                self._local = self._create(self._local_path)
            if TYPE_CHECKING:
                assert self._local
            self._local.set(option, value)
        elif configfile == ConfigFile.GLOBAL:
            if not self._global_path.exists():
                self._global = self._create(self._global_path)
            if TYPE_CHECKING:
                assert self._global
            self._global.set(option, value)
        elif configfile == ConfigFile.SYSTEM:
            if not self._system_path.exists():
                self._system = self._create(self._system_path)
            if TYPE_CHECKING:
                assert self._system
            self._system.set(option, value)
        else:
            # Shouldn't happen.
            assert False, configfile

    @staticmethod
    def _create(path: Path) -> _InternalCF:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch(exist_ok=True)
        ret = _InternalCF.from_path(path)
        if TYPE_CHECKING:
            assert ret
        return ret

    def delete(self, option: str,
               configfile: Optional[ConfigFile] = None) -> None:
        '''Delete an option from the given file or files.

        If *option* is not set in the given *configfile*, KeyError is raised.

        :param option: option to delete, in 'foo.bar' form
        :param configfile: If ConfigFile.ALL, delete *option* in all files
                           where it is set.

                           If None, delete *option* only in the highest
                           precedence file where it is set.

                           Otherwise, delete from the given ConfigFile.
        '''

        if configfile == ConfigFile.ALL or configfile is None:
            found = False
            for cf in [self._local, self._global, self._system]:
                if cf and option in cf:
                    cf.delete(option)
                    if configfile is None:
                        return
                    found = True
            if not found:
                raise KeyError(option)
        elif configfile == ConfigFile.LOCAL:
            if not self._local:
                raise KeyError(option)
            self._local.delete(option)
        elif configfile == ConfigFile.GLOBAL:
            if not self._global:
                raise KeyError(option)
            self._global.delete(option)
        elif configfile == ConfigFile.SYSTEM:
            if not self._system:
                raise KeyError(option)
            self._system.delete(option)
        else:
            raise RuntimeError(f'bad configfile {configfile}')

    def _copy_to_configparser(self, cp: configparser.ConfigParser) -> None:
        # Internal API for main to use to maintain backwards
        # compatibility for existing extensions using the legacy
        # function-and-global-state APIs.

        def load(cf: _InternalCF):
            for section, contents in cf.cp.items():
                if section == 'DEFAULT':
                    continue
                if section not in cp:
                    cp.add_section(section)
                for key, value in contents.items():
                    cp[section][key] = value

        if self._system:
            load(self._system)
        if self._global:
            load(self._global)
        if self._local:
            load(self._local)

    def items(self, configfile: ConfigFile = ConfigFile.ALL
              ) -> Iterable[Tuple[str, Any]]:
        '''Iterator of option, value pairs.'''
        if configfile == ConfigFile.ALL:
            ret = {}
            ret.update(self._system_as_dict)
            ret.update(self._global_as_dict)
            ret.update(self._local_as_dict)
            return ret.items()

        if configfile == ConfigFile.SYSTEM:
            return self._system_as_dict.items()

        if configfile == ConfigFile.GLOBAL:
            return self._global_as_dict.items()

        if configfile == ConfigFile.LOCAL:
            return self._local_as_dict.items()

        raise RuntimeError(configfile)

    @property
    def _system_as_dict(self):
        return self._cf_to_dict(self._system)

    @property
    def _global_as_dict(self):
        return self._cf_to_dict(self._global)

    @property
    def _local_as_dict(self):
        return self._cf_to_dict(self._local)

    @staticmethod
    def _cf_to_dict(cf: Optional[_InternalCF]) -> Dict[str, Any]:
        ret: Dict[str, Any] = {}
        if cf is None:
            return ret
        for section, contents in cf.cp.items():
            if section == 'DEFAULT':
                continue
            for key, value in contents.items():
                ret[f'{section}.{key}'] = value
        return ret


# Global parser containing configuration values, for backwards
# compatibility. Populated in main.py to keep legacy extensions
# working following the deprecation of this API.
config = _configparser()

def _deprecated(old_function):
    warnings.warn(f'{old_function} is deprecated; '
                  'use a west.configuration.Configuration object',
                  DeprecationWarning, stacklevel=2)

def read_config(configfile: Optional[ConfigFile] = None,
                config: configparser.ConfigParser = config,
                topdir: Optional[PathType] = None) -> None:
    '''Read configuration files into *config*.

    Reads the files given by *configfile*, storing the values into the
    configparser.ConfigParser object *config*. If *config* is not
    given, the global `west.configuration.config` object is used.

    If *configfile* is given, only the files implied by its value are
    read. If not given, ``ConfigFile.ALL`` is used.

    If *configfile* requests local configuration options (i.e. if it
    is ``ConfigFile.LOCAL`` or ``ConfigFile.ALL``:

        - If *topdir* is given, topdir/.west/config is read

        - Next, if WEST_CONFIG_LOCAL is set in the environment, its
          contents (a file) are used.

        - Otherwise, the file system is searched for a local
          configuration file, and a failure to find one is ignored.

    :param configfile: a `west.configuration.ConfigFile`
    :param config: configuration object to read into
    :param topdir: west workspace root to read local options from
    '''
    _deprecated('read_config')

    if configfile is None:
        configfile = ConfigFile.ALL
    config.read(_gather_configs(configfile, topdir), encoding='utf-8')

def update_config(section: str, key: str, value: Any,
                  configfile: ConfigFile = ConfigFile.LOCAL,
                  topdir: Optional[PathType] = None) -> None:
    '''Sets ``section.key`` to *value* in the given configuration file.

    :param section: config section; will be created if it does not exist
    :param key: key to set in the given section
    :param value: value to set the key to
    :param configfile: `west.configuration.ConfigFile`, must not be ALL
    :param topdir: west workspace root to write local config options to

    The destination file to write is given by *configfile*. The
    default value (``ConfigFile.LOCAL``) writes to the local
    configuration file given by:

    - topdir/.west/config, if topdir is given, or
    - the value of 'WEST_CONFIG_LOCAL' in the environment, if set, or
    - the local configuration file in the west workspace
      found by searching the file system (raising WestNotFound if
      one is not found).
    '''
    _deprecated('update_config')

    if configfile == ConfigFile.ALL:
        # Not possible to update ConfigFile.ALL, needs specific conf file here.
        raise ValueError(f'invalid configfile: {configfile}')

    filename = _ensure_config(configfile, topdir)
    config = _configparser()
    config.read(filename)
    if section not in config:
        config[section] = {}
    config[section][key] = value
    with open(filename, 'w') as f:
        config.write(f)

def delete_config(section: str, key: str,
                  configfile: Optional[ConfigFile] = None,
                  topdir: Optional[PathType] = None) -> None:
    '''Delete the option section.key from the given file or files.

    :param section: section whose key to delete
    :param key: key to delete
    :param configfile: If ConfigFile.ALL, delete section.key in all files
                       where it is set.
                       If None, delete only from the highest-precedence
                       global or local file where it is set, allowing
                       lower-precedence values to take effect again.
                       If a list of ConfigFile enumerators, delete
                       from those files.
                       Otherwise, delete from the given ConfigFile.
    :param topdir: west workspace root to delete local options from

    Deleting the only key in a section deletes the entire section.

    If the option is not set, KeyError is raised.

    If an option is to be deleted from the local configuration file,
    it is:

    - topdir/.west/config, if topdir is given, or
    - the value of 'WEST_CONFIG_LOCAL' in the environment, if set, or
    - the local configuration file in the west workspace
      found by searching the file system (raising WestNotFound if
      one is not found).
    '''
    _deprecated('delete_config')

    stop = False
    if configfile is None:
        to_check = [_location(x, topdir=topdir) for x in
                    [ConfigFile.LOCAL, ConfigFile.GLOBAL]]
        stop = True
    elif configfile == ConfigFile.ALL:
        to_check = [_location(x, topdir=topdir) for x in
                    [ConfigFile.SYSTEM, ConfigFile.GLOBAL, ConfigFile.LOCAL]]
    elif isinstance(configfile, ConfigFile):
        to_check = [_location(configfile, topdir=topdir)]
    else:
        to_check = [_location(x, topdir=topdir) for x in configfile]

    found = False
    for path in to_check:
        config = _configparser()
        config.read(path)
        if section not in config or key not in config[section]:
            continue

        del config[section][key]
        if not config[section].items():
            del config[section]
        with open(path, 'w') as f:
            config.write(f)
        found = True
        if stop:
            break

    if not found:
        raise KeyError(f'{section}.{key}')

def _location(cfg: ConfigFile, topdir: Optional[PathType] = None,
              find_local: bool = True) -> str:
    # Making this a function that gets called each time you ask for a
    # configuration file makes it respect updated environment
    # variables (such as XDG_CONFIG_HOME, PROGRAMDATA) if they're set
    # during the program lifetime.
    #
    # Its existence is also relied on in the test cases, to ensure
    # that the WEST_CONFIG_xyz variables are respected and we're not about
    # to clobber the user's own configuration files.
    #
    # Make sure to use pathlib's / operator override to join paths in
    # any cases which might run on Windows, then call fspath() on the
    # final result. This lets the standard library do the work of
    # producing a canonical string name.
    env = os.environ

    if cfg == ConfigFile.ALL:
        raise ValueError('ConfigFile.ALL has no location')
    elif cfg == ConfigFile.SYSTEM:
        if 'WEST_CONFIG_SYSTEM' in env:
            return env['WEST_CONFIG_SYSTEM']

        plat = platform.system()

        if plat == 'Linux':
            return '/etc/westconfig'

        if plat == 'Darwin':
            return '/usr/local/etc/westconfig'

        if plat == 'Windows':
            return os.path.expandvars('%PROGRAMDATA%\\west\\config')

        if 'BSD' in plat:
            return '/etc/westconfig'

        if 'CYGWIN' in plat or 'MSYS_NT' in plat:
            # Cygwin can handle windows style paths, so make sure we
            # return one. We don't want to use os.path.join because
            # that uses '/' as separator character, and the ProgramData
            # variable is likely to be something like r'C:\ProgramData'.
            #
            # See https://github.com/zephyrproject-rtos/west/issues/300
            # for details.
            pd = PureWindowsPath(os.environ['ProgramData'])
            return os.fspath(pd / 'west' / 'config')

        raise ValueError('unsupported platform ' + plat)
    elif cfg == ConfigFile.GLOBAL:
        if 'WEST_CONFIG_GLOBAL' in env:
            return env['WEST_CONFIG_GLOBAL']

        if platform.system() == 'Linux' and 'XDG_CONFIG_HOME' in env:
            return os.path.join(env['XDG_CONFIG_HOME'], 'west', 'config')

        return os.fspath(Path.home() / '.westconfig')
    elif cfg == ConfigFile.LOCAL:
        if 'WEST_CONFIG_LOCAL' in env:
            return env['WEST_CONFIG_LOCAL']

        if topdir:
            return os.fspath(Path(topdir) / WEST_DIR / 'config')

        if find_local:
            # Might raise WestNotFound!
            return os.fspath(Path(west_dir()) / 'config')
        else:
            return ''
    else:
        raise ValueError(f'invalid configuration file {cfg}')

def _gather_configs(cfg: ConfigFile, topdir: Optional[PathType]) -> List[str]:
    # Find the paths to the given configuration files, in increasing
    # precedence order.
    ret = []

    if cfg == ConfigFile.ALL or cfg == ConfigFile.SYSTEM:
        ret.append(_location(ConfigFile.SYSTEM, topdir=topdir))
    if cfg == ConfigFile.ALL or cfg == ConfigFile.GLOBAL:
        ret.append(_location(ConfigFile.GLOBAL, topdir=topdir))
    if cfg == ConfigFile.ALL or cfg == ConfigFile.LOCAL:
        try:
            ret.append(_location(ConfigFile.LOCAL, topdir=topdir))
        except WestNotFound:
            pass

    return ret

def _ensure_config(configfile: ConfigFile, topdir: Optional[PathType]) -> str:
    # Ensure the given configfile exists, returning its path. May
    # raise permissions errors, WestNotFound, etc.
    loc = _location(configfile, topdir=topdir)
    path = Path(loc)

    if path.is_file():
        return loc

    path.parent.mkdir(parents=True, exist_ok=True)
    path.touch(exist_ok=True)
    return os.fspath(path)
