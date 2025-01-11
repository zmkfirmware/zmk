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

from . import enums

import ctypes


class JLinkConnectInfo(ctypes.Structure):
    """J-Link connection info structure.

    Attributes:
      SerialNumber: J-Link serial number.
      Connection: type of connection (e.g. ``enums.JLinkHost.USB``)
      USBAddr: USB address if connected via USB.
      aIPAddr: IP address if connected via IP.
      Time: Time period (ms) after which UDP discover answer was received.
      Time_us: Time period (uS) after which UDP discover answer was received.
      HWVersion: Hardware version of J-Link, if connected via IP.
      abMACAddr: MAC Address, if connected via IP.
      acProduct: Product name, if connected via IP.
      acNickname: Nickname, if connected via IP.
      acFWString: Firmware string, if connected via IP.
      IsDHCPAssignedIP: Is IP address reception via DHCP.
      IsDHCPAssignedIPIsValid: True if connected via IP.
      NumIPConnections: Number of IP connections currently established.
      NumIPConnectionsIsValid: True if connected via IP.
      aPadding: Bytes reserved for future use.
    """
    _fields_ = [
        ('SerialNumber', ctypes.c_uint32),
        ('Connection', ctypes.c_ubyte),
        ('USBAddr', ctypes.c_uint32),
        ('aIPAddr', ctypes.c_uint8 * 16),
        ('Time', ctypes.c_int),
        ('Time_us', ctypes.c_uint64),
        ('HWVersion', ctypes.c_uint32),
        ('abMACAddr', ctypes.c_uint8 * 6),
        ('acProduct', ctypes.c_char * 32),
        ('acNickname', ctypes.c_char * 32),
        ('acFWString', ctypes.c_char * 112),
        ('IsDHCPAssignedIP', ctypes.c_char),
        ('IsDHCPAssignedIPIsValid', ctypes.c_char),
        ('NumIPConnections', ctypes.c_char),
        ('NumIPConnectionsIsValid', ctypes.c_char),
        ('aPadding', ctypes.c_uint8 * 34)
    ]

    def __repr__(self):
        """Returns a representation of this class.

        Args:
          self (JLinkConnectInfo): the ``JlinkConnectInfo`` instance

        Returns:
          String representation of the class.
        """
        return 'JLinkConnectInfo(%s)' % self.__str__()

    def __str__(self):
        """Returns a string representation of the connection info.

        Args:
          self (JLinkConnectInfo): the ``JLinkConnectInfo`` instance

        Returns:
          String specifying the product, its serial number, and the type of
          connection that it has (one of USB or IP).
        """
        conn = 'USB' if self.Connection == 1 else 'IP'
        return '%s <Serial No. %s, Conn. %s>' % (self.acProduct.decode(), self.SerialNumber, conn)


class JLinkFlashArea(ctypes.Structure):
    """Definition for a region of Flash.

    Attributes:
      Addr: address where the flash area starts.
      Size: size of the flash area.
    """

    _fields_ = [
        ('Addr', ctypes.c_uint32),
        ('Size', ctypes.c_uint32)
    ]

    def __repr__(self):
        """Returns a representation of the instance.

        Args:
          self (FlashArea): the ``FlashArea`` instance

        Returns:
          String representation of the Flash Area.
        """
        return '%s(%s)' % (self.__class__.__name__, self.__str__())

    def __str__(self):
        """Returns a string representation of the instance.

        Args:
          self (FlashArea): the ``FlashArea`` instance

        Returns:
          String specifying address of flash region, and its size.
        """
        return 'Address = 0x%x, Size = %s' % (self.Addr, self.Size)


class JLinkRAMArea(JLinkFlashArea):
    """Definition for a region of RAM.

    Attributes:
      Addr: address where the flash area starts.
      Size: size of the flash area.
    """
    pass


class JLinkDeviceInfo(ctypes.Structure):
    """J-Link device information.

    This structure is used to represent a device that is supported by the
    J-Link.

    Attributes:
      SizeOfStruct: Size of the struct (DO NOT CHANGE).
      sName: name of the device.
      CoreId: core identifier of the device.
      FlashAddr: base address of the internal flash of the device.
      RAMAddr: base address of the internal RAM of the device.
      EndianMode: the endian mode of the device (0 -> only little endian,
          1 -> only big endian, 2 -> both).
      FlashSize: total flash size in bytes.
      RAMSize: total RAM size in bytes.
      sManu: device manufacturer.
      aFlashArea: a list of ``JLinkFlashArea`` instances.
      aRamArea: a list of ``JLinkRAMArea`` instances.
      Core: CPU core.
    """
    _fields_ = [
        ('SizeofStruct', ctypes.c_uint32),
        ('sName', ctypes.POINTER(ctypes.c_char)),
        ('CoreId', ctypes.c_uint32),
        ('FlashAddr', ctypes.c_uint32),
        ('RAMAddr', ctypes.c_uint32),
        ('EndianMode', ctypes.c_char),
        ('FlashSize', ctypes.c_uint32),
        ('RAMSize', ctypes.c_uint32),
        ('sManu', ctypes.POINTER(ctypes.c_char)),
        ('aFlashArea', JLinkFlashArea * 32),
        ('aRAMArea', JLinkRAMArea * 32),
        ('Core', ctypes.c_uint32)
    ]

    def __init__(self, *args, **kwargs):
        """Initializes the instance.

        Populates the ``.SizeofStruct`` parameter to the size of the instance.

        Args:
          self (JLinkDeviceInfo): the ``JLinkDeviceInfo`` instance
          args: list of arguments
          kwargs: key-word arguments dictionary

        Returns:
          ``None``
        """
        super(JLinkDeviceInfo, self).__init__(*args, **kwargs)
        self.SizeofStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a representation of this instance.

        Args:
          self (JLinkDeviceInfo): the ``JLinkDeviceInfo`` instance

        Returns:
          Returns a string representation of the instance.
        """
        return 'JLinkDeviceInfo(%s)' % self.__str__()

    def __str__(self):
        """Returns a string representation of this instance.

        Args:
          self (JLinkDeviceInfo): the ``JLinkDeviceInfo`` instance

        Returns:
          Returns a string specifying the device name, core, and manufacturer.
        """
        manu = self.manufacturer
        return '%s <Core Id. %s, Manu. %s>' % (self.name, self.Core, manu)

    @property
    def name(self):
        """Returns the name of the device.

        Args:
          self (JLinkDeviceInfo): the ``JLinkDeviceInfo`` instance

        Returns:
          Device name.
        """
        return ctypes.cast(self.sName, ctypes.c_char_p).value.decode()

    @property
    def manufacturer(self):
        """Returns the name of the manufacturer of the device.

        Args:
          self (JLinkDeviceInfo): the ``JLinkDeviceInfo`` instance

        Returns:
          Manufacturer name.
        """
        buf = ctypes.cast(self.sManu, ctypes.c_char_p).value
        return buf.decode() if buf else None


class JLinkHardwareStatus(ctypes.Structure):
    """Definition for the hardware status information for a J-Link.

    Attributes:
      VTarget: target supply voltage.
      tck: measured state of TCK pin.
      tdi: measured state of TDI pin.
      tdo: measured state of TDO pin.
      tms: measured state of TMS pin.
      tres: measured state of TRES pin.
      trst: measured state of TRST pin.
    """
    _fields_ = [
        ('VTarget', ctypes.c_uint16),
        ('tck', ctypes.c_uint8),
        ('tdi', ctypes.c_uint8),
        ('tdo', ctypes.c_uint8),
        ('tms', ctypes.c_uint8),
        ('tres', ctypes.c_uint8),
        ('trst', ctypes.c_uint8)
    ]

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkHardwareStatus): the ``JlinkHardwareStatus`` instance

        Returns:
          String representation of the instance.
        """
        return '%s(VTarget=%dmV)' % (self.__class__.__name__, self.voltage)

    @property
    def voltage(self):
        """Returns the target supply voltage.

        This is an alias for ``.VTarget``.

        Args:
          self (JLInkHardwareStatus): the ``JLinkHardwareStatus`` instance

        Returns:
          Target supply voltage as an integer.
        """
        return self.VTarget


class JLinkGPIODescriptor(ctypes.Structure):
    """Definition for the structure that details the name and capabilities of a
    user-controllable GPIO.

    Attributes:
      acName: name of the GPIO.
      Caps: bitfield of capabilities.
    """
    _fields_ = [
        ('acName', ctypes.c_char * 32),
        ('Caps', ctypes.c_uint32)
    ]

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkGPIODescriptor): the ``JLinkGPIODescriptor`` instance

        Returns:
          String representation of the instance.
        """
        return '%s(%s)' % (self.__class__.__name__, self.__str__())

    def __str__(self):
        """Returns the GPIO name.

        Args:
          self (JLinkGPIODescriptor): the ``JLInkGPIODescriptor`` instance

        Returns:
          GPIO name.
        """
        return self.acName.decode()


class JLinkMemoryZone(ctypes.Structure):
    """Represents a CPU memory zone.

    Attributes:
      sName: initials of the memory zone.
      sDesc: name of the memory zone.
      VirtAddr: start address of the virtual address space of the memory zone.
      abDummy: reserved for future use.
    """
    _fields_ = [
        ('sName', ctypes.c_char_p),
        ('sDesc', ctypes.c_char_p),
        ('VirtAddr', ctypes.c_uint64),
        ('abDummy', (ctypes.c_uint8 * 16))
    ]

    def __repr__(self):
        """Returns a string representation of the instance

        Args:
          self: the ``JLinkMemoryZone`` instance

        Returns:
          String representation of the instance.
        """
        return '%s(%s)' % (self.__class__.__name__, self.__str__())

    def __str__(self):
        """Returns a formatted string describing the memory zone.

        Args:
          self: the ``JLinkMemoryZone`` instance

        Returns:
          String representation of the memory zone.
        """
        return '%s <Desc. %s, VirtAddr. 0x%x>' % (self.sName, self.sDesc, self.VirtAddr)

    @property
    def name(self):
        """Alias for the memory zone name.

        Args:
          self (JLinkMemoryZone): the ``JLinkMemoryZone`` instance

        Returns:
          The memory zone name.
        """
        return self.sName


class JLinkSpeedInfo(ctypes.Structure):
    """Represents information about an emulator's supported speeds.

    The emulator can support all target interface speeds calculated by dividing
    the base frequency by atleast ``MinDiv``.

    Attributes:
      SizeOfStruct: the size of this structure.
      BaseFreq: Base frequency (in HZ) used to calculate supported speeds.
      MinDiv: minimum divider allowed to divide the base frequency.
      SupportAdaptive: ``1`` if emulator supports adaptive clocking, otherwise
          ``0``.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_uint32),
        ('BaseFreq', ctypes.c_uint32),
        ('MinDiv', ctypes.c_uint16),
        ('SupportAdaptive', ctypes.c_uint16)
    ]

    def __init__(self):
        """Initializes the ``JLinkSpeedInfo`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkSpeedInfo): the ``JLinkSpeedInfo`` instance

        Returns:
          ``None``
        """
        super(JLinkSpeedInfo, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkSpeedInfo): the ``JLinkSpeedInfo`` instance

        Returns:
          String representation of the instance.
        """
        return self.__str__()

    def __str__(self):
        """Returns this instance formatted as a string.

        Args:
          self (JLinkSpeedInfo): the ``JLinkSpeedInfo`` instance

        Returns:
          String formatted instance.
        """
        return '%s(Freq=%sHz)' % (self.__class__.__name__, self.BaseFreq)


class JLinkSWOStartInfo(ctypes.Structure):
    """Represents configuration information for collecting Serial Wire Output
    (SWO) information.

    Attributes:
      SizeofStruct: size of the structure.
      Interface: the interface type used for SWO.
      Speed: the frequency used for SWO communication in Hz.

    Note:
      You should *never* change ``.SizeofStruct`` or ``.Interface``.
    """
    _fields_ = [
        ('SizeofStruct', ctypes.c_uint32),
        ('Interface', ctypes.c_uint32),
        ('Speed', ctypes.c_uint32)
    ]

    def __init__(self):
        """Initializes the SWO start information.

        Args:
          self (JLinkSWOStartInfo): the ``JLinkSWOStartInfo`` instance

        Returns:
          ``None``
        """
        super(JLinkSWOStartInfo, self).__init__()
        self.SizeofStruct = ctypes.sizeof(self)
        self.Interface = enums.JLinkSWOInterfaces.UART

    def __repr__(self):
        """Returns a representation of this instance.

        Args:
          self (JLinkSWOStartInfo): the ``JLinkSWOStartInfo`` instance

        Returns:
          The string representation of this instance.
        """
        return self.__str__()

    def __str__(self):
        """Returns a string representation of this instance.

        Args:
          self (JLinkSWOStartInfo): the ``JLinkSWOStartInfo`` instance

        Returns:
          The string representation of this instance.
        """
        return '%s(Speed=%sHz)' % (self.__class__.__name__, self.Speed)


class JLinkSWOSpeedInfo(ctypes.Structure):
    """Structure representing information about target's supported SWO speeds.

    To calculate the supported SWO speeds, the base frequency is taken and
    divide by a number in the range of ``[ MinDiv, MaxDiv ]``.

    Attributes:
      SizeofStruct: size of the structure.
      Interface: interface type for the speed information.
      BaseFreq: base frequency (Hz) used to calculate supported SWO speeds.
      MinDiv: minimum divider allowed to divide the base frequency.
      MaxDiv: maximum divider allowed to divide the base frequency.
      MinPrescale: minimum prescaler allowed to adjust the base frequency.
      MaxPrescale: maximum prescaler allowed to adjust the base frequency.

    Note:
      You should *never* change ``.SizeofStruct`` or ``.Interface``.
    """
    _fields_ = [
        ('SizeofStruct', ctypes.c_uint32),
        ('Interface', ctypes.c_uint32),
        ('BaseFreq', ctypes.c_uint32),
        ('MinDiv', ctypes.c_uint32),
        ('MaxDiv', ctypes.c_uint32),
        ('MinPrescale', ctypes.c_uint32),
        ('MaxPrescale', ctypes.c_uint32)
    ]

    def __init__(self):
        """Initializes the J-Link SWO Speed Information instance.

        Args:
          self (JLinkSWOSpeedInfo): the ``JLinkSWOSpeedInfo`` instance

        Returns:
          ``None``
        """
        super(JLinkSWOSpeedInfo, self).__init__()
        self.SizeofStruct = ctypes.sizeof(self)
        self.Interface = enums.JLinkSWOInterfaces.UART

    def __repr__(self):
        """Returns a representation of the instance.

        Args:
          self (JLinkSWOSpeedInfo): the ``JLinkSWOSpeedInfo`` instance

        Returns:
          ``None``
        """
        return self.__str__()

    def __str__(self):
        """Returns a string representaton of the instance.

        Args:
          self (JLinkSWOSpeedInfo): the ``JLinkSWOSpeedInfo`` instance

        Returns:
          ``None``
        """
        return '%s(Interface=UART, Freq=%sHz)' % (self.__class__.__name__, self.BaseFreq)


class JLinkMOEInfo(ctypes.Structure):
    """Structure representing the Method of Debug Entry (MOE).

    The method of debug entry is a reason for which a CPU has stopped.  At any
    given time, there may be multiple methods of debug entry.

    Attributes:
      HaltReason: reason why the CPU stopped.
      Index: if cause of CPU stop was a code/data breakpoint, this identifies
        the index of the code/data breakpoint unit which causes the CPU to
        stop, otherwise it is ``-1``.
    """
    _fields_ = [
        ('HaltReason', ctypes.c_uint32),
        ('Index', ctypes.c_int)
    ]

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkMOEInfo): the ``JLinkMOEInfo`` instance

        Returns:
          A string representation of the instance.
        """
        return '%s(%s)' % (self.__class__.__name__, self.__str__())

    def __str__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkMOEInfo): the ``JLinkMOEInfo`` instance

        Returns:
          A string representation of the instance.
        """
        d = enums.JLinkHaltReasons.__dict__
        s = next(k for k, v in d.items() if v == self.HaltReason)
        if self.dbgrq():
            return s
        return s.replace('_', ' ').title()

    def dbgrq(self):
        """Returns whether this a DBGRQ.

        Args:
          self (JLinkMOEInfo): the ``JLinkMOEInfo`` instance

        Returns:
          ``True`` if this is a DBGRQ, otherwise ``False``.
        """
        return self.HaltReason == enums.JLinkHaltReasons.DBGRQ

    def code_breakpoint(self):
        """Returns whether this a code breakpoint.

        Args:
          self (JLinkMOEInfo): the ``JLinkMOEInfo`` instance

        Returns:
          ``True`` if this is a code breakpoint, otherwise ``False``.
        """
        return self.HaltReason == enums.JLinkHaltReasons.CODE_BREAKPOINT

    def data_breakpoint(self):
        """Returns whether this a data breakpoint.

        Args:
          self (JLinkMOEInfo): the ``JLinkMOEInfo`` instance

        Returns:
          ``True`` if this is a data breakpoint, otherwise ``False``.
        """
        return self.HaltReason == enums.JLinkHaltReasons.DATA_BREAKPOINT

    def vector_catch(self):
        """Returns whether this a vector catch.

        Args:
          self (JLinkMOEInfo): the ``JLinkMOEInfo`` instance

        Returns:
          ``True`` if this is a vector catch, otherwise ``False``.
        """
        return self.HaltReason == enums.JLinkHaltReasons.VECTOR_CATCH


class JLinkBreakpointInfo(ctypes.Structure):
    """Class representing information about a breakpoint.

    Attributes:
      SizeOfStruct: the size of the structure (this should not be modified).
      Handle: breakpoint handle.
      Addr: address of where the breakpoint has been set.
      Type: type flags which were specified when the breakpoint was created.
      ImpFlags: describes the current state of the breakpoint.
      UseCnt: describes how often the breakpoint is set at the same address.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_uint32),
        ('Handle', ctypes.c_uint32),
        ('Addr', ctypes.c_uint32),
        ('Type', ctypes.c_uint32),
        ('ImpFlags', ctypes.c_uint32),
        ('UseCnt', ctypes.c_uint32)
    ]

    def __init__(self):
        """Initializes the ``JLinkBreakpointInfo`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkBreakpointInfo): the ``JLinkBreakpointInfo`` instnace

        Returns:
          ``None``
        """
        super(JLinkBreakpointInfo, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a formatted string describing the breakpoint.

        Args:
          self (JLinkBreakpointInfo): the ``JLinkBreakpointInfo`` instance

        Returns:
          Stirng representation of the breakpoint.
        """
        return self.__str__()

    def __str__(self):
        """Returns a formatted string describing the breakpoint.

        Args:
          self (JLinkBreakpointInfo): the ``JLinkBreakpointInfo`` instance

        Returns:
          Stirng representation of the breakpoint.
        """
        name = self.__class__.__name__
        return '%s(Handle %d, Address %d)' % (name, self.Handle, self.Addr)

    def software_breakpoint(self):
        """Returns whether this is a software breakpoint.

        Args:
          self (JLinkBreakpointInfo): the ``JLinkBreakpointInfo`` instance

        Returns:
          ``True`` if the breakpoint is a software breakpoint, otherwise
          ``False``.
        """
        software_types = [
            enums.JLinkBreakpoint.SW_RAM,
            enums.JLinkBreakpoint.SW_FLASH,
            enums.JLinkBreakpoint.SW
        ]
        return any(self.Type & stype for stype in software_types)

    def hardware_breakpoint(self):
        """Returns whether this is a hardware breakpoint.

        Args:
          self (JLinkBreakpointInfo): the ``JLinkBreakpointInfo`` instance

        Returns:
          ``True`` if the breakpoint is a hardware breakpoint, otherwise
          ``False``.
        """
        return self.Type & enums.JLinkBreakpoint.HW

    def pending(self):
        """Returns if this breakpoint is pending.

        Args:
          self (JLinkBreakpointInfo): the ``JLinkBreakpointInfo`` instance

        Returns:
          ``True`` if the breakpoint is still pending, otherwise ``False``.
        """
        return self.ImpFlags & enums.JLinkBreakpointImplementation.PENDING


class JLinkDataEvent(ctypes.Structure):
    """Class representing a data event.

    A data may halt the CPU, trigger SWO output, or trigger trace output.

    Attributes:
      SizeOfStruct: the size of the structure (this should not be modified).
      Type: the type of the data event (this should not be modified).
      Addr: the address on which the watchpoint was set
      AddrMask: the address mask used for comparision.
      Data: the data on which the watchpoint has been set.
      DataMask: the data mask used for comparision.
      Access: the control data on which the event has been set.
      AccessMask: the control mask used for comparison.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_int),
        ('Type', ctypes.c_int),
        ('Addr', ctypes.c_uint32),
        ('AddrMask', ctypes.c_uint32),
        ('Data', ctypes.c_uint32),
        ('DataMask', ctypes.c_uint32),
        ('Access', ctypes.c_uint8),
        ('AccessMask', ctypes.c_uint8)
    ]

    def __init__(self):
        """Initializes the ``JLinkDataEvent`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkDataEvent): the ``JLinkDataEvent`` instance

        Returns:
          ``None``
        """
        super(JLinkDataEvent, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)
        self.Type = enums.JLinkEventTypes.BREAKPOINT

    def __repr__(self):
        """Returns a string representation of the data event.

        Args:
          self (JLinkDataEvent): the ``JLinkDataEvent`` instance

        Returns:
          A string representation of the data event.
        """
        return self.__str__()

    def __str__(self):
        """Returns a string representation of the data event.

        Args:
          self (JLinkDataEvent): the ``JLinkDataEvent`` instance

        Returns:
          A string representation of the data event.
        """
        name = self.__class__.__name__
        return '%s(Type %d, Address %d)' % (name, self.Type, self.Addr)


class JLinkWatchpointInfo(ctypes.Structure):
    """Class representing information about a watchpoint.

    Attributes:
      SizeOfStruct: the size of the structure (this should not be modified).
      Handle: the watchpoint handle.
      Addr: the address the watchpoint was set at.
      AddrMask: the address mask used for comparison.
      Data: the data on which the watchpoint was set.
      DataMask: the data mask used for comparision.
      Ctrl: the control data on which the breakpoint was set.
      CtrlMask: the control mask used for comparison.
      WPUnit: the index of the watchpoint unit.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_uint32),
        ('Handle', ctypes.c_uint32),
        ('Addr', ctypes.c_uint32),
        ('AddrMask', ctypes.c_uint32),
        ('Data', ctypes.c_uint32),
        ('DataMask', ctypes.c_uint32),
        ('Ctrl', ctypes.c_uint32),
        ('CtrlMask', ctypes.c_uint32),
        ('WPUnit', ctypes.c_uint8)
    ]

    def __init__(self):
        """Initializes the ``JLinkWatchpointInfo`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkWatchpointInfo): the ``JLinkWatchpointInfo`` instance

        Returns:
          ``None``
        """
        super(JLinkWatchpointInfo, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a formatted string describing the watchpoint.

        Args:
          self (JLinkWatchpointInfo): the ``JLinkWatchpointInfo`` instance

        Returns:
          String representation of the watchpoint.
        """
        return self.__str__()

    def __str__(self):
        """Returns a formatted string describing the watchpoint.

        Args:
          self (JLinkWatchpointInfo): the ``JLinkWatchpointInfo`` instance

        Returns:
          String representation of the watchpoint.
        """
        name = self.__class__.__name__
        return '%s(Handle %d, Address %d)' % (name, self.Handle, self.Addr)


class JLinkStraceEventInfo(ctypes.Structure):
    """Class representing the STRACE event information.

    Attributes:
      SizeOfStruct: size of the structure.
      Type: type of event.
      Op: the STRACE operation to perform.
      AccessSize: access width for trace events.
      Reserved0: reserved.
      Addr: specifies the load/store address for data.
      Data: the data to be compared for the operation for data access events.
      DataMask: bitmask for bits of data to omit in comparision for data access
        events.
      AddrRangeSize: address range for range events.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_uint32),
        ('Type', ctypes.c_uint8),
        ('Op', ctypes.c_uint8),
        ('AccessSize', ctypes.c_uint8),
        ('Reserved0', ctypes.c_uint8),
        ('Addr', ctypes.c_uint64),
        ('Data', ctypes.c_uint64),
        ('DataMask', ctypes.c_uint64),
        ('AddrRangeSize', ctypes.c_uint32)
    ]

    def __init__(self):
        """Initializes the ``JLinkStraceEventInfo`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkStraceEventInfo): the ``JLinkStraceEventInfo`` instance

        Returns:
          ``None``
        """
        super(JLinkStraceEventInfo, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a formatted string describing the event info.

        Args:
          self (JLinkStraceEventInfo): the ``JLinkStraceEventInfo`` instance

        Returns:
          String representation of the event info.
        """
        return self.__str__()

    def __str__(self):
        """Returns a formatted string describing the event info.

        Args:
          self (JLinkStraceEventInfo): the ``JLinkStraceEventInfo`` instance

        Returns:
          String representation of the event information.
        """
        name = self.__class__.__name__
        return '%s(Type=%d, Op=%d)' % (name, self.Type, self.Op)


class JLinkTraceData(ctypes.Structure):
    """Structure representing trace data returned by the trace buffer.

    Attributes:
      PipeStat: type of trace data.
      Sync: sync point in buffer.
      Packet: trace data packet.
    """
    _fields_ = [
        ('PipeStat', ctypes.c_uint8),
        ('Sync', ctypes.c_uint8),
        ('Packet', ctypes.c_uint16)
    ]

    def __repr__(self):
        """Returns a string representation of the trace data instance.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          A string representation of the instance.
        """
        return self.__str__()

    def __str__(self):
        """Returns a string representation of the trace data instance.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          A string representation of the instance.
        """
        return '%s(%d)' % (self.__class__.__name__, self.Packet)

    def instruction(self):
        """Returns whether the data corresponds to an executed instruction.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for an executed instruction.
        """
        return (self.PipeStat == 0)

    def data_instruction(self):
        """Returns whether the data corresponds to an data instruction.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for an data instruction.
        """
        return (self.PipeStat == 1)

    def non_instruction(self):
        """Returns whether the data corresponds to an un-executed instruction.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for an un-executed instruction.
        """
        return (self.PipeStat == 2)

    def wait(self):
        """Returns whether the data corresponds to a wait.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for a wait.
        """
        return (self.PipeStat == 3)

    def branch(self):
        """Returns whether the data corresponds to a branch execution.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for a branch execution.
        """
        return (self.PipeStat == 4)

    def data_branch(self):
        """Returns whether the data corresponds to a branch with data.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for a branch with data.
        """
        return (self.PipeStat == 5)

    def trigger(self):
        """Returns whether the data corresponds to a trigger event.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for a trigger event.
        """
        return (self.PipeStat == 6)

    def trace_disabled(self):
        """Returns whether the data corresponds to trace being disabled.

        Args:
          self (JLinkTraceData): the ``JLinkTraceData`` instance.

        Returns:
          ``True`` if this is trace data for the trace disabled event.
        """
        return (self.PipeStat == 7)


class JLinkTraceRegion(ctypes.Structure):
    """Structure describing a trace region.

    Attributes:
      SizeOfStruct: size of the structure.
      RegionIndex: index of the region.
      NumSamples: number of samples in the region.
      Off: offset in the trace buffer.
      RegionCnt: number of trace regions.
      Dummy: unused.
      Timestamp: timestamp of last event written to buffer.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_uint32),
        ('RegionIndex', ctypes.c_uint32),
        ('NumSamples', ctypes.c_uint32),
        ('Off', ctypes.c_uint32),
        ('RegionCnt', ctypes.c_uint32),
        ('Dummy', ctypes.c_uint32),
        ('Timestamp', ctypes.c_uint64)
    ]

    def __init__(self):
        """Initializes the trace region.

        Sets the size of the structure.

        Args:
          self (JLinkTraceRegion): the ``JLinkTraceRegion`` instance.

        Returns:
          ``None``
        """
        super(JLinkTraceRegion, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkTraceRegion): the ``JLinkTraceRegion`` instance.

        Returns:
          String representation of the trace region.
        """
        return self.__str__()

    def __str__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkTraceRegion): the ``JLinkTraceRegion`` instance.

        Returns:
          String representation of the trace region.
        """
        return '%s(Index=%d)' % (self.__class__.__name__, self.RegionIndex)


class JLinkRTTerminalStart(ctypes.Structure):
    """Structure used to configure an RTT instance.

    Attributes:
      ConfigBlockAddress: Address of the RTT block.
    """
    _fields_ = [
        ('ConfigBlockAddress', ctypes.c_uint32),
        ('Reserved', ctypes.c_uint32 * 3)
    ]

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkRTTerminalStart): RTT start instance.

        Returns:
          String representation of the instance.
        """
        return '%s(ConfigAddress=0x%X)' % (self.__class__.__name__, self.ConfigBlockAddress)

    def __str__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkRTTerminalStart): RTT start instance.

        Returns:
          String representation of the instance.
        """
        return self.__repr__()


class JLinkRTTerminalBufDesc(ctypes.Structure):
    """Structure describing a RTT buffer.

    Attributes:
      BufferIndex: index of the buffer to request information about.
      Direction: direction of the upper (`0` for up, `1` for Down).
      acName: Name of the buffer.
      SizeOfBuffer: size of the buffer in bytes.
      Flags: flags set on the buffer.
    """
    _fields_ = [
        ('BufferIndex', ctypes.c_int32),
        ('Direction', ctypes.c_uint32),
        ('acName', ctypes.c_char * 32),
        ('SizeOfBuffer', ctypes.c_uint32),
        ('Flags', ctypes.c_uint32)
    ]

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkRTTerminalBufDesc): the terminal buffer descriptor.

        Returns:
          String representation of the buffer descriptor.
        """
        return '%s(Index=%d, Name=%s)' % (self.__class__.__name__, self.BufferIndex, self.name)

    def __str__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkRTTerminalBufDesc): the terminal buffer descriptor.

        Returns:
          String representation of the buffer descriptor.
        """
        dir_string = 'up' if self.up else 'down'
        return '%s <Index=%d, Direction=%s, Size=%s>' % (self.name, self.BufferIndex,
                                                         dir_string, self.SizeOfBuffer)

    @property
    def up(self):
        """Returns a boolean indicating if the buffer is an 'UP' buffer.

        Args:
          self (JLinkRTTerminalBufDesc): the terminal buffer descriptor.

        Returns:
          ``True`` if the buffer is an 'UP' buffer, otherwise ``False``.
        """
        return (self.Direction == 0)

    @property
    def down(self):
        """Returns a boolean indicating if the buffer is an 'DOWN' buffer.

        Args:
          self (JLinkRTTerminalBufDesc): the terminal buffer descriptor.

        Returns:
          ``True`` if the buffer is an 'DOWN' buffer, otherwise ``False``.
        """
        return (self.Direction == 1)

    @property
    def name(self):
        """Returns the name of the buffer.

        Args:
          self (JLinkRTTerminalBufDesc): the terminal buffer descriptor.

        Returns:
          String name of the buffer.
        """
        return self.acName.decode()


class JLinkRTTerminalStatus(ctypes.Structure):
    """Structure describing the status of the RTT terminal.

    Attributes:
      NumBytesTransferred: number of bytes sent to the client application.
      NumBytesRead: number of bytes read from the target.
      HostOverflowCount: number of overflows on the host.
      IsRunning: if RTT is running.
      NumUpBuffers: number of 'UP' buffers.
      NumDownBuffers: number of 'DOWN' buffers.
    """
    _fields_ = [
        ('NumBytesTransferred', ctypes.c_uint32),
        ('NumBytesRead', ctypes.c_uint32),
        ('HostOverflowCount', ctypes.c_int),
        ('IsRunning', ctypes.c_int),
        ('NumUpBuffers', ctypes.c_int),
        ('NumDownBuffers', ctypes.c_int),
        ('Reserved', ctypes.c_uint32 * 2)
    ]

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkRTTerminalStatus): the status instance.

        Returns:
          Strings representation of the status.
        """
        return '%s(NumUpBuffers=%d, NumDownBuffers=%d)' % (self.__class__.__name__,
                                                           self.NumUpBuffers, self.NumDownBuffers)

    def __str__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkRTTerminalStatus): the status instance.

        Returns:
          Strings representation of the status.
        """
        return 'Status <NumUpBuffers=%d, NumDownBuffers=%d, Running=%s>' % (self.NumUpBuffers,
                                                                            self.NumDownBuffers,
                                                                            self.IsRunning)


class JLinkPowerTraceSetup(ctypes.Structure):
    """Structure used to setup the power tracing.

    Attributes:
      SizeOfStruct: Size of the struct (DO NOT CHANGE).
      ChannelMask: Bitmask indicating which channels to enable for capturing.
      SampleFreq: Sampling frequency in Hertz.
      RefSelect: Identifier of the reference value stored with every trace
        (see ``enums.JLinkPowerTraceRef``).
      EnableCond: `1` is tracing is only captured when CPU is running.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_int32),
        ('ChannelMask', ctypes.c_uint32),
        ('SampleFreq', ctypes.c_uint32),
        ('RefSelect', ctypes.c_int32),
        ('EnableCond', ctypes.c_int32)
    ]

    def __init__(self):
        """Initializes the ``JLinkPowerTraceSetup`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkPowerTraceSetup): the power trace instance.

        Returns:
          ``None``
        """
        super(JLinkPowerTraceSetup, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkPowerTraceSetup): the ``JLinkPowerTraceSetup`` instance

        Returns:
          String representation of the instance.
        """
        return self.__str__()

    def __str__(self):
        """Returns this instance formatted as a string.

        Args:
          self (JLinkPowerTraceSetup): the ``JLinkPowerTraceSetup`` instance

        Returns:
          String formatted instance.
        """
        return '%s(Channel Mask=%s, Freq=%uHz)' % (self.__class__.__name__, bin(self.ChannelMask), self.SampleFreq)


class JLinkPowerTraceItem(ctypes.Structure):
    """Structure used to represent a stored power trace item.

    Attributes:
      RefValue: the stored reference value.
      Value: the actual recorded trace value.
    """
    _fields_ = [
        ('RefValue', ctypes.c_uint32),
        ('Value', ctypes.c_uint32)
    ]

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkPowerTraceItem): the ``JLinkPowerTraceItem`` instance

        Returns:
          String representation of the instance.
        """
        return self.__str__()

    def __str__(self):
        """Returns this instance formatted as a string.

        Args:
          self (JLinkPowerTraceItem): the ``JLinkPowerTraceItem`` instance

        Returns:
          String formatted instance.
        """
        return '%s(Value=%u, Reference=%u)' % (self.__class__.__name__, self.Value, self.RefValue)


class JLinkPowerTraceCaps(ctypes.Structure):
    """Structure used to retrieve or specify available power tracing channels.

    Attributes:
      SizeOfStruct: the size of this structure (in bytes).
      ChannelMask: bitmask of available channels.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_int32),
        ('ChannelMask', ctypes.c_uint32)
    ]

    def __init__(self):
        """Initializes the ``JLinkPowerTraceCaps`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkPowerTraceCaps): the power trace caps instance.

        Returns:
          ``None``
        """
        super(JLinkPowerTraceCaps, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkPowerTraceCaps): the caps instance.

        Returns:
          String representation of the instance.
        """
        return self.__str__()

    def __str__(self):
        """Returns this instance formatted as a string.

        Args:
          self (JLinkPowerTraceCaps): the caps instance.

        Returns:
          String formatted instance.
        """
        return '%s(Channel Mask=%s)' % (self.__class__.__name__, bin(self.ChannelMask))


class JLinkPowerTraceChannelCaps(ctypes.Structure):
    """Structure representing the capabilities for the queried channels.

    Attributes:
      SizeOfStruct: the size of this structure (in bytes).
      BaseSampleFreq: the base sampling frequeny (in Hertz).
      MinDiv: the minimum divider of the base sampling frequency.
    """
    _fields_ = [
        ('SizeOfStruct', ctypes.c_int32),
        ('BaseSampleFreq', ctypes.c_uint32),
        ('MinDiv', ctypes.c_uint32)
    ]

    def __init__(self):
        """Initializes the ``JLinkPowerTraceChannelCaps`` instance.

        Sets the size of the structure.

        Args:
          self (JLinkPowerTraceChannelCaps): the channel capabilities.

        Returns:
          ``None``
        """
        super(JLinkPowerTraceChannelCaps, self).__init__()
        self.SizeOfStruct = ctypes.sizeof(self)

    @property
    def max_sample_freq(self):
        """Returns the maximum sample frequency that can be used.

        The maximum sampling frequency is the largest frequency that can be
        specified when configuring power tracing, and is computed based on the
        minimum divider and base sampling frequency.

        Args:
          self (JLinkPowerTraceChannelCaps): the channel capabilities.

        Returns:
          ``float``
        """
        return (self.BaseSampleFreq * 1.0) / self.MinDiv

    def __repr__(self):
        """Returns a string representation of the instance.

        Args:
          self (JLinkPowerTraceChannelCaps): the channel capabilities.

        Returns:
          String representation of the instance.
        """
        return self.__str__()

    def __str__(self):
        """Returns this instance formatted as a string.

        Args:
          self (JLinkPowerTraceChannelCaps): the channel capabilities.

        Returns:
          String formatted instance.
        """
        return '%s(SampleFreq=%uHz, MinDiv=%u)' % (self.__class__.__name__, self.BaseSampleFreq, self.MinDiv)
