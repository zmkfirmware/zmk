# Copyright 2017 Square, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from . import util

import ctypes
import ctypes.util as ctypes_util
import os
import platform
import sys
import tempfile


class Library(object):
    """Wrapper to provide easy access to loading the J-Link SDK DLL.

    This class provides a convenience for finding and loading the J-Link DLL
    across multiple platforms, and accounting for the inconsistencies between
    Windows and nix-based platforms.

    Attributes:
      _standard_calls_: list of names of the methods for the API calls that
        must be converted to standard calling convention on the Windows
        platform.
      JLINK_SDK_NAME: name of the J-Link DLL on nix-based platforms.
      WINDOWS_JLINK_SDK_NAME: name of the J-Link DLL on Windows platforms.
    """

    _standard_calls_ = [
        'JLINK_Configure',
        'JLINK_DownloadFile',
        'JLINK_GetAvailableLicense',
        'JLINK_GetPCode',
        'JLINK_PrintConfig',
        'JLINK_EraseChip',
        'JLINK_SPI_Transfer',
        'JLINK_GetpFunc',
        'JLINK_GetMemZones',
        'JLINK_ReadMemZonedEx',
        'JLINK_SetHookUnsecureDialog',
        'JLINK_WriteMemZonedEx',
        'JLINK_DIALOG_Configure',
        'JLINK_DIALOG_ConfigureEx',
        'JLINK_EMU_GPIO_GetProps',
        'JLINK_EMU_GPIO_GetState',
        'JLINK_EMU_GPIO_SetState',
        'JLINK_EMU_AddLicense',
        'JLINK_EMU_EraseLicenses',
        'JLINK_EMU_GetLicenses',
        'JLINK_HSS_GetCaps',
        'JLINK_HSS_Start',
        'JLINK_HSS_Stop',
        'JLINK_HSS_Read',
        'JLINK_POWERTRACE_Control',
        'JLINK_POWERTRACE_Read',
        'JLINK_RTTERMINAL_Control',
        'JLINK_RTTERMINAL_Read',
        'JLINK_RTTERMINAL_Write',
        'JLINK_STRACE_Config',
        'JLINK_STRACE_Control',
        'JLINK_STRACE_Read',
        'JLINK_STRACE_Start',
        'JLINK_STRACE_Stop',
        'JLINK_SWD_GetData',
        'JLINK_SWD_GetU8',
        'JLINK_SWD_GetU16',
        'JLINK_SWD_GetU32',
        'JLINK_SWD_StoreGetRaw',
        'JLINK_SWD_StoreRaw',
        'JLINK_SWD_SyncBits',
        'JLINK_SWD_SyncBytes',
        'JLINK_SetFlashProgProgressCallback',
        'JLINKARM_BeginDownload',
        'JLINKARM_EndDownload',
        'JLINKARM_WriteMem'
    ]

    # On Linux and macOS, represents the library file name prefix
    # (the part just before .so or .dylib).
    #
    # This is the value appropriate when calling the find_library_{linux,darwin}()
    # functions below.
    #
    # Note: this "constant" is also used by downstream projects,
    # and should therefore be considered public (frozen) API.
    JLINK_SDK_NAME = 'libjlinkarm'

    # Linux/MacOS: The JLink shared object name, without any prefix like lib,
    # suffix like .so, .dylib or version number.
    #
    # This is the value appropriate when invoking the ctypes API find_libary().
    JLINK_SDK_OBJECT = 'jlinkarm'

    # Windows: these are suitable for both the ctypes find_library() API,
    # and for the directory scanning done in Library.find_library_windows()
    WINDOWS_32_JLINK_SDK_NAME = 'JLinkARM'
    WINDOWS_64_JLINK_SDK_NAME = 'JLink_x64'

    # Linux: Represents the dlinfo(3) associated to the jlinkarm shared library.
    #
    # Poor man's singleton: having this as a class member avoids to create
    # a new JLinkarmDlInfo() for each pylink.Library instance.
    #
    # For e.g., the simple 'pyocd list' command creates three instances
    # of the pylink.Library class, and it's worth not repeating the dlinfo()
    # dance three times.
    _dlinfo = None

    @classmethod
    def get_appropriate_windows_sdk_name(cls):
        """Returns the appropriate JLink SDK library name on Windows depending
        on 32bit or 64bit Python variant.

        SEGGER delivers two variants of their dynamic library on Windows:
          - ``JLinkARM.dll`` for 32-bit platform
          - ``JLink_x64.dll`` for 64-bit platform

        Args:
          cls (Library): the ``Library`` class

        Returns:
          The name of the library depending on the platform this module is run on.

        """
        if sys.maxsize == (2**63 - 1):
            return Library.WINDOWS_64_JLINK_SDK_NAME
        else:
            return Library.WINDOWS_32_JLINK_SDK_NAME

    @classmethod
    def can_load_library(cls, dllpath):
        """Test whether a library is the correct architecture to load.

        Args:
          cls (Library): the ``Library`` class
          dllpath (str): A path to a library.

        Returns:
          ``True`` if the library could be successfully loaded, ``False`` if not.
        """
        try:
            ctypes.CDLL(dllpath)
            return True
        except OSError:
            return False

    @classmethod
    def find_library_windows(cls):
        """Loads the SEGGER DLL from the windows installation directory.

        On Windows, these are found either under:
          - ``C:\\Program Files\\SEGGER\\JLink``
          - ``C:\\Program Files (x86)\\SEGGER\\JLink``.

        Args:
          cls (Library): the ``Library`` class

        Returns:
          The paths to the J-Link library files in the order that they are
          found.
        """
        dll = cls.get_appropriate_windows_sdk_name() + '.dll'
        root = 'C:\\'
        for d in os.listdir(root):
            dir_path = os.path.join(root, d)

            # Have to account for the different Program Files directories.
            if d.startswith('Program Files') and os.path.isdir(dir_path):
                dir_path = os.path.join(dir_path, 'SEGGER')
                if not os.path.isdir(dir_path):
                    continue

                # Find all the versioned J-Link directories.
                ds = filter(lambda x: x.startswith('JLink'), os.listdir(dir_path))
                for jlink_dir in ds:
                    # The DLL always has the same name, so if it is found, just
                    # return it.
                    lib_path = os.path.join(dir_path, jlink_dir, dll)
                    if os.path.isfile(lib_path):
                        yield lib_path

    @classmethod
    def find_library_linux(cls):
        """Loads the SEGGER DLL from the root directory.

        On Linux, the SEGGER tools are installed under the ``/opt/SEGGER``
        directory with versioned directories having the suffix ``_VERSION``.

        Args:
          cls (Library): the ``Library`` class

        Returns:
          The paths to the J-Link library files in the order that they are
          found.
        """
        dll = Library.JLINK_SDK_NAME
        root = os.path.join('/', 'opt', 'SEGGER')

        for (directory_name, subdirs, files) in os.walk(root):
            fnames = []
            x86_found = False
            for f in files:
                path = os.path.join(directory_name, f)
                if os.path.isfile(path) and f.startswith(dll):
                    fnames.append(f)
                    if '_x86' in path:
                        x86_found = True

            for fname in fnames:
                fpath = os.path.join(directory_name, fname)
                if util.is_os_64bit():
                    if not cls.can_load_library(fpath):
                        continue
                    elif '_x86' not in fname:
                        yield fpath
                elif x86_found:
                    if '_x86' in fname:
                        yield fpath
                else:
                    yield fpath

    @classmethod
    def find_library_darwin(cls):
        """Loads the SEGGER DLL from the installed applications.

        This method accounts for the all the different ways in which the DLL
        may be installed depending on the version of the DLL.  Always uses
        the first directory found.

        SEGGER's DLL is installed in one of three ways dependent on which
        which version of the SEGGER tools are installed:

        ======== ============================================================
        Versions Directory
        ======== ============================================================
        < 5.0.0  ``/Applications/SEGGER/JLink\\ NUMBER``
        < 6.0.0  ``/Applications/SEGGER/JLink/libjlinkarm.major.minor.dylib``
        >= 6.0.0 ``/Applications/SEGGER/JLink/libjlinkarm``
        ======== ============================================================

        Args:
          cls (Library): the ``Library`` class

        Returns:
          The path to the J-Link library files in the order they are found.
        """
        dll = Library.JLINK_SDK_NAME
        root = os.path.join('/', 'Applications', 'SEGGER')
        if not os.path.isdir(root):
            return

        for d in os.listdir(root):
            dir_path = os.path.join(root, d)

            # Navigate through each JLink directory.
            if os.path.isdir(dir_path) and d.startswith('JLink'):
                files = list(f for f in os.listdir(dir_path) if
                             os.path.isfile(os.path.join(dir_path, f)))

                # For versions >= 6.0.0 and < 5.0.0, this file will exist, so
                # we want to use this one instead of the versioned one.
                if (dll + '.dylib') in files:
                    yield os.path.join(dir_path, dll + '.dylib')

                # For versions >= 5.0.0 and < 6.0.0, there is no strictly
                # linked library file, so try and find the versioned one.
                for f in files:
                    if f.startswith(dll):
                        yield os.path.join(dir_path, f)

    def __init__(self, dllpath=None, use_tmpcpy=True):
        """Initializes an instance of a ``Library``.

        Loads the default J-Link DLL if ``dllpath`` is ``None``, otherwise
        loads the DLL specified by the given ``dllpath``.

        Args:
          self (Library): the ``Library`` instance
          dllpath (str): the DLL to load into the library
          use_tmpcpy (bool): True to load a temporary copy of J-Link DLL

        Returns:
          ``None``
        """
        self._lib = None
        self._winlib = None
        self._path = None
        self._windows = sys.platform.startswith('win')
        self._cygwin = sys.platform.startswith('cygwin')
        self._use_tmpcpy = use_tmpcpy
        self._temp = None

        if self._windows or self._cygwin:
            self._sdk = self.get_appropriate_windows_sdk_name()
        else:
            self._sdk = self.JLINK_SDK_OBJECT

        if dllpath is not None:
            self.load(dllpath)
        else:
            self.load_default()

    def __del__(self):
        """Cleans up the temporary DLL file created when the lib was loaded.

        Args:
          self (Library): the ``Library`` instance

        Returns:
          ``None``
        """
        self.unload()

    def load_default(self):
        """Loads the default J-Link SDK DLL.

        The default J-Link SDK is determined by first checking if ``ctypes``
        can find the DLL, then by searching the platform-specific paths.

        Args:
          self (Library): the ``Library`` instance

        Returns:
          ``True`` if the DLL was loaded, otherwise ``False``.
        """

        # Request the underlying operating system, through ctypes,
        # to resolve the J-Link DLL "the standard way" by its
        # library name.
        path = ctypes_util.find_library(self._sdk)

        # On Linux, find_library() actually returns the soname,
        # so we've got something like path = 'libjlinkarm.so.7',
        # and now have to retrieve the absolute file path.
        if (path is not None) and sys.platform.startswith('linux'):
            # For this, we'll rely on dlinfo(), which is not a POSIX API,
            # but a GNU libc extension.
            if platform.libc_ver()[0] == 'glibc':
                if Library._dlinfo is None:
                    Library._dlinfo = JLinkarmDlInfo(path)
                path = Library._dlinfo.path
            else:
                # When GNU libc extensions aren't available,
                # continue as if find_library() had failed.
                path = None

        if path is None:
            # Couldn't find it the standard way.  Fallback to the non-standard
            # way of finding the J-Link library.  These methods are operating
            # system specific.
            if self._windows or self._cygwin:
                path = next(self.find_library_windows(), None)
            elif sys.platform.startswith('linux'):
                path = next(self.find_library_linux(), None)
            elif sys.platform.startswith('darwin'):
                path = next(self.find_library_darwin(), None)

        if path is not None:
            return self.load(path)

        return False

    def load(self, path=None):
        """Loads the specified DLL, if any, otherwise re-loads the current DLL.

        If ``path`` is specified, loads the DLL at the given ``path``,
        otherwise re-loads the DLL currently specified by this library.

        Note:
          This creates a temporary DLL file to use for the instance.  This is
          necessary to work around a limitation of the J-Link DLL in which
          multiple J-Links cannot be accessed from the same process.

        Args:
          self (Library): the ``Library`` instance
          path (path): path to the DLL to load

        Returns:
          ``True`` if library was loaded successfully.

        Raises:
          OSError: if there is no J-LINK SDK DLL present at the path.

        See Also:
          `J-Link Multi-session <http://forum.segger.com/index.php?page=Thread&threadID=669>`_.
        """
        self.unload()

        self._path = path or self._path

        # Windows requires a proper suffix in order to load the library file,
        # so it must be set here.
        if self._windows or self._cygwin:
            suffix = '.dll'
        elif sys.platform.startswith('darwin'):
            suffix = '.dylib'
        else:
            suffix = '.so'

        lib_path = self._path

        if self._use_tmpcpy:
            # Copy the J-Link DLL to a temporary file.  This will be cleaned up the
            # next time we load a DLL using this library or if this library is
            # cleaned up.
            tf = tempfile.NamedTemporaryFile(delete=False, suffix=suffix)
            with open(tf.name, 'wb') as outputfile:
                with open(self._path, 'rb') as inputfile:
                    outputfile.write(inputfile.read())

            # This is needed to work around a WindowsError where the file is not
            # being properly cleaned up after exiting the with statement.
            tf.close()

            lib_path = tf.name
            self._temp = tf

        self._lib = ctypes.cdll.LoadLibrary(lib_path)

        if self._windows:
            # The J-Link library uses a mix of __cdecl and __stdcall function
            # calls.  While this is fine on a nix platform or in cygwin, this
            # causes issues with Windows, where it expects the __stdcall
            # methods to follow the standard calling convention.  As a result,
            # we have to convert them to windows function calls.
            self._winlib = ctypes.windll.LoadLibrary(lib_path)
            for stdcall in self._standard_calls_:
                if hasattr(self._winlib, stdcall):
                    # Backwards compatibility.  Some methods do not exist on
                    # older versions of the J-Link firmware, so ignore them in
                    # these cases.
                    setattr(self._lib, stdcall, getattr(self._winlib, stdcall))
        return True

    def unload(self):
        """Unloads the library's DLL if it has been loaded.

        This additionally cleans up the temporary DLL file that was created
        when the library was loaded.

        Args:
          self (Library): the ``Library`` instance

        Returns:
          ``True`` if the DLL was unloaded, otherwise ``False``.
        """
        unloaded = False

        if self._lib is not None:
            if self._winlib is not None:
                # ctypes passes integers as 32-bit C integer types, which will
                # truncate the value of a 64-bit pointer in 64-bit python, so
                # we have to change the FreeLibrary method to take a pointer
                # instead of an integer handle.
                ctypes.windll.kernel32.FreeLibrary.argtypes = (
                    ctypes.c_void_p,
                )

                # On Windows we must free both loaded libraries before the
                # temporary file can be cleaned up.
                ctypes.windll.kernel32.FreeLibrary(self._lib._handle)
                ctypes.windll.kernel32.FreeLibrary(self._winlib._handle)

                self._lib = None
                self._winlib = None

                unloaded = True
            else:
                # On OSX and Linux, just release the library; it's not safe
                # to close a dll that ctypes is using.
                del self._lib
                self._lib = None
                unloaded = True

        if self._temp is not None:
            os.remove(self._temp.name)
            self._temp = None

        return unloaded

    def dll(self):
        """Returns the DLL for the underlying shared library.

        Args:
          self (Library): the ``Library`` instance

        Returns:
          A ``ctypes`` DLL instance if one was loaded, otherwise ``None``.
        """
        return self._lib


class JLinkarmDlInfo:
    """Helper to retrieve the absolute path of the JLink library (aka DLL)
    based on its soname.

    This is used on Linux, where ctypes.util.find_library() will not return
    the library full file path, but only the file name (aka soname).
    We'll then rely on the native dlinfo() API to retrieve the library absolute
    file path.

    For e.g.:
    - LD_LIBRARY_PATH=/mnt/platform/segger/JLink
    - ctypes.util.find_library('jlinkarm') -> libjlinkarm.so.7
    - JLinkarmDlInfo('libjlinkarm.so.7').path -> /mnt/platform/segger/JLink/libjlinkarm.so.7

    Note that unlike dlopen(), dlsym() and friends, dlinfo() is not a POSIX API,
    but a GNU extension, available only on systems with the GNU libc implementation (glibc).

    The dlinfo() dance implementation is adapted from @cloudflightio,
    https://github.com/cloudflightio/python-dlinfo.
    """

    # Request to obtain a pointer to the link_map structure corresponding
    # to a given handle (Linux).
    # See: man dlinfo(3)
    RTLD_DI_LINKMAP = 2

    # dlinfo(3): struct link_map, where l_name will be the file path.
    class LinkMap(ctypes.Structure):
        """
        Represents a C struct link_map (Linux).
        See: man dlinfo(3)
        """
        _fields_ = [
            ('l_addr', ctypes.c_void_p),
            ('l_name', ctypes.c_char_p),
            ('l_ld', ctypes.c_void_p),
            ('l_next', ctypes.c_void_p),
            ('l_prev', ctypes.c_void_p),
        ]

    def __init__(self, jlinkarm_soname):
        """Retrieves the absolute file path of the JLink library (aka DLL)
        corresponding to the given soname.

        This runs the dlinfo() dance using the ctypes API:
        - loads the JLink DLL shared object for given soname
        - loads the dl library and lookup the dlinfo symbol
        - calls dlinfo() with request RTLD_DI_LINKMAP to get the struct link_map
          for the JLink library
        - access the struct content to retrieve the library's absolute path

        The dlinfo() dance implementation does not try to hide any OSError
        the ctypes API may raise, since this would likely hide a host
        system configuration issue (aka "should not happen").

        Args:
        - jlinkarm_soname: The JLink DLL soname returned by find_library(),
          for e.g. 'libjlinkarm.so.7'.

        Raises:
          - OSError when the actual library file has been removed, is not readable,
            is not a loadable shared object, etc.
        """
        # JLink DLL's absolute file path.
        self._dll_path = None

        # We're using the soname returned by a successful call to find_library(),
        # hence we can expect LoadLibrary() to in turn successfully load the JLink DLL.
        tmp_cdll_jlink = ctypes.cdll.LoadLibrary(jlinkarm_soname)

        # dlinfo() dance to retrieve the library's absolute file path.
        #
        # This code path should be involved only for POSIX systems
        # with GNU libc, where:
        # - libdl is available (POSIX)
        # - dlinfo() is defined (glibc)
        #
        # Hence bellow calls to find_library(), LoadLibrary(), and dlinfo() should succeed.
        dl_soname = ctypes_util.find_library('dl')
        if dl_soname is not None:
            tmp_cdll_dl = ctypes.cdll.LoadLibrary(dl_soname)
            dlinfo = tmp_cdll_dl.dlinfo
            dlinfo.argtypes = ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p
            dlinfo.restype = ctypes.c_int

            linkmap = ctypes.c_void_p()
            if dlinfo(tmp_cdll_jlink._handle, JLinkarmDlInfo.RTLD_DI_LINKMAP, ctypes.byref(linkmap)) == 0:
                linkmap = ctypes.cast(linkmap, ctypes.POINTER(JLinkarmDlInfo.LinkMap))
                self._dll_path = linkmap.contents.l_name.decode(sys.getdefaultencoding())

            # "Free" tmp dl library
            del tmp_cdll_dl
            tmp_cdll_dl = None

        else:
            # Should not happen.
            pass

        # "Free" tmp jlinkarm library
        del tmp_cdll_jlink
        tmp_cdll_jlink = None

    @property
    def path(self):
        """Answers the JLink DLL file path.

        Returns the JLink DLL's absolute path,
        or None when the dl library is unavailable despite the system
        presenting itself as POSIX.
        """
        return self._dll_path
