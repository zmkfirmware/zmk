# Copyright 2018 Square, Inc.
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

from . import binpacker
from . import decorators
from . import enums
from . import errors
from . import jlock
from . import library
from . import structs
from . import unlockers
from . import util

import ctypes
import datetime
import functools
import itertools
import logging
import math
import operator
import sys
import time
import six


logger = logging.getLogger(__name__)


class JLink(object):
    """Python interface for the SEGGER J-Link.

    This is a wrapper around the J-Link C SDK to provide a Python interface
    to it.  The shared library is loaded and used to call the SDK methods.
    """

    # J-Link V9 and J-Link ULTRA/PRO V4 have 336 bytes of memory for licenses,
    # so we base this number on that.  Other models have 80 bytes.
    MAX_BUF_SIZE = 336

    # Maximum number of CPU registers.
    MAX_NUM_CPU_REGISTERS = 256

    # Maximum speed (in kHz) that can be passed to `set_speed()`.
    MAX_JTAG_SPEED = 50000

    # Minimum speed (in kHz) that can be passed to `set_speed()`.
    MIN_JTAG_SPEED = 5

    # This speed cannot be passed to `set_speed()`.
    INVALID_JTAG_SPEED = 0xFFFE

    # Auto detection of JTAG speed.
    AUTO_JTAG_SPEED = 0x0

    # Adaptive clocking as JTAG speed.
    ADAPTIVE_JTAG_SPEED = 0xFFFF

    # Maximum number of methods of debug entry at a single time.
    MAX_NUM_MOES = 8

    def minimum_required(version):
        """Decorator to specify the minimum SDK version required.

        Args:
          version (str): valid version string

        Returns:
          A decorator function.
        """
        def _minimum_required(func):
            """Internal decorator that wraps around the given function.

            Args:
              func (function): function being decorated

            Returns:
              The wrapper unction.
            """
            @functools.wraps(func)
            def wrapper(self, *args, **kwargs):
                """Wrapper function to compare the DLL's SDK version.

                Args:
                  self (JLink): the ``JLink`` instance
                  args (list): list of arguments to pass to ``func``
                  kwargs (dict): key-word arguments dict to pass to ``func``

                Returns:
                  The return value of the wrapped function.

                Raises:
                  JLinkException: if the DLL's version is less than ``version``.
                """
                if list(self.version) < list(version):
                    raise errors.JLinkException('Version %s required.' % version)
                return func(self, *args, **kwargs)
            return wrapper
        return _minimum_required

    def open_required(func):
        """Decorator to specify that the J-Link DLL must be opened, and a
        J-Link connection must be established.

        Args:
          func (function): function being decorated

        Returns:
          The wrapper function.
        """
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            """Wrapper function to check that the given ``JLink`` has been
            opened.

            Args:
              self (JLink): the ``JLink`` instance
              args: list of arguments to pass to the wrapped function
              kwargs: key-word arguments dict to pass to the wrapped function

            Returns:
              The return value of the wrapped function.

            Raises:
              JLinkException: if the J-Link DLL is not open or the J-Link is
                  disconnected.
            """
            if not self.opened():
                raise errors.JLinkException('J-Link DLL is not open.')
            elif not self.connected():
                raise errors.JLinkException('J-Link connection has been lost.')
            return func(self, *args, **kwargs)
        return wrapper

    def connection_required(func):
        """Decorator to specify that a target connection is required in order
        for the given method to be used.

        Args:
          func (function): function being decorated

        Returns:
          The wrapper function.
        """
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            """Wrapper function to check that the given ``JLink`` has been
            connected to a target.

            Args:
              self (JLink): the ``JLink`` instance
              args: list of arguments to pass to the wrapped function
              kwargs: key-word arguments dict to pass to the wrapped function

            Returns:
              The return value of the wrapped function.

            Raises:
              JLinkException: if the JLink's target is not connected.
            """
            if not self.target_connected():
                raise errors.JLinkException('Target is not connected.')
            return func(self, *args, **kwargs)
        return wrapper

    def coresight_configuration_required(func):
        """Decorator to specify that a coresight configuration or target connection
        is required in order for the given method to be used.

        Args:
          func (function): function being decorated

        Returns:
          The wrapper function.
        """
        @functools.wraps(func)
        def wrapper(self, *args, **kwargs):
            """Wrapper function to check that the given ``JLink`` has been
            connected to a target or at least the coresight configuration has been done.

            Args:
              self (JLink): the ``JLink`` instance
              args: list of arguments to pass to the wrapped function
              kwargs: key-word arguments dict to pass to the wrapped function

            Returns:
              The return value of the wrapped function.

            Raises:
              JLinkException: if the JLink's target is not connected.
            """
            if not self.target_connected() and not self._coresight_configured:
                raise errors.JLinkException('Target is not connected neither coresight is not configured.')
            return func(self, *args, **kwargs)
        return wrapper

    def interface_required(interface):
        """Decorator to specify that a particular interface type is required
        for the given method to be used.

        Args:
          interface (int): attribute of ``JLinkInterfaces``

        Returns:
          A decorator function.
        """
        def _interface_required(func):
            """Internal decorator that wraps around the decorated function.

            Args:
              func (function): function being decorated

            Returns:
              The wrapper function.
            """
            @functools.wraps(func)
            def wrapper(self, *args, **kwargs):
                """Wrapper function to check that the given ``JLink`` has the
                same interface as the one specified by the decorator.

                Args:
                  self (JLink): the ``JLink`` instance
                  args: list of arguments to pass to ``func``
                  kwargs: key-word arguments dict to pass to ``func``

                Returns:
                  The return value of the wrapped function.

                Raises:
                  JLinkException: if the current interface is not supported by
                      the wrapped method.
                """
                if self.tif != interface:
                    raise errors.JLinkException('Unsupported for current interface.')
                return func(self, *args, **kwargs)
            return wrapper
        return _interface_required

    def __init__(self, lib=None, log=None, detailed_log=None, error=None, warn=None, unsecure_hook=None,
                 serial_no=None, ip_addr=None, open_tunnel=False, use_tmpcpy=True):
        """Initializes the J-Link interface object.

        Note:
          By default, the unsecure dialog will reject unsecuring the device on
          connection.  If you wish to change this behaviour (to have the device
          be unsecured and erased), pass a callback that returns
          ``JLinkFlags.DLG_BUTTON_YES`` as its return value.

        Args:
          self (JLink): the ``JLink`` instance
          lib (Library): a valid ``Library`` instance (not ``None`` dll)
          log (function): function to be called to write out log messages, by
            default this writes to standard out
          detailed_log (function): function to be called to write out detailed
            log messages, by default this writes to standard out
          error (function): function to be called to write out error messages,
            default this writes to standard error
          warn (function): function to be called to write out warning messages,
            default this his writes to standard error
          unsecure_hook (function): function to be called for the unsecure
            dialog
          serial_no (int): serial number of the J-Link
          ip_addr (str): IP address and port of the J-Link
            (e.g. 192.168.1.1:80)
          open_tunnel (bool, None): If ``False`` (default), the ``open``
            method will be called when entering the context manager using
            the ``serial_no`` and ``ip_addr`` provided here.
            If ``True`` ``open_tunnel`` method will be called instead
            of ``open`` method.
            If ``None``, the driver will not be opened automatically
            (however, it is still closed when exiting the context manager).
          use_tmpcpy (bool): True to load a temporary copy of J-Link DLL

        Returns:
          ``None``

        Raises:
          TypeError: if lib's DLL is ``None``
        """
        self._initialized = False

        if lib is None:
            lib = library.Library(use_tmpcpy=use_tmpcpy)

        if lib.dll() is None:
            raise TypeError('Expected to be given a valid DLL.')

        self._library = lib
        self._dll = lib.dll()
        self._tif = enums.JLinkInterfaces.JTAG
        self._unsecure_hook = unsecure_hook or util.unsecure_hook_dialog
        self._log_handler = None
        self._warning_handler = None
        self._error_handler = None
        self._detailed_log_handler = None
        self._swo_enabled = False
        self._lock = None
        self._device = None

        # Track the number of .open() calls to avoid multiple calls to
        # JLINKARM_Close, which can cause a crash.
        self._open_refcount = 0

        self._coresight_configured = False

        # Bind Types for function calls.
        self._dll.JLINKARM_OpenEx.restype = ctypes.POINTER(ctypes.c_char)
        self._dll.JLINKARM_GetCompileDateTime.restype = ctypes.POINTER(ctypes.c_char)
        self._dll.JLINKARM_GetRegisterName.restype = ctypes.POINTER(ctypes.c_char)

        self.error_handler = lambda s: (error or logger.error)(s.decode(errors='replace'))
        self.warning_handler = lambda s: (warn or logger.warning)(s.decode(errors='replace'))
        self.log_handler = lambda s: (log or logger.info)(s.decode(errors='replace'))
        self.detailed_log_handler = lambda s: (detailed_log or logger.debug)(s.decode(errors='replace'))

        # Parameters used for open() in context manager
        self.__serial_no = serial_no
        self.__ip_addr = ip_addr
        self.__open_tunnel = open_tunnel

        self._initialized = True

    def __del__(self):
        """Destructor for the ``JLink`` instance.  Closes the J-Link connection
        if one exists.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        # weakref callbacks are rather low level, and working out how to use
        # them correctly requires a bit of head scratching.  One must find
        # somewhere to store the weakref till after the referent is dead, and
        # without accidentally keeping the referent alive.  Then one must
        # ensure that the callback frees the weakref (without leaving any
        # remnant ref-cycles).
        #
        # When it is an option, using a __del__ method is far less hassle.
        #
        # Source: https://bugs.python.org/issue15528
        self._finalize()

    def __enter__(self):
        """Connects to the J-Link emulator (defaults to USB) using context manager.

        Parameters passed to __init__ are used for open() function.

        Returns:
          the ``JLink`` instance

        Raises:
          JLinkException: if fails to open (i.e. if device is unplugged)
          TypeError: if ``serial_no`` is present, but not ``int`` coercible.
          AttributeError: if ``serial_no`` and ``ip_addr`` are both ``None``.
        """
        if self.__open_tunnel is False:
            self.open(serial_no=self.__serial_no, ip_addr=self.__ip_addr)
        elif self.__open_tunnel is True:
            self.open_tunnel(serial_no=self.__serial_no)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Closes the JLink connection on exit of the context manager.

        Stops the SWO if enabled and closes the J-Link connection if one
        exists.

        Args:
          self (JLink): the ``JLink`` instance
          exc_type (BaseExceptionType, None): the exception class, if any
            raised inside the context manager
          exc_val (BaseException, None): the exception object, if any raised
            inside the context manager
          exc_tb (TracebackType, None): the exception traceback, if any
            exception was raised inside the context manager.

        Returns:
          ``True`` if exception raised inside the context manager was handled
            and shall be suppressed (not propagated), ``None`` otherwise.
        """
        self._finalize()
        # Do not return anything to pass on all other exceptions.

    def _finalize(self):
        """Finalizer ("destructor") for the ``JLink`` instance.

        Stops the SWO if enabled and closes the J-Link connection if one
        exists.
        Called when exiting the context manager or when this object is
        destructed (garbage collected).

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        if self._initialized:
            if self.connected():
                if self.swo_enabled():
                    self.swo_stop()

            if self.opened():
                self.close()

    def _get_register_index_from_name(self, register):
        """
        Converts a register name to a register index

        Args:
            self (JLink): the ``JLink`` instance
            register (str): the register name

        Returns:
          ``int``
        """
        regs = list(self.register_name(idx) for idx in self.register_list())
        if isinstance(register, six.string_types):
            try:
                result = regs.index(register)
            except ValueError:
                error_message = "No register found matching name: {}. (available registers: {})"
                raise errors.JLinkException(error_message.format(register, ', '.join(regs)))
        return result

    def opened(self):
        """Returns whether the DLL is open.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if the J-Link is open, otherwise ``False``.
        """
        return bool(self._dll.JLINKARM_IsOpen())

    def connected(self):
        """Returns whether a J-Link is connected.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if the J-Link is open and connected, otherwise ``False``.
        """
        return self.opened() and bool(self._dll.JLINKARM_EMU_IsConnected())

    def target_connected(self):
        """Returns whether a target is connected to the J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if a target is connected, otherwise ``False``.
        """
        return self.connected() and bool(self._dll.JLINKARM_IsConnected())

    @property
    def log_handler(self):
        """Returns the log handler function.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None`` if the log handler was not set, otherwise a
          ``ctypes.CFUNCTYPE``.
        """
        return self._log_handler

    @log_handler.setter
    def log_handler(self, handler):
        """Setter for the log handler function.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        if not self.opened():
            handler = handler or util.noop
            self._log_handler = enums.JLinkFunctions.LOG_PROTOTYPE(handler)
            self._dll.JLINKARM_EnableLog(self._log_handler)

    @property
    def detailed_log_handler(self):
        """Returns the detailed log handler function.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None`` if the detailed log handler was not set, otherwise a
          ``ctypes.CFUNCTYPE``.
        """
        return self._detailed_log_handler

    @detailed_log_handler.setter
    def detailed_log_handler(self, handler):
        """Setter for the detailed log handler function.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        if not self.opened():
            handler = handler or util.noop
            self._detailed_log_handler = enums.JLinkFunctions.LOG_PROTOTYPE(handler)
            self._dll.JLINKARM_EnableLogCom(self._detailed_log_handler)

    @property
    def error_handler(self):
        """Returns the error handler function.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None`` if the error handler was not set, otherwise a
          ``ctypes.CFUNCTYPE``.
        """
        return self._error_handler

    @error_handler.setter
    def error_handler(self, handler):
        """Setter for the error handler function.

        If the DLL is open, this function is a no-op, so it should be called
        prior to calling ``open()``.

        Args:
          self (JLink): the ``JLink`` instance
          handler (function): function to call on error messages

        Returns:
          ``None``
        """
        if not self.opened():
            handler = handler or util.noop
            self._error_handler = enums.JLinkFunctions.LOG_PROTOTYPE(handler)
            self._dll.JLINKARM_SetErrorOutHandler(self._error_handler)

    @property
    def warning_handler(self):
        """Returns the warning handler function.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None`` if the warning handler was not set, otherwise a
          ``ctypes.CFUNCTYPE``.
        """
        return self._warning_handler

    @warning_handler.setter
    def warning_handler(self, handler):
        """Setter for the warning handler function.

        If the DLL is open, this function is a no-op, so it should be called
        prior to calling ``open()``.

        Args:
          self (JLink): the ``JLink`` instance
          handler (function): function to call on warning messages

        Returns:
          ``None``
        """
        if not self.opened():
            handler = handler or util.noop
            self._warning_handler = enums.JLinkFunctions.LOG_PROTOTYPE(handler)
            self._dll.JLINKARM_SetWarnOutHandler(self._warning_handler)

    def num_connected_emulators(self):
        """Returns the number of emulators which are connected via USB to the
        host.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The number of connected emulators.
        """
        return self._dll.JLINKARM_EMU_GetNumDevices()

    def connected_emulators(self, host=enums.JLinkHost.USB):
        """Returns a list of all the connected emulators.

        Args:
          self (JLink): the ``JLink`` instance
          host (int): host type to search (default: ``JLinkHost.USB``)

        Returns:
          List of ``JLinkConnectInfo`` specifying the connected emulators.

        Raises:
          JLinkException: if fails to enumerate devices.
        """
        res = self._dll.JLINKARM_EMU_GetList(host, 0, 0)
        if res < 0:
            raise errors.JLinkException(res)

        num_devices = res
        info = (structs.JLinkConnectInfo * num_devices)()
        num_found = self._dll.JLINKARM_EMU_GetList(host, info, num_devices)
        if num_found < 0:
            raise errors.JLinkException(num_found)

        return list(info)[:num_found]

    def get_device_index(self, chip_name):
        """Finds index of device with chip name

        Args:
          self (JLink): the ``JLink`` instance
          chip_name (str): target chip name

        Returns:
          Index of the device with the matching chip name.

        Raises:
          ``JLinkException``: if chip is unsupported.
        """
        index = self._dll.JLINKARM_DEVICE_GetIndex(chip_name.encode('ascii'))

        if index <= 0:
            raise errors.JLinkException('Unsupported device selected.')

        return index

    def num_supported_devices(self):
        """Returns the number of devices that are supported by the opened
        J-Link DLL.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Number of devices the J-Link DLL supports.
        """
        return int(self._dll.JLINKARM_DEVICE_GetInfo(-1, 0))

    def supported_device(self, index=0):
        """Gets the device at the given ``index``.

        Args:
          self (JLink): the ``JLink`` instance
          index (int): the index of the device whose information to get

        Returns:
          A ``JLinkDeviceInfo`` describing the requested device.

        Raises:
          ValueError: if index is less than 0 or >= supported device count.
        """
        if not util.is_natural(index) or index >= self.num_supported_devices():
            raise ValueError('Invalid index.')

        info = structs.JLinkDeviceInfo()

        result = self._dll.JLINKARM_DEVICE_GetInfo(index, ctypes.byref(info))
        return info

    def open(self, serial_no=None, ip_addr=None):
        """Connects to the J-Link emulator (defaults to USB).

        If ``serial_no`` and ``ip_addr`` are both given, this function will
        connect to the J-Link over TCP/IP.

        Args:
          self (JLink): the ``JLink`` instance
          serial_no (int): serial number of the J-Link
          ip_addr (str): IP address and port of the J-Link (e.g. 192.168.1.1:80)

        Returns:
          ``None``

        Raises:
          JLinkException: if fails to open (i.e. if device is unplugged)
          TypeError: if ``serial_no`` is present, but not ``int`` coercible.
          AttributeError: if ``serial_no`` and ``ip_addr`` are both ``None``.
        """
        if self._open_refcount > 0:
            self._open_refcount += 1
            return None

        # For some reason, the J-Link driver complains if this isn't called
        # first (may have something to do with it trying to establish a
        # connection).  Without this call, it will log an error stating
        #        NET_WriteRead(): USB communication not locked
        #        PID0017A8C (): Lock count error (decrement)
        self.close()

        if ip_addr is not None:
            addr, port = ip_addr.rsplit(':', 1)
            if serial_no is None:
                result = self._dll.JLINKARM_SelectIP(addr.encode(), int(port))
                if result == 1:
                    raise errors.JLinkException('Could not connect to emulator at %s.' % ip_addr)
            else:
                # Note: No return code when selecting IP by serial number.
                self._dll.JLINKARM_EMU_SelectIPBySN(int(serial_no))

        elif serial_no is not None:
            result = self._dll.JLINKARM_EMU_SelectByUSBSN(int(serial_no))
            if result < 0:
                raise errors.JLinkException('No emulator with serial number %s found.' % serial_no)

        else:
            # The original method of connecting to USB0-3 via
            # JLINKARM_SelectUSB has been obsolesced, however its use is
            # preserved here to simplify workflows using one emulator:
            result = self._dll.JLINKARM_SelectUSB(0)
            if result != 0:
                raise errors.JlinkException('Could not connect to default emulator.')

        # Acquire the lock for the J-Link being opened only if the serial
        # number was passed in, otherwise skip it here.  Note that the lock
        # must be acquired before calling 'JLINKARM_OpenEx()', otherwise the
        # call will fail on Windows.
        if serial_no is not None:
            self._lock = jlock.JLock(serial_no)
            if not self._lock.acquire():
                raise errors.JLinkException('J-Link is already open.')

        result = self._dll.JLINKARM_OpenEx(self.log_handler, self.error_handler)
        result = ctypes.cast(result, ctypes.c_char_p).value
        if result is not None:
            raise errors.JLinkException(result.decode())

        # Configuration of the J-Link DLL.  These are configuration steps that
        # have to be done after 'open()'.  The unsecure hook is only supported
        # on versions greater than V4.98a.
        unsecure_hook = self._unsecure_hook
        if unsecure_hook is not None and hasattr(self._dll, 'JLINK_SetHookUnsecureDialog'):
            self.unsecure_hook = enums.JLinkFunctions.UNSECURE_HOOK_PROTOTYPE(unsecure_hook)
            self._dll.JLINK_SetHookUnsecureDialog(self.unsecure_hook)

        self._open_refcount = 1
        return None

    def open_tunnel(self, serial_no, port=19020):
        """Connects to the J-Link emulator (over SEGGER tunnel).

        Args:
          self (JLink): the ``JLink`` instance
          serial_no (int): serial number of the J-Link
          port (int): optional port number (default to 19020).

        Returns:
          ``None``
        """
        return self.open(ip_addr='tunnel:' + str(serial_no) + ':' + str(port))

    def close(self):
        """Closes the open J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``

        Raises:
          JLinkException: if there is no connected JLink.
        """
        if self._open_refcount == 0:
            # Do nothing if .open() has not been called.
            return None

        self._open_refcount -= 1
        if self._open_refcount > 0:
            return None

        self._coresight_configured = False

        self._dll.JLINKARM_Close()

        if self._lock is not None:
            del self._lock
            self._lock = None

        return None

    def test(self):
        """Performs a self test.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if test passed, otherwise ``False``.
        """
        res = self._dll.JLINKARM_Test()
        return (res == 0)

    @open_required
    def set_log_file(self, file_path):
        """Sets the log file output path.
        see https://wiki.segger.com/Enable_J-Link_log_file

        Args:
          self (JLink): the ``JLink`` instance
          file_path (str): the file path where the log file will be stored

        Returns:
          ``None``

        Raises:
          JLinkException: if the path specified is invalid.
        """
        res = self._dll.JLINKARM_SetLogFile(file_path.encode())
        if res:
            raise errors.JLinkException(res)

    @open_required
    def set_script_file(self, file_path):
        """Sets the custom `Script File`_ to use.

        Args:
          self (JLink): the ``JLink`` instance
          file_path (str): file path to the JLink script file to be used

        Returns:
          ``None``

        Raises:
          JLinkException: if the path specified is invalid.

        .. _Script File:
          https://wiki.segger.com/J-Link_script_files
        """
        res = self.exec_command('scriptfile = %s' % file_path)
        if res:
            raise errors.JLinkException('Failed to set JLink Script File: %r' % file_path)

    @open_required
    def invalidate_firmware(self):
        """Invalidates the emulator's firmware.

        This method is useful for downgrading the firmware on an emulator.  By
        calling this method, the current emulator's firmware is invalidated,
        which will make the emulator download the firmware of the J-Link SDK
        DLL that this instance was created with.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``

        Raises:
          JLinkException: on hardware error.
        """
        self.exec_command('InvalidateFW')
        return None

    @open_required
    def update_firmware(self):
        """Performs a firmware update.

        If there is a newer version of firmware available for the J-Link
        device, then updates the firmware.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Checksum of the new firmware on update, ``0`` if the firmware was not
          changed.
        """
        return self._dll.JLINKARM_UpdateFirmwareIfNewer()

    @open_required
    def sync_firmware(self):
        """Syncs the emulator's firmware version and the DLL's firmware.

        This method is useful for ensuring that the firmware running on the
        J-Link matches the firmware supported by the DLL.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        serial_no = self.serial_number

        if self.firmware_newer():
            # The J-Link's firmware is newer than the one compatible with the
            # DLL (though there are promises of backwards compatibility), so
            # perform a downgrade.
            try:
                # This may throw an exception on older versions of the J-Link
                # software due to the software timing out after a firmware
                # upgrade.
                self.invalidate_firmware()
                self.update_firmware()
            except errors.JLinkException as e:
                pass

            res = self.open(serial_no=serial_no)

            if self.firmware_newer():
                raise errors.JLinkException('Failed to sync firmware version.')

            return res

        elif self.firmware_outdated():
            # The J-Link's firmware is older than the one compatible with the
            # DLL, so perform a firmware upgrade.
            try:
                # This may throw an exception on older versions of the J-Link
                # software due to the software timing out after a firmware
                # upgrade.
                self.update_firmware()
            except errors.JLinkException as e:
                pass

            if self.firmware_outdated():
                raise errors.JLinkException('Failed to sync firmware version.')

            return self.open(serial_no=serial_no)

        return None

    def exec_command(self, cmd):
        """Executes the given command.

        This method executes a command by calling the DLL's exec method.
        Direct API methods should be prioritized over calling this method.

        Args:
          self (JLink): the ``JLink`` instance
          cmd (str): the command to run

        Returns:
          The return code of running the command.

        Raises:
          JLinkException: if the command is invalid or fails.

        See Also:
          For a full list of the supported commands, please see the SEGGER
          J-Link documentation,
          `UM08001 <https://www.segger.com/downloads/jlink>`__.
        """
        err_buf = (ctypes.c_char * self.MAX_BUF_SIZE)()
        res = self._dll.JLINKARM_ExecCommand(cmd.encode(), err_buf, self.MAX_BUF_SIZE)
        err_buf = ctypes.string_at(err_buf).decode()

        if len(err_buf) > 0:
            # This is how they check for error in the documentation, so check
            # this way as well.
            raise errors.JLinkException(err_buf.strip())

        return res

    @minimum_required('5.02')
    def enable_dialog_boxes(self):
        """Enables showing dialog boxes on certain methods.

        Note:
          This can be used for batch or automized test running.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self.exec_command('SetBatchMode = 0')
        self.exec_command("HideDeviceSelection = 0")
        self.exec_command("EnableInfoWinFlashDL")
        self.exec_command("EnableInfoWinFlashBPs")

    @minimum_required('5.02')
    def disable_dialog_boxes(self):
        """Disables showing dialog boxes on certain methods.

        Warning:
          This has the effect of also silencing dialog boxes that appear when
          updating firmware / to confirm updating firmware.

        Dialog boxes will be shown for a brief period of time (approximately
        five seconds), before being automatically hidden, and the default
        option chosen.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self.exec_command('SilentUpdateFW')
        self.exec_command('SuppressInfoUpdateFW')
        self.exec_command('SetBatchMode = 1')

        # SuppressControlPanel
        self.exec_command("HideDeviceSelection = 1")
        self.exec_command("SuppressControlPanel")
        # Hide Flash Windows
        self.exec_command("DisableInfoWinFlashDL")
        self.exec_command("DisableInfoWinFlashBPs")

    @open_required
    def jtag_configure(self, instr_regs=0, data_bits=0):
        """Configures the JTAG scan chain to determine which CPU to address.

        Must be called if the J-Link is connected to a JTAG scan chain with
        multiple devices.

        Args:
          self (JLink): the ``JLink`` instance
          instr_regs (int): length of instruction registers of all devices
            closer to TD1 then the addressed CPU
          data_bits (int): total number of data bits closer to TD1 than the
            addressed CPU

        Returns:
          ``None``

        Raises:
          ValueError: if ``instr_regs`` or ``data_bits`` are not natural numbers
        """
        if not util.is_natural(instr_regs):
            raise ValueError('IR value is not a natural number.')

        if not util.is_natural(data_bits):
            raise ValueError('Data bits is not a natural number.')

        self._dll.JLINKARM_ConfigJTAG(instr_regs, data_bits)
        return None

    @open_required
    @minimum_required('4.98e')
    def coresight_configure(self,
                            ir_pre=0,
                            dr_pre=0,
                            ir_post=0,
                            dr_post=0,
                            ir_len=0,
                            perform_tif_init=True):
        """Prepares target and J-Link for CoreSight function usage.

        Args:
          self (JLink): the ``JLink`` instance
          ir_pre (int): sum of instruction register length of all JTAG devices
            in the JTAG chain, close to TDO than the actual one, that J-Link
            shall communicate with
          dr_pre (int): number of JTAG devices in the JTAG chain, closer to TDO
            than the actual one, that J-Link shall communicate with
          ir_post (int): sum of instruction register length of all JTAG devices
            in the JTAG chain, following the actual one, that J-Link shall
            communicate with
          dr_post (int): Number of JTAG devices in the JTAG chain, following
            the actual one, J-Link shall communicate with
          ir_len (int): instruction register length of the actual device that
            J-Link shall communicate with
          perform_tif_init (bool): if ``False``, then do not output switching
            sequence on completion

        Returns:
          ``None``

        Note:
          This must be called before calling ``coresight_read()`` or
          ``coresight_write()``.
        """
        if self.tif == enums.JLinkInterfaces.SWD:
            # No special setup is needed for SWD, just need to output the
            # switching sequence.
            res = self._dll.JLINKARM_CORESIGHT_Configure('')
            if res < 0:
                raise errors.JLinkException(res)

            self._coresight_configured = True

            return None

        # JTAG requires more setup than SWD.
        config_string = 'IRPre=%s;DRPre=%s;IRPost=%s;DRPost=%s;IRLenDevice=%s;'
        config_string = config_string % (ir_pre, dr_pre, ir_post, dr_post, ir_len)

        if not perform_tif_init:
            config_string = config_string + ('PerformTIFInit=0;')

        res = self._dll.JLINKARM_CORESIGHT_Configure(config_string.encode())
        if res < 0:
            raise errors.JLinkException(res)

        self._coresight_configured = True

        return None

    @open_required
    def connect(self, chip_name, speed='auto', verbose=False):
        """Connects the J-Link to its target.

        Args:
          self (JLink): the ``JLink`` instance
          chip_name (str): target chip name
          speed (int): connection speed, one of ``{5-12000, 'auto', 'adaptive'}``
          verbose (bool): boolean indicating if connection should be verbose in logging

        Returns:
          ``None``

        Raises:
          JLinkException: if connection fails to establish.
          TypeError: if given speed is invalid
        """

        if verbose:
            self.exec_command('EnableRemarks = 1')

        # Determine which device we are.  This is essential for using methods
        # like 'unlock' or 'lock'.
        index = self.get_device_index(chip_name)

        self._device = self.supported_device(index)

        # This is weird but is currently the only way to specify what the
        # target is to the J-Link.
        self.exec_command('Device = %s' % chip_name)

        # Need to select target interface speed here, so the J-Link knows what
        # speed to use to establish target communication.
        if speed == 'auto':
            self.set_speed(auto=True)
        elif speed == 'adaptive':
            self.set_speed(adaptive=True)
        else:
            self.set_speed(speed)

        # When we specify 'Device =', we will trigger an auto-connect to the
        # target under debugging. If the 'exec_command' failed, then we want
        # to force the connect here.
        if not self.target_connected():
            result = self._dll.JLINKARM_Connect()
            if result < 0:
                raise errors.JLinkException(result)

        try:
            # Issue a no-op command after connect. This has to be in a try-catch.
            self.halted()
        except errors.JLinkException:
            pass

        return None

    @property
    def error(self):
        """DLL internal error state.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The DLL internal error state.  This is set if any error occurs in
          underlying DLL, otherwise it is ``None``.
        """
        error = int(self._dll.JLINKARM_HasError())
        if error == 0:
            return None
        return error

    def clear_error(self):
        """Clears the DLL internal error state.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The error state before the clear.
        """
        error = self.error
        self._dll.JLINKARM_ClrError()
        return error

    @property
    def compile_date(self):
        """Returns a string specifying the date and time at which the DLL was
        translated.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Datetime string.
        """
        result = self._dll.JLINKARM_GetCompileDateTime()
        return ctypes.cast(result, ctypes.c_char_p).value.decode()

    @property
    def version(self):
        """Returns the device's version.

        The device's version is returned as a string of the format: M.mr where
        ``M`` is major number, ``m`` is minor number, and ``r`` is revision
        character.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Device version string.
        """
        version = int(self._dll.JLINKARM_GetDLLVersion())
        major = version / 10000
        minor = (version / 100) % 100
        rev = version % 100
        rev = '' if rev == 0 else chr(rev + ord('a') - 1)
        return '%d.%02d%s' % (major, minor, rev)

    @property
    @open_required
    def compatible_firmware_version(self):
        """Returns the DLL's compatible J-Link firmware version.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The firmware version of the J-Link that the DLL is compatible
          with.

        Raises:
          JLinkException: on error.
        """
        identifier = self.firmware_version.split('compiled')[0]
        buf_size = self.MAX_BUF_SIZE
        buf = (ctypes.c_char * buf_size)()
        res = self._dll.JLINKARM_GetEmbeddedFWString(identifier.encode(), buf, buf_size)
        if res < 0:
            raise errors.JLinkException(res)

        return ctypes.string_at(buf).decode()

    @open_required
    def firmware_outdated(self):
        """Returns whether the J-Link's firmware version is older than the one
        that the DLL is compatible with.

        Note:
          This is not the same as calling ``not jlink.firmware_newer()``.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if the J-Link's firmware is older than the one supported by
          the DLL, otherwise ``False``.
        """
        datefmt = ' %b %d %Y %H:%M:%S'

        compat_date = self.compatible_firmware_version.split('compiled')[1]
        compat_date = datetime.datetime.strptime(compat_date, datefmt)

        fw_date = self.firmware_version.split('compiled')[1]
        fw_date = datetime.datetime.strptime(fw_date, datefmt)
        return (compat_date > fw_date)

    @open_required
    def firmware_newer(self):
        """Returns whether the J-Link's firmware version is newer than the one
        that the DLL is compatible with.

        Note:
          This is not the same as calling ``not jlink.firmware_outdated()``.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if the J-Link's firmware is newer than the one supported by
          the DLL, otherwise ``False``.
        """
        if self.firmware_outdated():
            return False
        return self.firmware_version != self.compatible_firmware_version

    @property
    @open_required
    def hardware_info(self, mask=0xFFFFFFFF):
        """Returns a list of 32 integer values corresponding to the bitfields
        specifying the power consumption of the target.

        The values returned by this function only have significance if the
        J-Link is powering the target.

        The words, indexed, have the following significance:
          0. If ``1``, target is powered via J-Link.
          1. Overcurrent bitfield:
             0: No overcurrent.
             1: Overcurrent happened.  2ms @ 3000mA
             2: Overcurrent happened.  10ms @ 1000mA
             3: Overcurrent happened.  40ms @ 400mA
          2. Power consumption of target (mA).
          3. Peak of target power consumption (mA).
          4. Peak of target power consumption during J-Link operation (mA).

        Args:
          self (JLink): the ``JLink`` instance
          mask (int): bit mask to decide which hardware information words are
            returned (defaults to all the words).

        Returns:
          List of bitfields specifying different states based on their index
          within the list and their value.

        Raises:
          JLinkException: on hardware error.
        """
        buf = (ctypes.c_uint32 * 32)()
        res = self._dll.JLINKARM_GetHWInfo(mask, ctypes.byref(buf))
        if res != 0:
            raise errors.JLinkException(res)
        return list(buf)

    @property
    @open_required
    def hardware_status(self):
        """Retrieves and returns the hardware status.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          A ``JLinkHardwareStatus`` describing the J-Link hardware.
        """
        stat = structs.JLinkHardwareStatus()
        res = self._dll.JLINKARM_GetHWStatus(ctypes.byref(stat))
        if res == 1:
            raise errors.JLinkException('Error in reading hardware status.')
        return stat

    @property
    @open_required
    def hardware_version(self):
        """Returns the hardware version of the connected J-Link as a
        major.minor string.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Hardware version string.
        """
        version = self._dll.JLINKARM_GetHardwareVersion()
        major = version / 10000 % 100
        minor = version / 100 % 100
        return '%d.%02d' % (major, minor)

    @property
    @open_required
    def firmware_version(self):
        """Returns a firmware identification string of the connected J-Link.

        It consists of the following:
          - Product Name (e.g. J-Link)
          - The string: compiled
          - Compile data and time.
          - Optional additional information.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Firmware identification string.
        """
        buf = (ctypes.c_char * self.MAX_BUF_SIZE)()
        self._dll.JLINKARM_GetFirmwareString(buf, self.MAX_BUF_SIZE)
        return ctypes.string_at(buf).decode()

    @property
    @open_required
    def capabilities(self):
        """Returns a bitwise combination of the emulator's capabilities.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Bitfield of emulator capabilities.
        """
        return self._dll.JLINKARM_GetEmuCaps()

    @property
    @open_required
    def extended_capabilities(self):
        """Gets the capabilities of the connected emulator as a list.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          List of 32 integers which define the extended capabilities based on
          their value and index within the list.
        """
        buf = (ctypes.c_uint8 * 32)()
        self._dll.JLINKARM_GetEmuCapsEx(buf, 32)
        return list(buf)

    @open_required
    def extended_capability(self, capability):
        """Checks if the emulator has the given extended capability.

        Args:
          self (JLink): the ``JLink`` instance
          capability (int): capability being queried

        Returns:
          ``True`` if the emulator has the given extended capability, otherwise
          ``False``.
        """
        res = self._dll.JLINKARM_EMU_HasCapEx(capability)
        return (res == 1)

    @property
    @open_required
    def features(self):
        """Returns a list of the J-Link embedded features.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          A list of strings, each a feature.  Example:
          ``[ 'RDI', 'FlashBP', 'FlashDL', 'JFlash', 'GDB' ]``
        """
        buf = (ctypes.c_char * self.MAX_BUF_SIZE)()
        self._dll.JLINKARM_GetFeatureString(buf)

        result = ctypes.string_at(buf).decode().strip()
        if len(result) == 0:
            return list()

        return result.split(', ')

    @property
    @open_required
    def product_name(self):
        """Returns the product name of the connected J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Product name.
        """
        buf = (ctypes.c_char * self.MAX_BUF_SIZE)()
        self._dll.JLINKARM_EMU_GetProductName(buf, self.MAX_BUF_SIZE)
        return ctypes.string_at(buf).decode()

    @property
    @open_required
    def serial_number(self):
        """Returns the serial number of the connected J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Serial number as an integer.
        """
        return self._dll.JLINKARM_GetSN()

    @property
    @open_required
    def oem(self):
        """Retrieves and returns the OEM string of the connected J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The string of the OEM.  If this is an original SEGGER product, then
          ``None`` is returned instead.

        Raises:
          JLinkException: on hardware error.
        """
        buf = (ctypes.c_char * self.MAX_BUF_SIZE)()
        res = self._dll.JLINKARM_GetOEMString(ctypes.byref(buf))
        if res != 0:
            raise errors.JLinkException('Failed to grab OEM string.')

        oem = ctypes.string_at(buf).decode()
        if len(oem) == 0:
            # In the case that the product is an original SEGGER product, then
            # the OEM string is the empty string, so there is no OEM.
            return None

        return oem

    @property
    @open_required
    def index(self):
        """Retrieves and returns the index number of the actual selected
        J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Index of the currently connected J-Link.
        """
        return self._dll.JLINKARM_GetSelDevice()

    @property
    @open_required
    def speed(self):
        """Returns the current JTAG connection speed.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          JTAG connection speed.
        """
        return self._dll.JLINKARM_GetSpeed()

    @open_required
    def set_speed(self, speed=None, auto=False, adaptive=False):
        """Sets the speed of the JTAG communication with the ARM core.

        If no arguments are present, automatically detects speed.

        If a ``speed`` is provided, the speed must be no larger than
        ``JLink.MAX_JTAG_SPEED`` and no smaller than ``JLink.MIN_JTAG_SPEED``.
        The given ``speed`` can also not be ``JLink.INVALID_JTAG_SPEED``.

        Args:
          self (JLink): the ``JLink`` instance
          speed (int): the speed in kHz to set the communication at
          auto (bool): automatically detect correct speed
          adaptive (bool): select adaptive clocking as JTAG speed

        Returns:
          ``None``

        Raises:
          TypeError: if given speed is not a natural number.
          ValueError: if given speed is too high, too low, or invalid.
        """
        if speed is None:
            speed = 0
        elif not util.is_natural(speed):
            raise TypeError('Expected positive number for speed, given %s.' % speed)
        elif speed > self.MAX_JTAG_SPEED:
            raise ValueError('Given speed exceeds max speed of %d.' % self.MAX_JTAG_SPEED)
        elif speed < self.MIN_JTAG_SPEED:
            raise ValueError('Given speed is too slow.  Minimum is %d.' % self.MIN_JTAG_SPEED)

        if auto:
            speed = speed | self.AUTO_JTAG_SPEED

        if adaptive:
            speed = speed | self.ADAPTIVE_JTAG_SPEED

        self._dll.JLINKARM_SetSpeed(speed)

        return None

    @open_required
    def set_max_speed(self):
        """Sets JTAG communication speed to the maximum supported speed.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_SetMaxSpeed()
        return None

    @property
    @open_required
    def speed_info(self):
        """Retrieves information about supported target interface speeds.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The ``JLinkSpeedInfo`` instance describing the supported target
          interface speeds.
        """
        speed_info = structs.JLinkSpeedInfo()
        self._dll.JLINKARM_GetSpeedInfo(ctypes.byref(speed_info))
        return speed_info

    @property
    @open_required
    def licenses(self):
        """Returns a string of the built-in licenses the J-Link has.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          String of the contents of the built-in licenses the J-Link has.
        """
        buf_size = self.MAX_BUF_SIZE
        buf = (ctypes.c_char * buf_size)()
        res = self._dll.JLINK_GetAvailableLicense(buf, buf_size)
        if res < 0:
            raise errors.JLinkException(res)
        return ctypes.string_at(buf).decode()

    @property
    @open_required
    @minimum_required('4.98b')
    def custom_licenses(self):
        """Returns a string of the installed licenses the J-Link has.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          String of the contents of the custom licenses the J-Link has.
        """
        buf = (ctypes.c_char * self.MAX_BUF_SIZE)()
        result = self._dll.JLINK_EMU_GetLicenses(buf, self.MAX_BUF_SIZE)
        if result < 0:
            raise errors.JLinkException(result)
        return ctypes.string_at(buf).decode()

    @open_required
    @minimum_required('4.98b')
    def add_license(self, contents):
        """Adds the given ``contents`` as a new custom license to the J-Link.

        Args:
          self (JLink): the ``JLink`` instance
          contents: the string contents of the new custom license

        Returns:
          ``True`` if license was added, ``False`` if license already existed.

        Raises:
          JLinkException: if the write fails.

        Note:
          J-Link V9 and J-Link ULTRA/PRO V4 have 336 Bytes of memory for
          licenses, while older versions of 80 bytes.
        """
        buf_size = len(contents)
        buf = (ctypes.c_char * (buf_size + 1))(*contents.encode())

        res = self._dll.JLINK_EMU_AddLicense(buf)

        if res == -1:
            raise errors.JLinkException('Unspecified error.')
        elif res == -2:
            raise errors.JLinkException('Failed to read/write license area.')
        elif res == -3:
            raise errors.JLinkException('J-Link out of space.')

        return (res == 0)

    @open_required
    @minimum_required('4.98b')
    def erase_licenses(self):
        """Erases the custom licenses from the connected J-Link.

        Note:
          This method will erase all licenses stored on the J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` on success, otherwise ``False``.
        """
        res = self._dll.JLINK_EMU_EraseLicenses()
        return (res == 0)

    @property
    @open_required
    def tif(self):
        """Returns the current target interface of the J-Link.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Integer specifying the current target interface.
        """
        return self._tif

    @open_required
    def supported_tifs(self):
        """Returns a bitmask of the supported target interfaces.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Bitfield specifying which target interfaces are supported.
        """
        buf = ctypes.c_uint32()
        self._dll.JLINKARM_TIF_GetAvailable(ctypes.byref(buf))
        return buf.value

    @open_required
    def set_tif(self, interface):
        """Selects the specified target interface.

        Note that a restart must be triggered for this to take effect.

        Args:
          self (Jlink): the ``JLink`` instance
          interface (int): integer identifier of the interface

        Returns:
          ``True`` if target was updated, otherwise ``False``.

        Raises:
          JLinkException: if the given interface is invalid or unsupported.
        """
        if not ((1 << interface) & self.supported_tifs()):
            raise errors.JLinkException('Unsupported target interface: %s' % interface)

        # The return code here is actually *NOT* the previous set interface, it
        # is ``0`` on success, otherwise ``1``.
        res = self._dll.JLINKARM_TIF_Select(interface)
        if res != 0:
            return False

        self._tif = interface
        return True

    @open_required
    def gpio_properties(self):
        """Returns the properties of the user-controllable GPIOs.

        Provided the device supports user-controllable GPIOs, they will be
        returned by this method.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          A list of ``JLinkGPIODescriptor`` instances totalling the number of
          requested properties.

        Raises:
          JLinkException: on error.
        """
        res = self._dll.JLINK_EMU_GPIO_GetProps(0, 0)
        if res < 0:
            raise errors.JLinkException(res)

        num_props = res
        buf = (structs.JLinkGPIODescriptor * num_props)()
        res = self._dll.JLINK_EMU_GPIO_GetProps(ctypes.byref(buf), num_props)
        if res < 0:
            raise errors.JLinkException(res)

        return list(buf)

    @open_required
    def gpio_get(self, pins=None):
        """Returns a list of states for the given pins.

        Defaults to the first four pins if an argument is not given.

        Args:
          self (JLink): the ``JLink`` instance
          pins (list): indices of the GPIO pins whose states are requested

        Returns:
          A list of states.

        Raises:
          JLinkException: on error.
        """
        if pins is None:
            pins = range(4)

        size = len(pins)
        indices = (ctypes.c_uint8 * size)(*pins)
        statuses = (ctypes.c_uint8 * size)()
        result = self._dll.JLINK_EMU_GPIO_GetState(ctypes.byref(indices),
                                                   ctypes.byref(statuses),
                                                   size)
        if result < 0:
            raise errors.JLinkException(result)

        return list(statuses)

    @open_required
    def gpio_set(self, pins, states):
        """Sets the state for one or more user-controllable GPIOs.

        For each of the given pins, sets the the corresponding state based on
        the index.

        Args:
          self (JLink): the ``JLink`` instance
          pins (list): list of GPIO indices
          states (list): list of states to set

        Returns:
          A list of updated states.

        Raises:
          JLinkException: on error.
          ValueError: if ``len(pins) != len(states)``
        """
        if len(pins) != len(states):
            raise ValueError('Length mismatch between pins and states.')

        size = len(pins)
        indices = (ctypes.c_uint8 * size)(*pins)
        states = (ctypes.c_uint8 * size)(*states)
        result_states = (ctypes.c_uint8 * size)()
        result = self._dll.JLINK_EMU_GPIO_SetState(ctypes.byref(indices),
                                                   ctypes.byref(states),
                                                   ctypes.byref(result_states),
                                                   size)
        if result < 0:
            raise errors.JLinkException(result)

        return list(result_states)

    @open_required
    def comm_supported(self):
        """Returns true if the connected emulator supports ``comm_*``
        functions.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if the emulator supports ``comm_*`` functions, otherwise
          ``False``.
        """
        return bool(self._dll.JLINKARM_EMU_COM_IsSupported())

    @open_required
    def power_on(self, default=False):
        """Turns on the power supply over pin 19 of the JTAG connector.

        If given the optional ``default`` parameter, activates the power supply
        by default.

        Args:
          self (JLink): the ``JLink`` instance
          default (bool): boolean indicating if to set power by default

        Returns:
          ``None``

        Raises:
          JLinkException: if J-Link does not support powering the target.
        """
        if default:
            return self.exec_command('SupplyPowerDefault = 1')
        return self.exec_command('SupplyPower = 1')

    @open_required
    def power_off(self, default=False):
        """Turns off the power supply over pin 19 of the JTAG connector.

        If given the optional ``default`` parameter, deactivates the power supply
        by default.

        Args:
          self (JLink): the ``JLink`` instance
          default (bool): boolean indicating if to set power off by default

        Returns:
          The current ``JLink`` instance

        Raises:
          JLinkException: if J-Link does not support powering the target.
        """
        if default:
            return self.exec_command('SupplyPowerDefault = 0')
        return self.exec_command('SupplyPower = 0')

    @connection_required
    def unlock(self):
        """Unlocks the device connected to the J-Link.

        Unlocking a device allows for access to read/writing memory, as well as
        flash programming.

        Note:
          Unlock is not supported on all devices.

        Supported Devices:
          Kinetis

        Returns:
          ``True``.

        Raises:
          JLinkException: if the device fails to unlock.
        """
        if not unlockers.unlock(self, self._device.manufacturer):
            raise errors.JLinkException('Failed to unlock device.')

        return True

    @connection_required
    def cpu_capability(self, capability):
        """Checks whether the J-Link has support for a CPU capability.

        This method checks if the emulator has built-in intelligence to handle
        the given CPU capability for the target CPU it is connected to.

        Args:
          self (JLink): the ``JLink`` instance
          capability (int): the capability to check for

        Returns:
          ``True`` if the J-Link has built-in intelligence to support the given
          ``capability`` for the CPU it is connected to, otherwise ``False``.
        """
        res = self._dll.JLINKARM_EMU_HasCPUCap(capability)
        return (res == 1)

    @connection_required
    def set_trace_source(self, source):
        """Sets the source to be used for tracing.

        The ``source`` must be one of the ones provided by
        ``enums.JLinkTraceSource``.

        Args:
          self (JLink): the ``JLink`` instance.
          source (int): the source to use.

        Returns:
          ``None``
        """
        self._dll.JLINKARM_SelectTraceSource(source)
        return None

    @connection_required
    def set_etb_trace(self):
        """Sets the trace source to ETB.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``
        """
        return self.set_trace_source(enums.JLinkTraceSource.ETB)

    @connection_required
    def set_etm_trace(self):
        """Sets the trace source to ETM.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``
        """
        return self.set_trace_source(enums.JLinkTraceSource.ETM)

    @open_required
    def set_reset_strategy(self, strategy):
        """Sets the reset strategy for the target.

        The reset strategy defines what happens when the target is reset.

        Args:
          self (JLink): the ``JLink`` instance
          strategy (int): the reset strategy to use

        Returns:
          The previous reset streategy.
        """
        return self._dll.JLINKARM_SetResetType(strategy)

    @open_required
    def set_reset_pin_high(self):
        """Sets the reset pin high.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_SetRESET()
        return None

    @open_required
    def set_reset_pin_low(self):
        """Sets the reset pin low.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ClrRESET()
        return None

    @open_required
    def set_tck_pin_high(self):
        """Sets the TCK pin to the high value (1).

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``

        Raises:
          JLinkException: if the emulator does not support this feature.
        """
        res = self._dll.JLINKARM_SetTCK()
        if res < 0:
            raise errors.JLinkException('Feature not supported.')
        return None

    @open_required
    def set_tck_pin_low(self):
        """Sets the TCK pin to the low value (0).

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``

        Raises:
          JLinkException: if the emulator does not support this feature.
        """
        res = self._dll.JLINKARM_ClrTCK()
        if res < 0:
            raise errors.JLinkException('Feature not supported.')
        return None

    @open_required
    def set_tdi_pin_high(self):
        """Sets the test data input to logical ``1``.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_SetTDI()

    @open_required
    def set_tdi_pin_low(self):
        """Clears the test data input.

        TDI is set to logical ``0`` (Ground).

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ClrTDI()

    @open_required
    def set_tms_pin_high(self):
        """Sets the test mode select to logical ``1``.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_SetTMS()

    @open_required
    def set_tms_pin_low(self):
        """Clears the test mode select.

        TMS is set to logical ``0`` (Ground).

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ClrTMS()

    @open_required
    def set_trst_pin_high(self):
        """Sets the TRST pin to high (``1``).

        Deasserts the TRST pin.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_SetTRST()

    @open_required
    def set_trst_pin_low(self):
        """Sets the TRST pin to low (``0``).

        This asserts the TRST pin.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ClrTRST()

    @connection_required
    def erase(self):
        """Erases the flash contents of the device.

        This erases the flash memory of the target device.  If this method
        fails, the device may be left in an inoperable state.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Number of bytes erased.
        """
        try:
            # This has to be in a try-catch, as the device may not be in a
            # state where it can halt, but we still want to try and erase.
            if not self.halted():
                self.halt()
        except errors.JLinkException:
            # Can't halt, so just continue to erasing.
            pass

        res = self._dll.JLINK_EraseChip()
        if res < 0:
            raise errors.JLinkEraseException(res)

        return res

    @connection_required
    def flash(self, data, addr, on_progress=None, power_on=False, flags=0):
        """Flashes the target device.

        The given ``on_progress`` callback will be called as
        ``on_progress(action, progress_string, percentage)`` periodically as the
        data is written to flash.  The action is one of ``Compare``, ``Erase``,
        ``Verify``, ``Flash``.

        Args:
          self (JLink): the ``JLink`` instance
          data (list|bytes): list or byte object of bytes to write to flash
          addr (int): start address on flash which to write the data
          on_progress (function): callback to be triggered on flash progress
          power_on (boolean): whether to power the target before flashing
          flags (int): reserved, do not use

        Returns:
          Number of bytes flashed.  This number may not necessarily be equal to
          ``len(data)``, but that does not indicate an error.

        Raises:
          JLinkException: on hardware errors.
        """
        if flags != 0:
            raise errors.JLinkException('Flags are reserved for future use.')

        if on_progress is not None:
            # Set the function to be called on flash programming progress.
            func = enums.JLinkFunctions.FLASH_PROGRESS_PROTOTYPE(on_progress)
            self._dll.JLINK_SetFlashProgProgressCallback(func)
        else:
            self._dll.JLINK_SetFlashProgProgressCallback(0)

        # First power on the device.
        if power_on:
            self.power_on()

        try:
            # Stop the target before flashing.  This is required to be in a
            # try-catch as the 'halted()' check may fail with an exception.
            if not self.halted():
                self.halt()
        except errors.JLinkException:
            pass

        # Perform read-modify-write operation.
        self._dll.JLINKARM_BeginDownload(flags=flags)

        if isinstance(data, list):
            data = bytes(data)

        bytes_flashed = self._dll.JLINKARM_WriteMem(addr, len(data), data)

        res = self._dll.JLINKARM_EndDownload()
        if res < 0:
            raise errors.JLinkEraseException(res)

        return bytes_flashed

    @connection_required
    def flash_file(self, path, addr, on_progress=None, power_on=False):
        """Flashes the target device.

        The given ``on_progress`` callback will be called as
        ``on_progress(action, progress_string, percentage)`` periodically as the
        data is written to flash.  The action is one of ``Compare``, ``Erase``,
        ``Verify``, ``Flash``.

        Args:
          self (JLink): the ``JLink`` instance
          path (str): absolute path to the source file to flash
          addr (int): start address on flash which to write the data
          on_progress (function): callback to be triggered on flash progress
          power_on (boolean): whether to power the target before flashing

        Returns:
          Integer value greater than or equal to zero.  Has no significance.

        Raises:
          JLinkException: on hardware errors.
        """
        if on_progress is not None:
            # Set the function to be called on flash programming progress.
            func = enums.JLinkFunctions.FLASH_PROGRESS_PROTOTYPE(on_progress)
            self._dll.JLINK_SetFlashProgProgressCallback(func)
        else:
            self._dll.JLINK_SetFlashProgProgressCallback(0)

        # First power on the device.
        if power_on:
            self.power_on()

        try:
            # Stop the target before flashing.  This is required to be in a
            # try-catch as the 'halted()' check may fail with an exception.
            if not self.halted():
                self.halt()
        except errors.JLinkException:
            pass

        # Program the target.
        bytes_flashed = self._dll.JLINK_DownloadFile(path.encode(), addr)
        if bytes_flashed < 0:
            raise errors.JLinkFlashException(bytes_flashed)

        return bytes_flashed

    @connection_required
    def reset(self, ms=0, halt=True):
        """Resets the target.

        This method resets the target, and by default toggles the RESET and
        TRST pins.

        Args:
          self (JLink): the ``JLink`` instance
          ms (int): Amount of milliseconds to delay after reset (default: 0)
          halt (bool): if the CPU should halt after reset (default: True)

        Returns:
          Number of bytes read.
        """
        self._dll.JLINKARM_SetResetDelay(ms)

        res = self._dll.JLINKARM_Reset()
        if res < 0:
            raise errors.JLinkException(res)
        elif not halt:
            self._dll.JLINKARM_Go()

        return res

    @connection_required
    def reset_tap(self):
        """Resets the TAP controller via TRST.

        Note:
          This must be called at least once after power up if the TAP
          controller is to be used.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ResetTRST()

    @connection_required
    def restart(self, num_instructions=0, skip_breakpoints=False):
        """Restarts the CPU core and simulates/emulates instructions.

        Note:
          This is a no-op if the CPU isn't halted.

        Args:
          self (JLink): the ``JLink`` instance
          num_instructions (int): number of instructions to simulate, defaults
            to zero
          skip_breakpoints (bool): skip current breakpoint (default: ``False``)

        Returns:
          ``True`` if device was restarted, otherwise ``False``.

        Raises:
          ValueError: if instruction count is not a natural number.
        """
        if not util.is_natural(num_instructions):
            raise ValueError('Invalid instruction count: %s.' % num_instructions)

        if not self.halted():
            return False

        flags = 0
        if skip_breakpoints:
            flags = flags | enums.JLinkFlags.GO_OVERSTEP_BP

        self._dll.JLINKARM_GoEx(num_instructions, flags)

        return True

    @connection_required
    @decorators.async_decorator
    def halt(self):
        """Halts the CPU Core.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if halted, ``False`` otherwise.
        """
        res = int(self._dll.JLINKARM_Halt())
        if res == 0:
            time.sleep(1)
            return True
        return False

    @connection_required
    def halted(self):
        """Returns whether the CPU core was halted.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if the CPU core is halted, otherwise ``False``.

        Raises:
          JLinkException: on device errors.
        """
        result = int(self._dll.JLINKARM_IsHalted())
        if result < 0:
            raise errors.JLinkException(result)

        return (result > 0)

    @connection_required
    def core_id(self):
        """Returns the identifier of the target ARM core.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Integer identifier of ARM core.
        """
        return self._dll.JLINKARM_GetId()

    @connection_required
    def core_cpu(self):
        """Returns the identifier of the core CPU.

        Note:
          This is distinct from the value returned from ``core_id()`` which is
          the ARM specific identifier.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The identifier of the CPU core.
        """
        return self._dll.JLINKARM_CORE_GetFound()

    @connection_required
    def core_name(self):
        """Returns the name of the target ARM core.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The target core's name.
        """
        buf_size = self.MAX_BUF_SIZE
        buf = (ctypes.c_char * buf_size)()
        self._dll.JLINKARM_Core2CoreName(self.core_cpu(), buf, buf_size)
        return ctypes.string_at(buf).decode()

    @connection_required
    def ir_len(self):
        """Counts and returns the total length of instruction registers of all
        the devices in the JTAG scan chain.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Total instruction register length.
        """
        return self._dll.JLINKARM_GetIRLen()

    @connection_required
    def scan_len(self):
        """Retrieves and returns the length of the scan chain select register.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Length of the scan chain select register.
        """
        return self._dll.JLINKARM_GetScanLen()

    @connection_required
    def scan_chain_len(self, scan_chain):
        """Retrieves and returns the number of bits in the scan chain.

        Args:
          self (JLink): the ``JLink`` instance
          scan_chain (int): scan chain to be measured

        Returns:
          Number of bits in the specified scan chain.

        Raises:
          JLinkException: on error.
        """
        res = self._dll.JLINKARM_MeasureSCLen(scan_chain)
        if res < 0:
            raise errors.JLinkException(res)
        return res

    @connection_required
    def device_family(self):
        """Returns the device family of the target CPU.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Integer identifier of the device family.
        """
        return self._dll.JLINKARM_GetDeviceFamily()

    @connection_required
    def register_list(self):
        """Returns a list of the indices for the CPU registers.

        The returned indices can be used to read the register content or grab
        the register name.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          List of registers.
        """
        num_items = self.MAX_NUM_CPU_REGISTERS
        buf = (ctypes.c_uint32 * num_items)()
        num_regs = self._dll.JLINKARM_GetRegisterList(buf, num_items)
        return buf[:num_regs]

    @connection_required
    def register_name(self, register_index):
        """Retrives and returns the name of an ARM CPU register.

        Args:
          self (JLink): the ``JLink`` instance
          register_index (int): index of the register whose name to retrieve

        Returns:
          Name of the register.
        """
        result = self._dll.JLINKARM_GetRegisterName(register_index)
        return ctypes.cast(result, ctypes.c_char_p).value.decode()

    @connection_required
    def cpu_speed(self, silent=False):
        """Retrieves the CPU speed of the target.

        If the target does not support CPU frequency detection, this function
        will return ``0``.

        Args:
          self (JLink): the ``JLink`` instance
          silent (bool): ``True`` if the CPU detection should not report errors
            to the error handler on failure.

        Returns:
          The measured CPU frequency on success, otherwise ``0`` if the core does
          not support CPU frequency detection.

        Raises:
          JLinkException: on hardware error.
        """
        res = self._dll.JLINKARM_MeasureCPUSpeedEx(-1, 1, int(silent))
        if res < 0:
            raise errors.JLinkException(res)
        return res

    @connection_required
    def cpu_halt_reasons(self):
        """Retrives the reasons that the CPU was halted.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          A list of ``JLInkMOEInfo`` instances specifying the reasons for which
          the CPU was halted.  This list may be empty in the case that the CPU
          is not halted.

        Raises:
          JLinkException: on hardware error.
        """
        buf_size = self.MAX_NUM_MOES
        buf = (structs.JLinkMOEInfo * buf_size)()
        num_reasons = self._dll.JLINKARM_GetMOEs(buf, buf_size)
        if num_reasons < 0:
            raise errors.JLinkException(num_reasons)

        return list(buf)[:num_reasons]

    @connection_required
    def jtag_create_clock(self):
        """Creates a JTAG clock on TCK.

        Note:
          This function only needs to be called once.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The state of the TDO pin: either ``0`` or ``1``.
        """
        return self._dll.JLINKARM_Clock()

    @connection_required
    def jtag_send(self, tms, tdi, num_bits):
        """Sends data via JTAG.

        Sends data via JTAG on the rising clock edge, TCK.  At on each rising
        clock edge, on bit is transferred in from TDI and out to TDO.  The
        clock uses the TMS to step through the standard JTAG state machine.

        Args:
          self (JLink): the ``JLink`` instance
          tms (int): used to determine the state transitions for the Test
            Access Port (TAP) controller from its current state
          tdi (int): input data to be transferred in from TDI to TDO
          num_bits (int): a number in the range ``[1, 32]`` inclusively
            specifying the number of meaningful bits in the ``tms`` and
            ``tdi`` parameters for the purpose of extracting state and data
            information

        Returns:
          ``None``

        Raises:
          ValueError: if ``num_bits < 1`` or ``num_bits > 32``.

        See Also:
          `JTAG Technical Overview <https://www.xjtag.com/about-jtag/jtag-a-technical-overview>`_.
        """
        if not util.is_natural(num_bits) or num_bits <= 0 or num_bits > 32:
            raise ValueError('Number of bits must be >= 1 and <= 32.')
        self._dll.JLINKARM_StoreBits(tms, tdi, num_bits)
        return None

    @connection_required
    def jtag_flush(self):
        """Flushes the internal JTAG buffer.

        Note:
          The buffer is automatically flushed when a response from the target
          is expected, or the buffer is full.  This can be used after a
          ``memory_write()`` in order to flush the buffer.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_WriteBits()

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_read8(self, offset):
        """Gets a unit of ``8`` bits from the input buffer.

        Args:
          self (JLink): the ``JLink`` instance
          offset (int): the offset (in bits) from which to start reading

        Returns:
          The integer read from the input buffer.
        """
        value = self._dll.JLINK_SWD_GetU8(offset)
        return ctypes.c_uint8(value).value

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_read16(self, offset):
        """Gets a unit of ``16`` bits from the input buffer.

        Args:
          self (JLink): the ``JLink`` instance
          offset (int): the offset (in bits) from which to start reading

        Returns:
          The integer read from the input buffer.
        """
        value = self._dll.JLINK_SWD_GetU16(offset)
        return ctypes.c_uint16(value).value

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_read32(self, offset):
        """Gets a unit of ``32`` bits from the input buffer.

        Args:
          self (JLink): the ``JLink`` instance
          offset (int): the offset (in bits) from which to start reading

        Returns:
          The integer read from the input buffer.
        """
        value = self._dll.JLINK_SWD_GetU32(offset)
        return ctypes.c_uint32(value).value

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_write(self, output, value, nbits):
        """Writes bytes over SWD (Serial Wire Debug).

        Args:
          self (JLink): the ``JLink`` instance
          output (int): the output buffer offset to write to
          value (int): the value to write to the output buffer
          nbits (int): the number of bits needed to represent the ``output`` and
            ``value``

        Returns:
          The bit position of the response in the input buffer.
        """
        pDir = binpacker.pack(output, nbits)
        pIn = binpacker.pack(value, nbits)
        bitpos = self._dll.JLINK_SWD_StoreRaw(pDir, pIn, nbits)
        if bitpos < 0:
            raise errors.JLinkException(bitpos)

        return bitpos

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_write8(self, output, value):
        """Writes one byte over SWD (Serial Wire Debug).

        Args:
          self (JLink): the ``JLink`` instance
          output (int): the output buffer offset to write to
          value (int): the value to write to the output buffer

        Returns:
          The bit position of the response in the input buffer.
        """
        return self.swd_write(output, value, 8)

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_write16(self, output, value):
        """Writes two bytes over SWD (Serial Wire Debug).

        Args:
          self (JLink): the ``JLink`` instance
          output (int): the output buffer offset to write to
          value (int): the value to write to the output buffer

        Returns:
          The bit position of the response in the input buffer.
        """
        return self.swd_write(output, value, 16)

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_write32(self, output, value):
        """Writes four bytes over SWD (Serial Wire Debug).

        Args:
          self (JLink): the ``JLink`` instance
          output (int): the output buffer offset to write to
          value (int): the value to write to the output buffer

        Returns:
          The bit position of the response in the input buffer.
        """
        return self.swd_write(output, value, 32)

    @interface_required(enums.JLinkInterfaces.SWD)
    @connection_required
    def swd_sync(self, pad=False):
        """Causes a flush to write all data remaining in output buffers to SWD
        device.

        Args:
          self (JLink): the ``JLink`` instance
          pad (bool): ``True`` if should pad the data to full byte size

        Returns:
          ``None``
        """
        if pad:
            self._dll.JLINK_SWD_SyncBytes()
        else:
            self._dll.JLINK_SWD_SyncBits()
        return None

    @connection_required
    def flash_write(self, addr, data, nbits=None, flags=0):
        """Writes data to the flash region of a device.

        The given number of bits, if provided, must be either ``8``, ``16``, or
        ``32``.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): starting flash address to write to
          data (list): list of data units to write
          nbits (int): number of bits to use for each unit

        Returns:
          Number of bytes written to flash.
        """
        # This indicates that all data written from this point on will go into
        # the buffer of the flashloader of the DLL.
        self._dll.JLINKARM_BeginDownload(flags)

        self.memory_write(addr, data, nbits=nbits)

        # Start downloading the data into the flash memory.
        bytes_flashed = self._dll.JLINKARM_EndDownload()
        if bytes_flashed < 0:
            raise errors.JLinkFlashException(bytes_flashed)

        return bytes_flashed

    @connection_required
    def flash_write8(self, addr, data):
        """Writes bytes to the flash region of a device.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): starting flash address to write to
          data (list): list of bytes to write

        Returns:
          Number of bytes written to flash.
        """
        return self.flash_write(addr, data, 8)

    @connection_required
    def flash_write16(self, addr, data):
        """Writes halfwords to the flash region of a device.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): starting flash address to write to
          data (list): list of halfwords to write

        Returns:
          Number of bytes written to flash.
        """
        return self.flash_write(addr, data, 16)

    @connection_required
    def flash_write32(self, addr, data):
        """Writes words to the flash region of a device.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): starting flash address to write to
          data (list): list of words to write

        Returns:
          Number of bytes written to flash.
        """
        return self.flash_write(addr, data, 32)

    @connection_required
    def code_memory_read(self, addr, num_bytes):
        """Reads bytes from code memory.

        Note:
          This is similar to calling ``memory_read`` or ``memory_read8``,
          except that this uses a cache and reads ahead.  This should be used
          in instances where you want to read a small amount of bytes at a
          time, and expect to always read ahead.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): starting address from which to read
          num_bytes (int): number of bytes to read

        Returns:
          A list of bytes read from the target.

        Raises:
          JLinkException: if memory could not be read.
        """
        buf_size = num_bytes
        buf = (ctypes.c_uint8 * buf_size)()
        res = self._dll.JLINKARM_ReadCodeMem(addr, buf_size, buf)
        if res < 0:
            raise errors.JLinkException(res)
        return list(buf)[:res]

    @connection_required
    def num_memory_zones(self):
        """Returns the number of memory zones supported by the target.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          An integer count of the number of memory zones supported by the
          target.

        Raises:
          JLinkException: on error.
        """
        count = self._dll.JLINK_GetMemZones(0, 0)
        if count < 0:
            raise errors.JLinkException(count)
        return count

    @connection_required
    def memory_zones(self):
        """Gets all memory zones supported by the current target.

        Some targets support multiple memory zones.  This function provides the
        ability to get a list of all the memory zones to facilate using the
        memory zone routing functions.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          A list of all the memory zones as ``JLinkMemoryZone`` structures.

        Raises:
          JLinkException: on hardware errors.
        """
        count = self.num_memory_zones()
        if count == 0:
            return list()

        buf = (structs.JLinkMemoryZone * count)()
        res = self._dll.JLINK_GetMemZones(buf, count)
        if res < 0:
            raise errors.JLinkException(res)

        return list(buf)

    @connection_required
    def memory_read(self, addr, num_units, zone=None, nbits=None):
        """Reads memory from a target system or specific memory zone.

        The optional ``zone`` specifies a memory zone to access to read from,
        e.g. ``IDATA``, ``DDATA``, or ``CODE``.

        The given number of bits, if provided, must be either ``8``, ``16``, or
        ``32``.  If not provided, always reads ``num_units`` bytes.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to read from
          num_units (int): number of units to read
          zone (str): optional memory zone name to access
          nbits (int): number of bits to use for each unit

        Returns:
          List of units read from the target system.

        Raises:
          JLinkException: if memory could not be read.
          ValueError: if ``nbits`` is not ``None``, and not in ``8``, ``16``,
            or ``32``.
        """
        buf_size = num_units
        buf = None
        access = 0

        if nbits is None:
            buf = (ctypes.c_uint8 * buf_size)()
            access = 0
        elif nbits == 8:
            buf = (ctypes.c_uint8 * buf_size)()
            access = 1
        elif nbits == 16:
            buf = (ctypes.c_uint16 * buf_size)()
            access = 2
            buf_size = buf_size * access
        elif nbits == 32:
            buf = (ctypes.c_uint32 * buf_size)()
            access = 4
            buf_size = buf_size * access
        else:
            raise ValueError('Given bit size is invalid: %s' % nbits)

        args = [addr, buf_size, buf, access]

        method = self._dll.JLINKARM_ReadMemEx
        if zone is not None:
            method = self._dll.JLINKARM_ReadMemZonedEx
            args.append(zone.encode())

        units_read = method(*args)
        if units_read < 0:
            raise errors.JLinkReadException(units_read)

        return buf[:units_read]

    @connection_required
    def memory_read8(self, addr, num_bytes, zone=None):
        """Reads memory from the target system in units of bytes.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to read from
          num_bytes (int): number of bytes to read
          zone (str): memory zone to read from

        Returns:
          List of bytes read from the target system.

        Raises:
          JLinkException: if memory could not be read.
        """
        return self.memory_read(addr, num_bytes, zone=zone, nbits=8)

    @connection_required
    def memory_read16(self, addr, num_halfwords, zone=None):
        """Reads memory from the target system in units of 16-bits.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to read from
          num_halfwords (int): number of half words to read
          zone (str): memory zone to read from

        Returns:
          List of halfwords read from the target system.

        Raises:
          JLinkException: if memory could not be read
        """
        return self.memory_read(addr, num_halfwords, zone=zone, nbits=16)

    @connection_required
    def memory_read32(self, addr, num_words, zone=None):
        """Reads memory from the target system in units of 32-bits.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to read from
          num_words (int): number of words to read
          zone (str): memory zone to read from

        Returns:
          List of words read from the target system.

        Raises:
          JLinkException: if memory could not be read
        """
        return self.memory_read(addr, num_words, zone=zone, nbits=32)

    @connection_required
    def memory_read64(self, addr, num_long_words):
        """Reads memory from the target system in units of 64-bits.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to read from
          num_long_words (int): number of long words to read

        Returns:
          List of long words read from the target system.

        Raises:
          JLinkException: if memory could not be read
        """
        buf_size = num_long_words
        buf = (ctypes.c_ulonglong * buf_size)()
        units_read = self._dll.JLINKARM_ReadMemU64(addr, buf_size, buf, 0)
        if units_read < 0:
            raise errors.JLinkException(units_read)

        return buf[:units_read]

    @connection_required
    def memory_write(self, addr, data, zone=None, nbits=None):
        """Writes memory to a target system or specific memory zone.

        The optional ``zone`` specifies a memory zone to access to write to,
        e.g. ``IDATA``, ``DDATA``, or ``CODE``.

        The given number of bits, if provided, must be either ``8``, ``16``, or
        ``32``.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to write to
          data (list): list of data units to write
          zone (str): optional memory zone name to access
          nbits (int): number of bits to use for each unit

        Returns:
          Number of units written.

        Raises:
          JLinkException: on write hardware failure.
          ValueError: if ``nbits`` is not ``None``, and not in ``8``, ``16`` or
            ``32``.
        """
        buf_size = len(data)
        buf = None
        access = 0

        if nbits is None:
            # Pack the given data into an array of 8-bit unsigned integers in
            # order to write it successfully
            packed_data = map(lambda d: reversed(binpacker.pack(d)), data)
            packed_data = list(itertools.chain(*packed_data))

            buf_size = len(packed_data)
            buf = (ctypes.c_uint8 * buf_size)(*packed_data)

            # Allow the access width to be chosen for us.
            access = 0
        elif nbits == 8:
            buf = (ctypes.c_uint8 * buf_size)(*data)
            access = 1
        elif nbits == 16:
            buf = (ctypes.c_uint16 * buf_size)(*data)
            access = 2
            buf_size = buf_size * access
        elif nbits == 32:
            buf = (ctypes.c_uint32 * buf_size)(*data)
            access = 4
            buf_size = buf_size * access
        else:
            raise ValueError('Given bit size is invalid: %s' % nbits)

        args = [addr, buf_size, buf, access]

        method = self._dll.JLINKARM_WriteMemEx
        if zone is not None:
            method = self._dll.JLINKARM_WriteMemZonedEx
            args.append(zone.encode())

        units_written = method(*args)
        if units_written < 0:
            raise errors.JLinkWriteException(units_written)

        return units_written

    @connection_required
    def memory_write8(self, addr, data, zone=None):
        """Writes bytes to memory of a target system.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to write to
          data (list): list of bytes to write
          zone (str): optional memory zone to access

        Returns:
          Number of bytes written to target.

        Raises:
          JLinkException: on memory access error.
        """
        return self.memory_write(addr, data, zone, 8)

    @connection_required
    def memory_write16(self, addr, data, zone=None):
        """Writes half-words to memory of a target system.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to write to
          data (list): list of half-words to write
          zone (str): optional memory zone to access

        Returns:
          Number of half-words written to target.

        Raises:
          JLinkException: on memory access error.
        """
        return self.memory_write(addr, data, zone, 16)

    @connection_required
    def memory_write32(self, addr, data, zone=None):
        """Writes words to memory of a target system.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to write to
          data (list): list of words to write
          zone (str): optional memory zone to access

        Returns:
          Number of words written to target.

        Raises:
          JLinkException: on memory access error.
        """
        return self.memory_write(addr, data, zone, 32)

    @connection_required
    def memory_write64(self, addr, data, zone=None):
        """Writes long words to memory of a target system.

        Note:
          This is little-endian.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): start address to write to
          data (list): list of long words to write
          zone (str): optional memory zone to access

        Returns:
          Number of long words written to target.

        Raises:
          JLinkException: on memory access error.
        """
        words = []
        bitmask = 0xFFFFFFFF
        for long_word in data:
            words.append(long_word & bitmask)          # Last 32-bits
            words.append((long_word >> 32) & bitmask)  # First 32-bits
        return self.memory_write32(addr, words, zone=zone)

    @connection_required
    def register_read(self, register_index):
        """Reads the value from the given register.

        Args:
          self (JLink): the ``JLink`` instance
          register_index (int/str): the register to read

        Returns:
          The value stored in the given register.
        """
        # TODO: rename 'register_index' to 'register'
        if isinstance(register_index, six.string_types):
            register_index = self._get_register_index_from_name(register_index)
        return self._dll.JLINKARM_ReadReg(register_index)

    @connection_required
    def register_read_multiple(self, register_indices):
        """Retrieves the values from the registers specified.

        Args:
          self (JLink): the ``JLink`` instance
          register_indices (list): list of registers to read

        Returns:
          A list of values corresponding one-to-one for each of the given
          register indices.  The returned list of values are the values in
          order of which the indices were specified.

        Raises:
          JLinkException: if a given register is invalid or an error occurs.
        """
        # TODO: rename 'register_indices' to 'registers'
        register_indices = register_indices[:]
        num_regs = len(register_indices)
        for idx, indice in enumerate(register_indices):
            if isinstance(indice, six.string_types):
                register_indices[idx] = self._get_register_index_from_name(indice)
        buf = (ctypes.c_uint32 * num_regs)(*register_indices)
        data = (ctypes.c_uint32 * num_regs)(0)

        # TODO: For some reason, these statuses are wonky, not sure why, might
        # be bad documentation, but they cannot be trusted at all.
        statuses = (ctypes.c_uint8 * num_regs)(0)

        res = self._dll.JLINKARM_ReadRegs(buf, data, statuses, num_regs)
        if res < 0:
            raise errors.JLinkException(res)

        return list(data)

    @connection_required
    def register_write(self, reg_index, value):
        """Writes into an ARM register.

        Note:
          The data is not immediately written, but is cached before being
          transferred to the CPU on CPU start.

        Args:
          self (JLink): the ``JLink`` instance
          reg_index (int/str): the ARM register to write to
          value (int): the value to write to the register

        Returns:
          The value written to the ARM register.

        Raises:
          JLinkException: on write error.
        """
        # TODO: rename 'reg_index' to 'register'
        if isinstance(reg_index, six.string_types):
            reg_index = self._get_register_index_from_name(reg_index)
        res = self._dll.JLINKARM_WriteReg(reg_index, value)
        if res != 0:
            raise errors.JLinkException('Error writing to register %d' % reg_index)
        return value

    @connection_required
    def register_write_multiple(self, register_indices, values):
        """Writes to multiple CPU registers.

        Writes the values to the given registers in order.  There must be a
        one-to-one correspondence between the values and the registers
        specified.

        Args:
          self (JLink): the ``JLink`` instance
          register_indices (list): list of registers to write to
          values (list): list of values to write to the registers

        Returns:
          ``None``

        Raises:
          ValueError: if ``len(register_indices) != len(values)``
          JLinkException: if a register could not be written to or on error
        """
        # TODO: rename 'register_indices' to 'registers'
        register_indices = register_indices[:]
        if len(register_indices) != len(values):
            raise ValueError('Must be an equal number of registers and values')

        num_regs = len(register_indices)
        for idx, indice in enumerate(register_indices):
            if isinstance(indice, six.string_types):
                register_indices[idx] = self._get_register_index_from_name(indice)
        buf = (ctypes.c_uint32 * num_regs)(*register_indices)
        data = (ctypes.c_uint32 * num_regs)(*values)

        # TODO: For some reason, these statuses are wonky, not sure why, might
        # be bad documentation, but they cannot be trusted at all.
        statuses = (ctypes.c_uint8 * num_regs)(0)

        res = self._dll.JLINKARM_WriteRegs(buf, data, statuses, num_regs)
        if res != 0:
            raise errors.JLinkException(res)

        return None

    @connection_required
    def ice_register_read(self, register_index):
        """Reads a value from an ARM ICE register.

        Args:
          self (JLink): the ``JLink`` instance
          register_index (int): the register to read

        Returns:
          The value read from the register.
        """
        return self._dll.JLINKARM_ReadICEReg(register_index)

    @connection_required
    def ice_register_write(self, register_index, value, delay=False):
        """Writes a value to an ARM ICE register.

        Args:
          self (JLink): the ``JLink`` instance
          register_index (int): the ICE register to write to
          value (int): the value to write to the ICE register
          delay (bool): boolean specifying if the write should be delayed

        Returns:
          ``None``
        """
        self._dll.JLINKARM_WriteICEReg(register_index, int(value), int(delay))
        return None

    @connection_required
    def etm_supported(self):
        """Returns if the CPU core supports ETM.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``True`` if the CPU has the ETM unit, otherwise ``False``.
        """
        res = self._dll.JLINKARM_ETM_IsPresent()
        if (res == 1):
            return True

        # JLINKARM_ETM_IsPresent() only works on ARM 7/9 devices.  This
        # fallback checks if ETM is present by checking the Cortex ROM table
        # for debugging information for ETM.
        info = ctypes.c_uint32(0)
        index = enums.JLinkROMTable.ETM
        res = self._dll.JLINKARM_GetDebugInfo(index, ctypes.byref(info))
        if (res == 1):
            return False

        return True

    @connection_required
    def etm_register_read(self, register_index):
        """Reads a value from an ETM register.

        Args:
          self (JLink): the ``JLink`` instance.
          register_index (int): the register to read.

        Returns:
          The value read from the ETM register.
        """
        return self._dll.JLINKARM_ETM_ReadReg(register_index)

    @connection_required
    def etm_register_write(self, register_index, value, delay=False):
        """Writes a value to an ETM register.

        Args:
          self (JLink): the ``JLink`` instance.
          register_index (int): the register to write to.
          value (int): the value to write to the register.
          delay (bool): boolean specifying if the write should be buffered.

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ETM_WriteReg(int(register_index), int(value), int(delay))
        return None

    @coresight_configuration_required
    def coresight_read(self, reg, ap=True):
        """Reads an Ap/DP register on a CoreSight DAP.

        Wait responses and special handling are both handled by this method.

        Note:
          ``coresight_configure()`` must be called prior to calling this method.

        Args:
          self (JLink): the ``JLink`` instance
          reg (int): index of DP/AP register to read
          ap (bool): ``True`` if reading from an Access Port register,
            otherwise ``False`` for Debug Port

        Returns:
          Data read from register.

        Raises:
          JLinkException: on hardware error
        """
        data = ctypes.c_uint32()
        ap = 1 if ap else 0
        res = self._dll.JLINKARM_CORESIGHT_ReadAPDPReg(reg, ap, ctypes.byref(data))
        if res < 0:
            raise errors.JLinkException(res)

        return data.value

    @coresight_configuration_required
    def coresight_write(self, reg, data, ap=True):
        """Writes an Ap/DP register on a CoreSight DAP.

        Note:
          ``coresight_configure()`` must be called prior to calling this method.

        Args:
          self (JLink): the ``JLink`` instance
          reg (int): index of DP/AP register to write
          data (int): data to write
          ap (bool): ``True`` if writing to an Access Port register, otherwise
            ``False`` for Debug Port

        Returns:
          Number of repetitions needed until write request accepted.

        Raises:
          JLinkException: on hardware error
        """
        ap = 1 if ap else 0
        res = self._dll.JLINKARM_CORESIGHT_WriteAPDPReg(reg, ap, data)
        if res < 0:
            raise errors.JLinkException(res)
        return res

    @connection_required
    def enable_reset_pulls_reset(self):
        """Enables RESET pin toggling on the JTAG bus on resets.

        When ``.reset()`` is called, it will also toggle the RESET pin on the
        JTAG bus.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ResetPullsRESET(1)
        return None

    @connection_required
    def disable_reset_pulls_reset(self):
        """Disables RESET pin toggling on the JTAG bus on resets.

        When ``.reset()`` is called, it will not toggle the RESET pin on the
        JTAG bus.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ResetPullsRESET(0)
        return None

    @connection_required
    def enable_reset_pulls_trst(self):
        """Enables TRST pin toggling on the JTAG bus on resets.

        When ``.reset()`` is called, it will also toggle the TRST pin on the
        JTAG bus.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ResetPullsTRST(1)
        return None

    @connection_required
    def disable_reset_pulls_trst(self):
        """Disables TRST pin toggling on the JTAG bus on resets.

        When ``.reset()`` is called, it will not toggle the TRST pin on the
        JTAG bus.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_ResetPullsTRST(0)
        return None

    @connection_required
    def enable_reset_inits_registers(self):
        """Enables CPU register initialization on resets.

        When ``.reset()`` is called, it will initialize the CPU registers.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if was previously enabled, otherwise ``False``.
        """
        return bool(self._dll.JLINKARM_SetInitRegsOnReset(1))

    @connection_required
    def disable_reset_inits_registers(self):
        """Disables CPU register initialization on resets.

        When ``.reset()`` is called, the CPU registers will be read and not
        initialized.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if was previously enabled, otherwise ``False``.
        """
        return bool(self._dll.JLINKARM_SetInitRegsOnReset(0))

    @connection_required
    def set_little_endian(self):
        """Sets the target hardware to little endian.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if target was big endian before call, otherwise ``False``.
        """
        res = self._dll.JLINKARM_SetEndian(0)
        return (res == 1)

    @connection_required
    def set_big_endian(self):
        """Sets the target hardware to big endian.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if target was little endian before call, otherwise ``False``.
        """
        res = self._dll.JLINKARM_SetEndian(1)
        return (res == 0)

    @connection_required
    def set_vector_catch(self, flags):
        """Sets vector catch bits of the processor.

        The CPU will jump to a vector if the given vector catch is active, and
        will enter a debug state.  This has the effect of halting the CPU as
        well, meaning the CPU must be explicitly restarted.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``

        Raises:
          JLinkException: on error.
        """
        res = self._dll.JLINKARM_WriteVectorCatch(flags)
        if res < 0:
            raise errors.JLinkException(res)
        return None

    @connection_required
    def step(self, thumb=False):
        """Executes a single step.

        Steps even if there is a breakpoint.

        Args:
          self (JLink): the ``JLink`` instance
          thumb (bool): boolean indicating if to step in thumb mode

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        method = self._dll.JLINKARM_Step
        if thumb:
            method = self._dll.JLINKARM_StepComposite

        res = method()
        if res != 0:
            raise errors.JLinkException("Failed to step over instruction.")

        return None

    @connection_required
    def enable_soft_breakpoints(self):
        """Enables software breakpoints.

        Note:
          This should be called before calling ``software_breakpoint_set()``.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_EnableSoftBPs(1)

    @connection_required
    def disable_soft_breakpoints(self):
        """Disables software breakpoints.

        Note:
          After this function is called, ``software_breakpoint_set()`` cannot
          be used without first calling ``enable_soft_breakpoints()``.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``
        """
        self._dll.JLINKARM_EnableSoftBPs(0)

    @connection_required
    def num_active_breakpoints(self):
        """Returns the number of currently active breakpoints.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The number of breakpoints that are currently set.
        """
        return self._dll.JLINKARM_GetNumBPs()

    @connection_required
    def num_available_breakpoints(self, arm=False, thumb=False, ram=False, flash=False, hw=False):
        """Returns the number of available breakpoints of the specified type.

        If ``arm`` is set, gets the number of available ARM breakpoint units.
        If ``thumb`` is set, gets the number of available THUMB breakpoint
        units.  If ``ram`` is set, gets the number of available software RAM
        breakpoint units.  If ``flash`` is set, gets the number of available
        software flash breakpoint units.  If ``hw`` is set, gets the number of
        available hardware breakpoint units.

        If a combination of the flags is given, then
        ``num_available_breakpoints()`` returns the number of breakpoints
        specified by the given flags.  If no flags are specified, then the
        count of available breakpoint units is returned.

        Args:
          self (JLink): the ``JLink`` instance
          arm (bool): Boolean indicating to get number of ARM breakpoints.
          thumb (bool): Boolean indicating to get number of THUMB breakpoints.
          ram (bool): Boolean indicating to get number of SW RAM breakpoints.
          flash (bool): Boolean indicating to get number of Flash breakpoints.
          hw (bool): Boolean indicating to get number of Hardware breakpoints.

        Returns:
          The number of available breakpoint units of the specified type.
        """
        flags = [
            enums.JLinkBreakpoint.ARM,
            enums.JLinkBreakpoint.THUMB,
            enums.JLinkBreakpoint.SW_RAM,
            enums.JLinkBreakpoint.SW_FLASH,
            enums.JLinkBreakpoint.HW
        ]

        set_flags = [
            arm,
            thumb,
            ram,
            flash,
            hw
        ]

        if not any(set_flags):
            flags = enums.JLinkBreakpoint.ANY
        else:
            flags = list(f for i, f in enumerate(flags) if set_flags[i])
            flags = functools.reduce(operator.__or__, flags, 0)

        return self._dll.JLINKARM_GetNumBPUnits(flags)

    @connection_required
    def breakpoint_info(self, handle=0, index=-1):
        """Returns the information about a set breakpoint.

        Note:
          Either ``handle`` or ``index`` can be specified.  If the ``index``
          is not provided, the ``handle`` must be set, and vice-versa.  If
          both ``index`` and ``handle`` are provided, the ``index`` overrides
          the provided ``handle``.

        Args:
          self (JLink): the ``JLink`` instance
          handle (int): option handle of a valid breakpoint
          index (int): optional index of the breakpoint.

        Returns:
          An instance of ``JLinkBreakpointInfo`` specifying information about
          the breakpoint.

        Raises:
          JLinkException: on error.
          ValueError: if both the handle and index are invalid.
        """
        if index < 0 and handle == 0:
            raise ValueError('Handle must be provided if index is not set.')

        bp = structs.JLinkBreakpointInfo()
        bp.Handle = int(handle)
        res = self._dll.JLINKARM_GetBPInfoEx(index, ctypes.byref(bp))
        if res < 0:
            raise errors.JLinkException('Failed to get breakpoint info.')

        return bp

    @connection_required
    def breakpoint_find(self, addr):
        """Returns the handle of a breakpoint at the given address, if any.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): the address to search for the breakpoint

        Returns:
          A non-zero integer if a breakpoint was found at the given address,
          otherwise zero.
        """
        return self._dll.JLINKARM_FindBP(addr)

    @connection_required
    def breakpoint_set(self, addr, thumb=False, arm=False):
        """Sets a breakpoint at the specified address.

        If ``thumb`` is ``True``, the breakpoint is set in THUMB-mode, while if
        ``arm`` is ``True``, the breakpoint is set in ARM-mode, otherwise a
        normal breakpoint is set.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): the address where the breakpoint will be set
          thumb (bool): boolean indicating to set the breakpoint in THUMB mode
          arm (bool): boolean indicating to set the breakpoint in ARM mode

        Returns:
          An integer specifying the breakpoint handle.  This handle should be
          retained for future breakpoint operations.

        Raises:
          TypeError: if the given address is not an integer.
          JLinkException: if the breakpoint could not be set.
        """
        flags = enums.JLinkBreakpoint.ANY

        if thumb:
            flags = flags | enums.JLinkBreakpoint.THUMB
        elif arm:
            flags = flags | enums.JLinkBreakpoint.ARM

        handle = self._dll.JLINKARM_SetBPEx(int(addr), flags)
        if handle <= 0:
            raise errors.JLinkException('Breakpoint could not be set.')

        return handle

    @connection_required
    def software_breakpoint_set(self, addr, thumb=False, arm=False, flash=False, ram=False):
        """Sets a software breakpoint at the specified address.

        If ``thumb`` is ``True``, the breakpoint is set in THUMB-mode, while if
        ``arm`` is ``True``, the breakpoint is set in ARM-mode, otherwise a
        normal breakpoint is set.

        If ``flash`` is ``True``, the breakpoint is set in flash, otherwise if
        ``ram`` is ``True``, the breakpoint is set in RAM.  If both are
        ``True`` or both are ``False``, then the best option is chosen for
        setting the breakpoint in software.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): the address where the breakpoint will be set
          thumb (bool): boolean indicating to set the breakpoint in THUMB mode
          arm (bool): boolean indicating to set the breakpoint in ARM mode
          flash (bool): boolean indicating to set the breakpoint in flash
          ram (bool): boolean indicating to set the breakpoint in RAM

        Returns:
          An integer specifying the breakpoint handle.  This handle should sbe
          retained for future breakpoint operations.

        Raises:
          TypeError: if the given address is not an integer.
          JLinkException: if the breakpoint could not be set.
        """
        if flash and not ram:
            flags = enums.JLinkBreakpoint.SW_FLASH
        elif not flash and ram:
            flags = enums.JLinkBreakpoint.SW_RAM
        else:
            flags = enums.JLinkBreakpoint.SW

        if thumb:
            flags = flags | enums.JLinkBreakpoint.THUMB
        elif arm:
            flags = flags | enums.JLinkBreakpoint.ARM

        handle = self._dll.JLINKARM_SetBPEx(int(addr), flags)
        if handle <= 0:
            raise errors.JLinkException('Software breakpoint could not be set.')

        return handle

    @connection_required
    def hardware_breakpoint_set(self, addr, thumb=False, arm=False):
        """Sets a hardware breakpoint at the specified address.

        If ``thumb`` is ``True``, the breakpoint is set in THUMB-mode, while if
        ``arm`` is ``True``, the breakpoint is set in ARM-mode, otherwise a
        normal breakpoint is set.

        Args:
          self (JLink): the ``JLink`` instance
          addr (int): the address where the breakpoint will be set
          thumb (bool): boolean indicating to set the breakpoint in THUMB mode
          arm (bool): boolean indicating to set the breakpoint in ARM mode

        Returns:
          An integer specifying the breakpoint handle.  This handle should sbe
          retained for future breakpoint operations.

        Raises:
          TypeError: if the given address is not an integer.
          JLinkException: if the breakpoint could not be set.
        """
        flags = enums.JLinkBreakpoint.HW

        if thumb:
            flags = flags | enums.JLinkBreakpoint.THUMB
        elif arm:
            flags = flags | enums.JLinkBreakpoint.ARM

        handle = self._dll.JLINKARM_SetBPEx(int(addr), flags)
        if handle <= 0:
            raise errors.JLinkException('Hardware breakpoint could not be set.')

        return handle

    @connection_required
    def breakpoint_clear(self, handle):
        """Removes a single breakpoint.

        Args:
          self (JLink): the ``JLink`` instance
          handle (int): the handle of the breakpoint to be removed

        Returns:
          ``True`` if the breakpoint was cleared, otherwise ``False`` if the
          breakpoint was not valid.
        """
        return not self._dll.JLINKARM_ClrBPEx(handle)

    @connection_required
    def breakpoint_clear_all(self):
        """Removes all breakpoints that have been set.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if they were cleared, otherwise ``False``.
        """
        return not self._dll.JLINKARM_ClrBPEx(0xFFFFFFFF)

    @connection_required
    def num_active_watchpoints(self):
        """Returns the number of currently active watchpoints.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The number of watchpoints that are currently set.
        """
        return self._dll.JLINKARM_GetNumWPs()

    @connection_required
    def num_available_watchpoints(self):
        """Returns the number of available watchpoints.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The number of watchpoints that are available to be set.
        """
        return self._dll.JLINKARM_GetNumWPUnits()

    @connection_required
    def watchpoint_info(self, handle=0, index=-1):
        """Returns information about the specified watchpoint.

        Note:
          Either ``handle`` or ``index`` can be specified.  If the ``index``
          is not provided, the ``handle`` must be set, and vice-versa.  If
          both ``index`` and ``handle`` are provided, the ``index`` overrides
          the provided ``handle``.

        Args:
          self (JLink): the ``JLink`` instance
          handle (int): optional handle of a valid watchpoint.
          index (int): optional index of a watchpoint.

        Returns:
          An instance of ``JLinkWatchpointInfo`` specifying information about
          the watchpoint if the watchpoint was found, otherwise ``None``.

        Raises:
          JLinkException: on error.
          ValueError: if both handle and index are invalid.
        """
        if index < 0 and handle == 0:
            raise ValueError('Handle must be provided if index is not set.')

        wp = structs.JLinkWatchpointInfo()
        res = self._dll.JLINKARM_GetWPInfoEx(index, ctypes.byref(wp))
        if res < 0:
            raise errors.JLinkException('Failed to get watchpoint info.')

        for i in range(res):
            res = self._dll.JLINKARM_GetWPInfoEx(i, ctypes.byref(wp))
            if res < 0:
                raise errors.JLinkException('Failed to get watchpoint info.')
            elif wp.Handle == handle or wp.WPUnit == index:
                return wp

        return None

    @connection_required
    def watchpoint_set(self,
                       addr,
                       addr_mask=0x0,
                       data=0x0,
                       data_mask=0x0,
                       access_size=None,
                       read=False,
                       write=False,
                       privileged=False):
        """Sets a watchpoint at the given address.

        This method allows for a watchpoint to be set on an given address or
        range of addresses.  The watchpoint can then be triggered if the data
        at the given address matches the specified ``data`` or range of data as
        determined by ``data_mask``, on specific access size events, reads,
        writes, or privileged accesses.

        Both ``addr_mask`` and ``data_mask`` are used to specify ranges.  Bits
        set to ``1`` are masked out and not taken into consideration when
        comparison against an address or data value.  E.g. an ``addr_mask``
        with a value of ``0x1`` and ``addr`` with value ``0xdeadbeef`` means
        that the watchpoint will be set on addresses ``0xdeadbeef`` and
        ``0xdeadbeee``.  If the ``data`` was ``0x11223340`` and the given
        ``data_mask`` has a value of ``0x0000000F``, then the watchpoint would
        trigger for data matching ``0x11223340 - 0x1122334F``.

        Note:
          If both ``read`` and ``write`` are specified, then the watchpoint
          will trigger on both read and write events to the given address.

        Args:
          self (JLink): the ``JLink`` instance
          addr_mask (int): optional mask to use for determining which address
            the watchpoint should be set on
          data (int): optional data to set the watchpoint on in order to have
            the watchpoint triggered when the value at the specified address
            matches the given ``data``
          data_mask (int): optional mask to use for determining the range of
            data on which the watchpoint should be triggered
          access_size (int): if specified, this must be one of ``{8, 16, 32}``
            and determines the access size for which the watchpoint should
            trigger
          read (bool): if ``True``, triggers the watchpoint on read events
          write (bool): if ``True``, triggers the watchpoint on write events
          privileged (bool): if ``True``, triggers the watchpoint on privileged
            accesses

        Returns:
          The handle of the created watchpoint.

        Raises:
          ValueError: if an invalid access size is given.
          JLinkException: if the watchpoint fails to be set.
        """
        access_flags = 0x0
        access_mask_flags = 0x0

        # If an access size is not specified, we must specify that the size of
        # the access does not matter by specifying the access mask flags.
        if access_size is None:
            access_mask_flags = access_mask_flags | enums.JLinkAccessMaskFlags.SIZE
        elif access_size == 8:
            access_flags = access_flags | enums.JLinkAccessFlags.SIZE_8BIT
        elif access_size == 16:
            access_flags = access_flags | enums.JLinkAccessFlags.SIZE_16BIT
        elif access_size == 32:
            access_flags = access_flags | enums.JLinkAccessFlags.SIZE_32BIT
        else:
            raise ValueError('Invalid access size given: %d' % access_size)

        # The read and write access flags cannot be specified together, so if
        # the user specifies that they want read and write access, then the
        # access mask flag must be set.
        if read and write:
            access_mask_flags = access_mask_flags | enums.JLinkAccessMaskFlags.DIR
        elif read:
            access_flags = access_flags | enums.JLinkAccessFlags.READ
        elif write:
            access_flags = access_flags | enums.JLinkAccessFlags.WRITE

        # If privileged is not specified, then there is no specification level
        # on which kinds of writes should be accessed, in which case we must
        # specify that flag.
        if privileged:
            access_flags = access_flags | enums.JLinkAccessFlags.PRIV
        else:
            access_mask_flags = access_mask_flags | enums.JLinkAccessMaskFlags.PRIV

        # Populate the Data event to configure how the watchpoint is triggered.
        wp = structs.JLinkDataEvent()
        wp.Addr = addr
        wp.AddrMask = addr_mask
        wp.Data = data
        wp.DataMask = data_mask
        wp.Access = access_flags
        wp.AccessMask = access_mask_flags

        # Return value of the function is <= 0 in the event of an error,
        # otherwise the watchpoint was set successfully.
        handle = ctypes.c_uint32()
        res = self._dll.JLINKARM_SetDataEvent(ctypes.pointer(wp), ctypes.pointer(handle))
        if res < 0:
            raise errors.JLinkDataException(res)

        return handle.value

    @connection_required
    def watchpoint_clear(self, handle):
        """Clears the watchpoint with the specified handle.

        Args:
          self (JLink): the ``JLink`` instance
          handle (int): the handle of the watchpoint

        Returns:
          ``True`` if watchpoint was removed, otherwise ``False``.
        """
        return not self._dll.JLINKARM_ClrDataEvent(handle)

    @connection_required
    def watchpoint_clear_all(self):
        """Removes all watchpoints that have been set.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if they were cleared, otherwise ``False``.
        """
        return not self._dll.JLINKARM_ClrDataEvent(0xFFFFFFFF)

    def disassemble_instruction(self, instruction):
        """Disassembles and returns the assembly instruction string.

        Args:
          self (JLink): the ``JLink`` instance.
          instruction (int): the instruction address.

        Returns:
          A string corresponding to the assembly instruction string at the
          given instruction address.

        Raises:
          JLinkException: on error.
          TypeError: if ``instruction`` is not a number.
        """
        if not util.is_integer(instruction):
            raise TypeError('Expected instruction to be an integer.')

        buf_size = self.MAX_BUF_SIZE
        buf = (ctypes.c_char * buf_size)()
        res = self._dll.JLINKARM_DisassembleInst(ctypes.byref(buf), buf_size, instruction)
        if res < 0:
            raise errors.JLinkException('Failed to disassemble instruction.')

        return ctypes.string_at(buf).decode()

###############################################################################
#
# STRACE API
#
###############################################################################

    @connection_required
    def strace_configure(self, port_width):
        """Configures the trace port width for tracing.

        Note that configuration cannot occur while STRACE is running.

        Args:
          self (JLink): the ``JLink`` instance
          port_width (int): the trace port width to use.

        Returns:
          ``None``

        Raises:
          ValueError: if ``port_width`` is not ``1``, ``2``, or ``4``.
          JLinkException: on error.
        """
        if port_width not in [1, 2, 4]:
            raise ValueError('Invalid port width: %s' % str(port_width))

        config_string = 'PortWidth=%d' % port_width
        res = self._dll.JLINK_STRACE_Config(config_string.encode())
        if res < 0:
            raise errors.JLinkException('Failed to configure STRACE port')

        return None

    @connection_required
    def strace_start(self):
        """Starts the capturing of STRACE data.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``

        Raises:
          JLinkException: on error.
        """
        res = self._dll.JLINK_STRACE_Start()
        if res < 0:
            raise errors.JLinkException('Failed to start STRACE.')

        return None

    @connection_required
    def strace_stop(self):
        """Stops the sampling of STRACE data.

        Any capturing of STRACE data is automatically stopped when the CPU is
        halted.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``

        Raises:
          JLinkException: on error.
        """
        res = self._dll.JLINK_STRACE_Stop()
        if res < 0:
            raise errors.JLinkException('Failed to stop STRACE.')

        return None

    @connection_required
    def strace_read(self, num_instructions):
        """Reads and returns a number of instructions captured by STRACE.

        The number of instructions must be a non-negative value of at most
        ``0x10000`` (``65536``).

        Args:
          self (JLink): the ``JLink`` instance.
          num_instructions (int): number of instructions to fetch.

        Returns:
          A list of instruction addresses in order from most recently executed
          to oldest executed instructions.  Note that the number of
          instructions returned can be less than the number of instructions
          requested in the case that there are not ``num_instructions`` in the
          trace buffer.

        Raises:
          JLinkException: on error.
          ValueError: if ``num_instructions < 0`` or
            ``num_instructions > 0x10000``.
        """
        if num_instructions < 0 or num_instructions > 0x10000:
            raise ValueError('Invalid instruction count.')

        buf = (ctypes.c_uint32 * num_instructions)()
        buf_size = num_instructions
        res = self._dll.JLINK_STRACE_Read(ctypes.byref(buf), buf_size)
        if res < 0:
            raise errors.JLinkException('Failed to read from STRACE buffer.')

        return list(buf)[:res]

    @connection_required
    def strace_code_fetch_event(self, operation, address, address_range=0):
        """Sets an event to trigger trace logic when an instruction is fetched.

        Args:
          self (JLink): the ``JLink`` instance.
          operation (int): one of the operations in ``JLinkStraceOperation``.
          address (int): the address of the instruction that is fetched.
          address_range (int): optional range of address to trigger event on.

        Returns:
          An integer specifying the trace event handle.  This handle should be
          retained in order to clear the event at a later time.

        Raises:
          JLinkException: on error.
        """
        cmd = enums.JLinkStraceCommand.TRACE_EVENT_SET
        event_info = structs.JLinkStraceEventInfo()
        event_info.Type = enums.JLinkStraceEvent.CODE_FETCH
        event_info.Op = operation
        event_info.Addr = int(address)
        event_info.AddrRangeSize = int(address_range)
        handle = self._dll.JLINK_STRACE_Control(cmd, ctypes.byref(event_info))
        if handle < 0:
            raise errors.JLinkException(handle)

        return handle

    @connection_required
    def strace_data_access_event(self,
                                 operation,
                                 address,
                                 data,
                                 data_mask=None,
                                 access_width=4,
                                 address_range=0):
        """Sets an event to trigger trace logic when data access is made.

        Data access corresponds to either a read or write.

        Args:
          self (JLink): the ``JLink`` instance.
          operation (int): one of the operations in ``JLinkStraceOperation``.
          address (int): the address of the load/store data.
          data (int): the data to be compared the event data to.
          data_mask (int): optional bitmask specifying bits to ignore in
            comparison.
          acess_width (int): optional access width for the data.
          address_range (int): optional range of address to trigger event on.

        Returns:
          An integer specifying the trace event handle.  This handle should be
          retained in order to clear the event at a later time.

        Raises:
          JLinkException: on error.
        """
        cmd = enums.JLinkStraceCommand.TRACE_EVENT_SET
        event_info = structs.JLinkStraceEventInfo()
        event_info.Type = enums.JLinkStraceEvent.DATA_ACCESS
        event_info.Op = operation
        event_info.AccessSize = int(access_width)
        event_info.Addr = int(address)
        event_info.Data = int(data)
        event_info.DataMask = int(data_mask or 0)
        event_info.AddrRangeSize = int(address_range)
        handle = self._dll.JLINK_STRACE_Control(cmd, ctypes.byref(event_info))
        if handle < 0:
            raise errors.JLinkException(handle)

        return handle

    @connection_required
    def strace_data_load_event(self, operation, address, address_range=0):
        """Sets an event to trigger trace logic when data read access is made.

        Args:
          self (JLink): the ``JLink`` instance.
          operation (int): one of the operations in ``JLinkStraceOperation``.
          address (int): the address of the load data.
          address_range (int): optional range of address to trigger event on.

        Returns:
          An integer specifying the trace event handle.  This handle should be
          retained in order to clear the event at a later time.

        Raises:
          JLinkException: on error.
        """
        cmd = enums.JLinkStraceCommand.TRACE_EVENT_SET
        event_info = structs.JLinkStraceEventInfo()
        event_info.Type = enums.JLinkStraceEvent.DATA_LOAD
        event_info.Op = operation
        event_info.Addr = int(address)
        event_info.AddrRangeSize = int(address_range)
        handle = self._dll.JLINK_STRACE_Control(cmd, ctypes.byref(event_info))
        if handle < 0:
            raise errors.JLinkException(handle)

        return handle

    @connection_required
    def strace_data_store_event(self, operation, address, address_range=0):
        """Sets an event to trigger trace logic when data write access is made.

        Args:
          self (JLink): the ``JLink`` instance.
          operation (int): one of the operations in ``JLinkStraceOperation``.
          address (int): the address of the store data.
          address_range (int): optional range of address to trigger event on.

        Returns:
          An integer specifying the trace event handle.  This handle should be
          retained in order to clear the event at a later time.

        Raises:
          JLinkException: on error.
        """
        cmd = enums.JLinkStraceCommand.TRACE_EVENT_SET
        event_info = structs.JLinkStraceEventInfo()
        event_info.Type = enums.JLinkStraceEvent.DATA_STORE
        event_info.Op = operation
        event_info.Addr = int(address)
        event_info.AddrRangeSize = int(address_range)
        handle = self._dll.JLINK_STRACE_Control(cmd, ctypes.byref(event_info))
        if handle < 0:
            raise errors.JLinkException(handle)

        return handle

    @connection_required
    def strace_clear(self, handle):
        """Clears the trace event specified by the given handle.

        Args:
          self (JLink): the ``JLink`` instance.
          handle (int): handle of the trace event.

        Returns:
          ``None``

        Raises:
          JLinkException: on error.
        """
        data = ctypes.c_int(handle)
        res = self._dll.JLINK_STRACE_Control(enums.JLinkStraceCommand.TRACE_EVENT_CLR, ctypes.byref(data))
        if res < 0:
            raise errors.JLinkException('Failed to clear STRACE event.')

        return None

    @connection_required
    def strace_clear_all(self):
        """Clears all STRACE events.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``

        Raises:
          JLinkException: on error.
        """
        data = 0
        res = self._dll.JLINK_STRACE_Control(enums.JLinkStraceCommand.TRACE_EVENT_CLR_ALL, data)
        if res < 0:
            raise errors.JLinkException('Failed to clear all STRACE events.')

        return None

    @connection_required
    def strace_set_buffer_size(self, size):
        """Sets the STRACE buffer size.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``

        Raises:
          JLinkException: on error.
        """
        size = ctypes.c_uint32(size)
        res = self._dll.JLINK_STRACE_Control(enums.JLinkStraceCommand.SET_BUFFER_SIZE, size)
        if res < 0:
            raise errors.JLinkException('Failed to set the STRACE buffer size.')

        return None

###############################################################################
#
# TRACE API
#
###############################################################################

    @connection_required
    def trace_start(self):
        """Starts collecting trace data.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``
        """
        cmd = enums.JLinkTraceCommand.START
        res = self._dll.JLINKARM_TRACE_Control(cmd, 0)
        if (res == 1):
            raise errors.JLinkException('Failed to start trace.')
        return None

    @connection_required
    def trace_stop(self):
        """Stops collecting trace data.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``
        """
        cmd = enums.JLinkTraceCommand.STOP
        res = self._dll.JLINKARM_TRACE_Control(cmd, 0)
        if (res == 1):
            raise errors.JLinkException('Failed to stop trace.')
        return None

    @connection_required
    def trace_flush(self):
        """Flushes the trace buffer.

        After this method is called, the trace buffer is empty.  This method is
        best called when the device is reset.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``
        """
        cmd = enums.JLinkTraceCommand.FLUSH
        res = self._dll.JLINKARM_TRACE_Control(cmd, 0)
        if (res == 1):
            raise errors.JLinkException('Failed to flush the trace buffer.')
        return None

    @connection_required
    def trace_sample_count(self):
        """Retrieves the number of samples in the trace buffer.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          Number of samples in the trace buffer.
        """
        cmd = enums.JLinkTraceCommand.GET_NUM_SAMPLES
        data = ctypes.c_uint32(self.trace_max_buffer_capacity())
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to get trace sample count.')
        return data.value

    @connection_required
    def trace_buffer_capacity(self):
        """Retrieves the trace buffer's current capacity.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          The current capacity of the trace buffer.  This is not necessarily
          the maximum possible size the buffer could be configured with.
        """
        cmd = enums.JLinkTraceCommand.GET_CONF_CAPACITY
        data = ctypes.c_uint32(0)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to get trace buffer size.')
        return data.value

    @connection_required
    def trace_set_buffer_capacity(self, size):
        """Sets the capacity for the trace buffer.

        Args:
          self (JLink): the ``JLink`` instance.
          size (int): the new capacity for the trace buffer.

        Returns:
          ``None``
        """
        cmd = enums.JLinkTraceCommand.SET_CAPACITY
        data = ctypes.c_uint32(size)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to set trace buffer size.')
        return None

    @connection_required
    def trace_min_buffer_capacity(self):
        """Retrieves the minimum capacity the trace buffer can be configured with.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          The minimum configurable capacity for the trace buffer.
        """
        cmd = enums.JLinkTraceCommand.GET_MIN_CAPACITY
        data = ctypes.c_uint32(0)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to get min trace buffer size.')
        return data.value

    @connection_required
    def trace_max_buffer_capacity(self):
        """Retrieves the maximum size the trace buffer can be configured with.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          The maximum configurable capacity for the trace buffer.
        """
        cmd = enums.JLinkTraceCommand.GET_MAX_CAPACITY
        data = ctypes.c_uint32(0)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to get max trace buffer size.')
        return data.value

    @connection_required
    def trace_set_format(self, fmt):
        """Sets the format for the trace buffer to use.

        Args:
          self (JLink): the ``JLink`` instance.
          fmt (int): format for the trace buffer; this is one of the attributes
            of ``JLinkTraceFormat``.

        Returns:
          ``None``
        """
        cmd = enums.JLinkTraceCommand.SET_FORMAT
        data = ctypes.c_uint32(fmt)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to set trace format.')
        return None

    @connection_required
    def trace_format(self):
        """Retrieves the current format the trace buffer is using.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          The current format the trace buffer is using.  This is one of the
          attributes of ``JLinkTraceFormat``.
        """
        cmd = enums.JLinkTraceCommand.GET_FORMAT
        data = ctypes.c_uint32(0)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to get trace format.')
        return data.value

    @connection_required
    def trace_region_count(self):
        """Retrieves a count of the number of available trace regions.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          Count of the number of available trace regions.
        """
        cmd = enums.JLinkTraceCommand.GET_NUM_REGIONS
        data = ctypes.c_uint32(0)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(data))
        if (res == 1):
            raise errors.JLinkException('Failed to get trace region count.')
        return data.value

    @connection_required
    def trace_region(self, region_index):
        """Retrieves the properties of a trace region.

        Args:
          self (JLink): the ``JLink`` instance.
          region_index (int): the trace region index.

        Returns:
          An instance of ``JLinkTraceRegion`` describing the specified region.
        """
        cmd = enums.JLinkTraceCommand.GET_REGION_PROPS_EX
        region = structs.JLinkTraceRegion()
        region.RegionIndex = int(region_index)
        res = self._dll.JLINKARM_TRACE_Control(cmd, ctypes.byref(region))
        if (res == 1):
            raise errors.JLinkException('Failed to get trace region.')
        return region

    @connection_required
    def trace_read(self, offset, num_items):
        """Reads data from the trace buffer and returns it.

        Args:
          self (JLink): the ``JLink`` instance.
          offset (int): the offset from which to start reading from the trace
            buffer.
          num_items (int): number of items to read from the trace buffer.

        Returns:
          A list of ``JLinkTraceData`` instances corresponding to the items
          read from the trace buffer.  Note that this list may have size less
          than ``num_items`` in the event that there are not ``num_items``
          items in the trace buffer.

        Raises:
          JLinkException: on error.
        """
        buf_size = ctypes.c_uint32(num_items)
        buf = (structs.JLinkTraceData * num_items)()
        res = self._dll.JLINKARM_TRACE_Read(buf, int(offset), ctypes.byref(buf_size))
        if (res == 1):
            raise errors.JLinkException('Failed to read from trace buffer.')
        return list(buf)[:int(buf_size.value)]

###############################################################################
#
# Serial Wire Output API
#
###############################################################################

    def swo_enabled(self):
        """Returns whether or not SWO is enabled.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``True`` if SWO is enabled, otherwise ``False``.
        """
        return self._swo_enabled

    @connection_required
    def swo_start(self, swo_speed=9600):
        """Starts collecting SWO data.

        Note:
          If SWO is already enabled, it will first stop SWO before enabling it
          again.

        Args:
          self (JLink): the ``JLink`` instance
          swo_speed (int): the frequency in Hz used by the target to communicate

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        if self.swo_enabled():
            self.swo_stop()

        info = structs.JLinkSWOStartInfo()
        info.Speed = swo_speed
        res = self._dll.JLINKARM_SWO_Control(enums.JLinkSWOCommands.START,
                                             ctypes.byref(info))
        if res < 0:
            raise errors.JLinkException(res)

        self._swo_enabled = True

        return None

    @connection_required
    def swo_stop(self):
        """Stops collecting SWO data.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINKARM_SWO_Control(enums.JLinkSWOCommands.STOP, 0)
        if res < 0:
            raise errors.JLinkException(res)

        return None

    @connection_required
    def swo_enable(self, cpu_speed, swo_speed=9600, port_mask=0x01):
        """Enables SWO output on the target device.

        Configures the output protocol, the SWO output speed, and enables any
        ITM & stimulus ports.

        This is equivalent to calling ``.swo_start()``.

        Note:
          If SWO is already enabled, it will first stop SWO before enabling it
          again.

        Args:
          self (JLink): the ``JLink`` instance
          cpu_speed (int): the target CPU frequency in Hz
          swo_speed (int): the frequency in Hz used by the target to communicate
          port_mask (int): port mask specifying which stimulus ports to enable

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        if self.swo_enabled():
            self.swo_stop()

        res = self._dll.JLINKARM_SWO_EnableTarget(cpu_speed,
                                                  swo_speed,
                                                  enums.JLinkSWOInterfaces.UART,
                                                  port_mask)
        if res != 0:
            raise errors.JLinkException(res)

        self._swo_enabled = True

        return None

    @connection_required
    def swo_disable(self, port_mask):
        """Disables ITM & Stimulus ports.

        Args:
          self (JLink): the ``JLink`` instance
          port_mask (int): mask specifying which ports to disable

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINKARM_SWO_DisableTarget(port_mask)
        if res != 0:
            raise errors.JLinkException(res)
        return None

    @connection_required
    def swo_flush(self, num_bytes=None):
        """Flushes data from the SWO buffer.

        After this method is called, the flushed part of the SWO buffer is
        empty.

        If ``num_bytes`` is not present, flushes all data currently in the SWO
        buffer.

        Args:
          self (JLink): the ``JLink`` instance
          num_bytes (int): the number of bytes to flush

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        if num_bytes is None:
            num_bytes = self.swo_num_bytes()

        buf = ctypes.c_uint32(num_bytes)
        res = self._dll.JLINKARM_SWO_Control(enums.JLinkSWOCommands.FLUSH,
                                             ctypes.byref(buf))
        if res < 0:
            raise errors.JLinkException(res)

        return None

    @connection_required
    def swo_speed_info(self):
        """Retrieves information about the supported SWO speeds.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          A ``JLinkSWOSpeedInfo`` instance describing the target's supported
          SWO speeds.

        Raises:
          JLinkException: on error
        """
        info = structs.JLinkSWOSpeedInfo()
        res = self._dll.JLINKARM_SWO_Control(enums.JLinkSWOCommands.GET_SPEED_INFO,
                                             ctypes.byref(info))
        if res < 0:
            raise errors.JLinkException(res)

        return info

    @connection_required
    def swo_num_bytes(self):
        """Retrives the number of bytes in the SWO buffer.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          Number of bytes in the SWO buffer.

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINKARM_SWO_Control(enums.JLinkSWOCommands.GET_NUM_BYTES,
                                             0)
        if res < 0:
            raise errors.JLinkException(res)

        return res

    @connection_required
    def swo_set_host_buffer_size(self, buf_size):
        """Sets the size of the buffer used by the host to collect SWO data.

        Args:
          self (JLink): the ``JLink`` instance
          buf_size (int): the new size of the host buffer

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        buf = ctypes.c_uint32(buf_size)
        res = self._dll.JLINKARM_SWO_Control(enums.JLinkSWOCommands.SET_BUFFERSIZE_HOST,
                                             ctypes.byref(buf))
        if res < 0:
            raise errors.JLinkException(res)

        return None

    @connection_required
    def swo_set_emu_buffer_size(self, buf_size):
        """Sets the size of the buffer used by the J-Link to collect SWO data.

        Args:
          self (JLink): the ``JLink`` instance
          buf_size (int): the new size of the emulator buffer

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        buf = ctypes.c_uint32(buf_size)
        res = self._dll.JLINKARM_SWO_Control(enums.JLinkSWOCommands.SET_BUFFERSIZE_EMU,
                                             ctypes.byref(buf))
        if res < 0:
            raise errors.JLinkException(res)

        return None

    @connection_required
    def swo_supported_speeds(self, cpu_speed, num_speeds=3):
        """Retrives a list of SWO speeds supported by both the target and the
        connected J-Link.

        The supported speeds are returned in order from highest to lowest.

        Args:
          self (JLink): the ``JLink`` instance
          cpu_speed (int): the target's CPU speed in Hz
          num_speeds (int): the number of compatible speeds to return

        Returns:
          A list of compatible SWO speeds in Hz in order from highest to lowest.
        """
        buf_size = num_speeds
        buf = (ctypes.c_uint32 * buf_size)()
        res = self._dll.JLINKARM_SWO_GetCompatibleSpeeds(cpu_speed, 0, buf, buf_size)
        if res < 0:
            raise errors.JLinkException(res)

        return list(buf)[:res]

    @connection_required
    def swo_read(self, offset, num_bytes, remove=False):
        """Reads data from the SWO buffer.

        The data read is not automatically removed from the SWO buffer after
        reading unless ``remove`` is ``True``.  Otherwise the callee must
        explicitly remove the data by calling ``.swo_flush()``.

        Args:
          self (JLink): the ``JLink`` instance
          offset (int): offset of first byte to be retrieved
          num_bytes (int): number of bytes to read
          remove (bool): if data should be removed from buffer after read

        Returns:
          A list of bytes read from the SWO buffer.
        """
        buf_size = ctypes.c_uint32(num_bytes)
        buf = (ctypes.c_uint8 * num_bytes)(0)

        self._dll.JLINKARM_SWO_Read(buf, offset, ctypes.byref(buf_size))

        # After the call, ``buf_size`` has been modified to be the actual
        # number of bytes that was read.
        buf_size = buf_size.value

        if remove:
            self.swo_flush(buf_size)

        return list(buf)[:buf_size]

    @connection_required
    def swo_read_stimulus(self, port, num_bytes):
        """Reads the printable data via SWO.

        This method reads SWO for one stimulus port, which is all printable
        data.

        Note:
          Stimulus port ``0`` is used for ``printf`` debugging.

        Args:
          self (JLink): the ``JLink`` instance
          port (int): the stimulus port to read from, ``0 - 31``
          num_bytes (int): number of bytes to read

        Returns:
          A list of bytes read via SWO.

        Raises:
          ValueError: if ``port < 0`` or ``port > 31``
        """
        if port < 0 or port > 31:
            raise ValueError('Invalid port number: %s' % port)

        buf_size = num_bytes
        buf = (ctypes.c_uint8 * buf_size)()
        bytes_read = self._dll.JLINKARM_SWO_ReadStimulus(port, buf, buf_size)

        return list(buf)[:bytes_read]

###############################################################################
#
# Real Time Terminal (RTT) API
#
###############################################################################

    @open_required
    def rtt_start(self, block_address=None):
        """Starts RTT processing, including background read of target data.

        Args:
          self (JLink): the ``JLink`` instance
          block_address (int): optional configuration address for the RTT block

        Returns:
          ``None``

        Raises:
          JLinkRTTException: if the underlying JLINK_RTTERMINAL_Control call fails.
        """
        config = None
        if block_address is not None:
            config = structs.JLinkRTTerminalStart()
            config.ConfigBlockAddress = block_address
        self.rtt_control(enums.JLinkRTTCommand.START, config)

    @open_required
    def rtt_stop(self):
        """Stops RTT on the J-Link and host side.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          ``None``

        Raises:
          JLinkRTTException: if the underlying JLINK_RTTERMINAL_Control call fails.
        """
        self.rtt_control(enums.JLinkRTTCommand.STOP, None)

    @open_required
    def rtt_get_buf_descriptor(self, buffer_index, up):
        """After starting RTT, get the descriptor for an RTT control block.

        Args:
          self (JLink): the ``JLink`` instance
          buffer_index (int): the index of the buffer to get.
          up (bool): ``True`` if buffer is an UP buffer, otherwise ``False``.

        Returns:
          ``JLinkRTTerminalBufDesc`` describing the buffer.

        Raises:
          JLinkRTTException: if the RTT control block has not yet been found.
        """
        desc = structs.JLinkRTTerminalBufDesc()
        desc.BufferIndex = buffer_index
        desc.Direction = 0 if up else 1
        self.rtt_control(enums.JLinkRTTCommand.GETDESC, desc)
        return desc

    @open_required
    def rtt_get_num_up_buffers(self):
        """After starting RTT, get the current number of up buffers.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The number of configured up buffers on the target.

        Raises:
          JLinkRTTException: if the underlying JLINK_RTTERMINAL_Control call fails.
        """
        cmd = enums.JLinkRTTCommand.GETNUMBUF
        dir = ctypes.c_int(enums.JLinkRTTDirection.UP)
        return self.rtt_control(cmd, dir)

    @open_required
    def rtt_get_num_down_buffers(self):
        """After starting RTT, get the current number of down buffers.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The number of configured down buffers on the target.

        Raises:
          JLinkRTTException: if the underlying JLINK_RTTERMINAL_Control call fails.
        """
        cmd = enums.JLinkRTTCommand.GETNUMBUF
        dir = ctypes.c_int(enums.JLinkRTTDirection.DOWN)
        return self.rtt_control(cmd, dir)

    @open_required
    def rtt_get_status(self):
        """After starting RTT, get the status.

        Args:
          self (JLink): the ``JLink`` instance

        Returns:
          The status of RTT.

        Raises:
          JLinkRTTException: on error.
        """
        status = structs.JLinkRTTerminalStatus()
        res = self.rtt_control(enums.JLinkRTTCommand.GETSTAT, status)
        return status

    @open_required
    def rtt_read(self, buffer_index, num_bytes):
        """Reads data from the RTT buffer.

        This method will read at most num_bytes bytes from the specified
        RTT buffer. The data is automatically removed from the RTT buffer.
        If there are not num_bytes bytes waiting in the RTT buffer, the
        entire contents of the RTT buffer will be read.

        Args:
          self (JLink): the ``JLink`` instance
          buffer_index (int): the index of the RTT buffer to read from
          num_bytes (int): the maximum number of bytes to read

        Returns:
          A list of bytes read from RTT.

        Raises:
          JLinkRTTException: if the underlying JLINK_RTTERMINAL_Read call fails.
        """
        buf = (ctypes.c_ubyte * num_bytes)()
        bytes_read = self._dll.JLINK_RTTERMINAL_Read(buffer_index, buf, num_bytes)

        if bytes_read < 0:
            raise errors.JLinkRTTException(bytes_read)

        return list(buf[:bytes_read])

    @open_required
    def rtt_write(self, buffer_index, data):
        """Writes data to the RTT buffer.

        This method will write at most len(data) bytes to the specified RTT
        buffer.

        Args:
          self (JLink): the ``JLink`` instance
          buffer_index (int): the index of the RTT buffer to write to
          data (list): the list of bytes to write to the RTT buffer

        Returns:
          The number of bytes successfully written to the RTT buffer.

        Raises:
          JLinkRTTException: if the underlying JLINK_RTTERMINAL_Write call fails.
        """
        buf_size = len(data)
        buf = (ctypes.c_ubyte * buf_size)(*bytearray(data))
        bytes_written = self._dll.JLINK_RTTERMINAL_Write(buffer_index, buf, buf_size)

        if bytes_written < 0:
            raise errors.JLinkRTTException(bytes_written)

        return bytes_written

    @open_required
    def rtt_control(self, command, config):
        """Issues an RTT Control command.

        All RTT control is done through a single API call which expects
        specifically laid-out configuration structures.

        Args:
          self (JLink): the ``JLink`` instance
          command (int): the command to issue (see enums.JLinkRTTCommand)
          config (ctypes type): the configuration to pass by reference.

        Returns:
          An integer containing the result of the command.

        Raises:
          JLinkRTTException: on error.
        """
        config_byref = ctypes.byref(config) if config is not None else None
        res = self._dll.JLINK_RTTERMINAL_Control(command, config_byref)

        if res < 0:
            raise errors.JLinkRTTException(res)

        return res

###############################################################################
#
# System control Co-Processor (CP15) API
#
###############################################################################

    @connection_required
    def cp15_present(self):
        """Returns whether target has CP15 co-processor.

        Returns:
            ``True`` if the target has CP15 co-processor, otherwise ``False``.
        """

        result = False
        if self._dll.JLINKARM_CP15_IsPresent() != 0:
            result = True
        return result

    @open_required
    def cp15_register_read(self, cr_n, op_1, cr_m, op_2):
        """Reads value from specified coprocessor register.

        Args:
          cr_n (int): CRn value
          op_1 (int): Op1 value
          cr_m (int): CRm value
          op_2 (int): Op2 value

        Returns:
          An integer containing the value of coprocessor register

        Raises:
          JLinkException: on error
        """
        value = ctypes.c_uint32(0)
        p_value = ctypes.pointer(value)
        res = self._dll.JLINKARM_CP15_ReadEx(cr_n, cr_m, op_1, op_2, p_value)
        if res != 0:
            raise errors.JLinkException(res)
        else:
            value = value.value
        return value

    @open_required
    def cp15_register_write(self, cr_n, op_1, cr_m, op_2, value):
        """Writes value to specified coprocessor register.

        Args:
          cr_n (int): CRn value
          op_1 (int): Op1 value
          cr_m (int): CRm value
          op_2 (int): Op2 value
          value (int): value to write

        Returns:
          An integer containing the result of the command

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINKARM_CP15_WriteEx(cr_n, cr_m, op_1, op_2, value)
        if res != 0:
            raise errors.JLinkException(res)
        return res

###############################################################################
#
# Power API
#
###############################################################################
    @open_required
    def power_trace_configure(self, channels, freq, ref, always):
        """Configures power tracing.

        This method must be called before calling the other power trace APIs. It
        is the responsibility of the calling application code to keep track of
        which channels were enabled in order to determine which trace samples
        correspond to which channels when read.

        Args:
          self (JLink): the ``JLink`` instance.
          channels (list[int]): list specifying which channels to capture on (0 - 7).
          freq (int): sampling frequency (in Hertz).
          ref (JLinkPowerTraceRef): reference value to stored on capture.
          always (bool): ``True`` to capture data even while CPU halted, otherwise ``False``.

        Returns:
          The sampling frequency (in Hz) for power sampling.

        Raises:
          JLinkException: on error
          ValueError: invalid channels specified
        """
        if isinstance(channels, list):
            channel_mask = 0x00
            for channel in channels:
                channel_mask |= (1 << channel)
        else:
            channel_mask = channels

        if channel_mask > 0xFF:
            raise ValueError("Channels must be in range 0 - 7")

        setup = structs.JLinkPowerTraceSetup()
        setup.ChannelMask = channel_mask
        setup.SampleFreq = int(freq)
        setup.RefSelect = int(ref)
        setup.EnableCond = 0 if always else 1

        res = self._dll.JLINK_POWERTRACE_Control(enums.JLinkPowerTraceCommand.SETUP, ctypes.byref(setup), 0)
        if res < 0:
            raise errors.JLinkException(res)
        return res

    @open_required
    def power_trace_start(self):
        """Starts capturing data on the channels enabled via ``power_trace_configure()``.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINK_POWERTRACE_Control(enums.JLinkPowerTraceCommand.START, 0, 0)
        if res < 0:
            raise errors.JLinkException(res)

    @open_required
    def power_trace_stop(self):
        """Stops a capture started by ``power_trace_start()``.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINK_POWERTRACE_Control(enums.JLinkPowerTraceCommand.STOP, 0, 0)
        if res < 0:
            raise errors.JLinkException(res)

    @open_required
    def power_trace_flush(self):
        """Flushes all capture data.

        Any data that has not been read by ``power_trace_read()`` is dropped.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          ``None``

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINK_POWERTRACE_Control(enums.JLinkPowerTraceCommand.FLUSH, 0, 0)
        if res < 0:
            raise errors.JLinkException(res)

    @open_required
    def power_trace_get_channels(self):
        """Returns a list of the available channels for power tracing.

        This method returns a list of the available channels for power tracing.
        The application code can use this to determine which channels to
        enable for tracing.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          List of available channel identifiers.

        Raises:
          JLinkException: on error
        """
        caps = structs.JLinkPowerTraceCaps()
        res = self._dll.JLINK_POWERTRACE_Control(enums.JLinkPowerTraceCommand.GET_CAPS, 0, ctypes.byref(caps))
        if res < 0:
            raise errors.JLinkException(res)

        return [i for i in range(0, 32) if (caps.ChannelMask >> i) & 0x1]

    @open_required
    def power_trace_get_channel_capabilities(self, channels):
        """Returns the capabilities for the specified channels.

        Args:
          self (JLink): the ``JLink`` instance.
          channels (list[int]): list specifying which channels to get capabilities for.

        Returns:
          Channel capabilities.

        Raises:
          JLinkException: on error
          ValueError: invalid channels specified
        """
        if isinstance(channels, list):
            channel_mask = 0x00
            for channel in channels:
                channel_mask |= (1 << channel)
        else:
            channel_mask = channels

        if channel_mask > 0xFF:
            raise ValueError("Channels must be in range 0 - 7")

        channel_caps = structs.JLinkPowerTraceChannelCaps()
        caps = structs.JLinkPowerTraceCaps()
        caps.ChannelMask = channel_mask

        res = self._dll.JLINK_POWERTRACE_Control(enums.JLinkPowerTraceCommand.GET_CHANNEL_CAPS,
                                                 ctypes.byref(caps),
                                                 ctypes.byref(channel_caps))
        if res < 0:
            raise errors.JLinkException(res)

        return channel_caps

    @open_required
    def power_trace_get_num_items(self):
        """Returns a count of the number of items in the power trace buffer.

        Since each channel is sampled simulataneously, the count of number of
        items per channel is the return value of this function divided by the
        number of active channels.

        Args:
          self (JLink): the ``JLink`` instance.

        Returns:
          Number of items in the power trace buffer.

        Raises:
          JLinkException: on error
        """
        res = self._dll.JLINK_POWERTRACE_Control(enums.JLinkPowerTraceCommand.GET_NUM_ITEMS, 0, 0)
        if res < 0:
            raise errors.JLinkException(res)
        return res

    @open_required
    def power_trace_read(self, num_items=None):
        """Reads data from the power trace buffer.

        Any read data is flushed from the power trace buffer.

        Args:
          self (JLink): the ``JLink`` instance.
          num_items (int): the number of items to read (if not specified, reads all).

        Returns:
          List of ``JLinkPowerTraceItem``s.

        Raises:
          JLinkException: on error
        """
        if num_items is None:
            num_items = self.power_trace_get_num_items()

        items = []
        if num_items < 0:
            raise ValueError("Invalid number of items requested, expected > 0, given %d" % num_items)
        elif num_items > 0:
            items = (structs.JLinkPowerTraceItem * num_items)()
            res = self._dll.JLINK_POWERTRACE_Read(ctypes.byref(items), num_items)
            if res < 0:
                raise errors.JLinkException(res)

            # Number of items may be less than the requested count, so clip the
            # array here.
            items = list(items)[:res]
        return items
