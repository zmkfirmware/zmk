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

import ctypes


class JLinkGlobalErrors(object):
    """Enumeration for the error codes which any J-Link SDK DLL API-function
    can have as a return value."""
    UNSPECIFIED_ERROR = -1
    EMU_NO_CONNECTION = -256
    EMU_COMM_ERROR = -257
    DLL_NOT_OPEN = -258
    VCC_FAILURE = -259
    INVALID_HANDLE = -260
    NO_CPU_FOUND = -261
    EMU_FEATURE_UNSUPPORTED = -262
    EMU_NO_MEMORY = -263
    TIF_STATUS_ERROR = -264
    FLASH_PROG_COMPARE_FAILED = -265
    FLASH_PROG_PROGRAM_FAILED = -266
    FLASH_PROG_VERIFY_FAILED = -267
    OPEN_FILE_FAILED = -268
    UNKNOWN_FILE_FORMAT = -269
    WRITE_TARGET_MEMORY_FAILED = -270
    DEVICE_FEATURE_NOT_SUPPORTED = -271
    WRONG_USER_CONFIG = -272
    NO_TARGET_DEVICE_SELECTED = -273
    CPU_IN_LOW_POWER_MODE = -274

    @classmethod
    def to_string(cls, error_code):
        """Returns the string message for the given ``error_code``.

        Args:
          cls (JlinkGlobalErrors): the ``JLinkGlobalErrors`` class
          error_code (int): error code to convert

        Returns:
          An error string corresponding to the error code.

        Raises:
          ValueError: if the error code is invalid.
        """
        if error_code == cls.EMU_NO_CONNECTION:
            return 'No connection to emulator.'
        elif error_code == cls.EMU_COMM_ERROR:
            return 'Emulator connection error.'
        elif error_code == cls.DLL_NOT_OPEN:
            return 'DLL has not been opened.  Did you call \'.connect()\'?'
        elif error_code == cls.VCC_FAILURE:
            return 'Target system has no power.'
        elif error_code == cls.INVALID_HANDLE:
            return 'Given file / memory handle is invalid.'
        elif error_code == cls.NO_CPU_FOUND:
            return 'Could not find supported CPU.'
        elif error_code == cls.EMU_FEATURE_UNSUPPORTED:
            return 'Emulator does not support the selected feature.'
        elif error_code == cls.EMU_NO_MEMORY:
            return 'Emulator out of memory.'
        elif error_code == cls.TIF_STATUS_ERROR:
            return 'Target interface error.'
        elif error_code == cls.FLASH_PROG_COMPARE_FAILED:
            return 'Programmed data differs from source data.'
        elif error_code == cls.FLASH_PROG_PROGRAM_FAILED:
            return 'Programming error occured.'
        elif error_code == cls.FLASH_PROG_VERIFY_FAILED:
            return 'Error while verifying programmed data.'
        elif error_code == cls.OPEN_FILE_FAILED:
            return 'Specified file could not be opened.'
        elif error_code == cls.UNKNOWN_FILE_FORMAT:
            return 'File format of selected file is not supported.'
        elif error_code == cls.WRITE_TARGET_MEMORY_FAILED:
            return 'Could not write target memory.'
        elif error_code == cls.DEVICE_FEATURE_NOT_SUPPORTED:
            return 'Feature not supported by connected device.'
        elif error_code == cls.WRONG_USER_CONFIG:
            return 'User configured DLL parameters incorrectly.'
        elif error_code == cls.NO_TARGET_DEVICE_SELECTED:
            return 'User did not specify core to connect to.'
        elif error_code == cls.CPU_IN_LOW_POWER_MODE:
            return 'Target CPU is in low power mode.'
        elif error_code == cls.UNSPECIFIED_ERROR:
            return 'Unspecified error.'
        raise ValueError('Invalid error code: %d' % error_code)


class JLinkEraseErrors(JLinkGlobalErrors):
    """Enumeration for the error codes generated during an erase operation."""

    ILLEGAL_COMMAND = -5

    @classmethod
    def to_string(cls, error_code):
        """Returns the string message for the given ``error_code``.

        Args:
          cls (JLinkEraseErrors): the ``JLinkEraseErrors`` class
          error_code (int): error code to convert

        Returns:
          An error string corresponding to the error code.

        Raises:
          ValueError: if the error code is invalid.
        """
        if error_code == cls.ILLEGAL_COMMAND:
            return 'Failed to erase sector.'
        return super(JLinkEraseErrors, cls).to_string(error_code)


class JLinkFlashErrors(JLinkGlobalErrors):
    """Enumeration for the error codes generated during a flash operation."""

    COMPARE_ERROR = -2
    PROGRAM_ERASE_ERROR = -3
    VERIFICATION_ERROR = -4

    @classmethod
    def to_string(cls, error_code):
        """Returns the string message for the given ``error_code``.

        Args:
          cls (JLinkFlashErrors): the ``JLinkFlashErrors`` class
          error_code (int): error code to convert

        Returns:
          An error string corresponding to the error code.

        Raises:
          ValueError: if the error code is invalid.
        """
        if error_code == cls.COMPARE_ERROR:
            return 'Error comparing flash content to programming data.'
        elif error_code == cls.PROGRAM_ERASE_ERROR:
            return 'Error during program/erase phase.'
        elif error_code == cls.VERIFICATION_ERROR:
            return 'Error verifying programmed data.'
        return super(JLinkFlashErrors, cls).to_string(error_code)


class JLinkWriteErrors(JLinkGlobalErrors):
    """Enumeration for the error codes generated during a write."""

    ZONE_NOT_FOUND_ERROR = -5

    @classmethod
    def to_string(cls, error_code):
        """Returns the string message for the given ``error_code``.

        Args:
          cls (JLinkWriteErrors): the ``JLinkWriteErrors`` class
          error_code (int): error code to convert

        Returns:
          An error string corresponding to the error code.

        Raises:
          ValueError: if the error code is invalid.
        """
        if error_code == cls.ZONE_NOT_FOUND_ERROR:
            return 'Zone not found'
        return super(JLinkWriteErrors, cls).to_string(error_code)


class JLinkReadErrors(JLinkGlobalErrors):
    """Enumeration for the error codes generated during a read."""

    ZONE_NOT_FOUND_ERROR = -5

    @classmethod
    def to_string(cls, error_code):
        """Returns the string message for the given ``error_code``.

        Args:
          cls (JLinkReadErrors): the ``JLinkReadErrors`` class
          error_code (int): error code to convert

        Returns:
          An error string corresponding to the error code.

        Raises:
          ValueError: if the error code is invalid.
        """
        if error_code == cls.ZONE_NOT_FOUND_ERROR:
            return 'Zone not found'
        return super(JLinkReadErrors, cls).to_string(error_code)


class JLinkDataErrors(JLinkGlobalErrors):
    """Enumeration for the error codes generated when setting a data event."""

    ERROR_UNKNOWN = 0x80000000
    ERROR_NO_MORE_EVENTS = 0x80000001
    ERROR_NO_MORE_ADDR_COMP = 0x80000002
    ERROR_NO_MORE_DATA_COMP = 0x80000004
    ERROR_INVALID_ADDR_MASK = 0x80000020
    ERROR_INVALID_DATA_MASK = 0x80000040
    ERROR_INVALID_ACCESS_MASK = 0x80000080

    @classmethod
    def to_string(cls, error_code):
        """Returns the string message for the given error code.

        Args:
          cls (JLinkDataErrors): the ``JLinkDataErrors`` class
          error_code (int): error code to convert

        Returns:
          An error string corresponding to the error code.

        Raises:
          ValueError: if the error code is invalid.
        """
        if error_code == cls.ERROR_UNKNOWN:
            return 'Unknown error.'
        elif error_code == cls.ERROR_NO_MORE_EVENTS:
            return 'There are no more available watchpoint units.'
        elif error_code == cls.ERROR_NO_MORE_ADDR_COMP:
            return 'No more address comparisons can be set.'
        elif error_code == cls.ERROR_NO_MORE_DATA_COMP:
            return 'No more data comparisons can be set.'
        elif error_code == cls.ERROR_INVALID_ADDR_MASK:
            return 'Invalid flags passed for the address mask.'
        elif error_code == cls.ERROR_INVALID_DATA_MASK:
            return 'Invalid flags passed for the data mask.'
        elif error_code == cls.ERROR_INVALID_ACCESS_MASK:
            return 'Invalid flags passed for the access mask.'
        return super(JLinkDataErrors, cls).to_string(error_code)


class JLinkRTTErrors(JLinkGlobalErrors):
    """Enumeration for error codes from RTT."""

    RTT_ERROR_CONTROL_BLOCK_NOT_FOUND = -2

    @classmethod
    def to_string(cls, error_code):
        """Returns the string message for the given error code.

        Args:
          cls (JLinkRTTErrors): the ``JLinkRTTErrors`` class
          error_code (int): error code to convert

        Returns:
          An error string corresponding to the error code.

        Raises:
          ValueError: if the error code is invalid.
        """
        if error_code == cls.RTT_ERROR_CONTROL_BLOCK_NOT_FOUND:
            return 'The RTT Control Block has not yet been found (wait?)'
        return super(JLinkRTTErrors, cls).to_string(error_code)


class JLinkHost(object):
    """Enumeration for the different JLink hosts: currently only IP and USB."""
    USB = (1 << 0)
    IP = (1 << 1)
    USB_OR_IP = USB | IP


class JLinkInterfaces(object):
    """Target interfaces for the J-Link."""
    JTAG = 0
    SWD = 1
    FINE = 3
    ICSP = 4
    SPI = 5
    C2 = 6


class JLinkResetStrategyCortexM3(object):
    """Target reset strategies for the J-Link.

    Attributes:
      NORMAL: default reset strategy, does whatever is best to reset.
      CORE: only the core is reset via the ``VECTRESET`` bit.
      RESETPIN: pulls the reset pin low to reset the core and peripherals.
      CONNECT_UNDER_RESET: J-Link connects to target while keeping reset
        active.  This is recommented for STM32 devices.
      HALT_AFTER_BTL: halt the core after the bootloader is executed.
      HALT_BEFORE_BTL: halt the core before the bootloader is executed.
      KINETIS: performs a normal reset, but also disables the watchdog.
      ADI_HALT_AFTER_KERNEL: sets the ``SYSRESETREQ`` bit in the ``AIRCR`` in
        order to reset the device.
      CORE_AND_PERIPHERALS: sets the ``SYSRESETREQ`` bit in the ``AIRCR``, and
        the ``VC_CORERESET`` bit in the ``DEMCR`` to make sure that the CPU is
        halted immediately after reset.
      LPC1200: reset for LPC1200 devices.
      S3FN60D: reset for Samsung S3FN60D devices.

    Note:
      Please see the J-Link SEGGER Documentation, UM8001, for full information
      about the different reset strategies.
    """
    NORMAL = 0
    CORE = 1
    RESETPIN = 2
    CONNECT_UNDER_RESET = 3
    HALT_AFTER_BTL = 4
    HALT_BEFORE_BTL = 5
    KINETIS = 6
    ADI_HALT_AFTER_KERNEL = 7
    CORE_AND_PERIPHERALS = 8
    LPC1200 = 9
    S3FN60D = 10


class JLinkFunctions(object):
    """Collection of function prototype and type builders for the J-Link SDK
    API calls."""
    LOG_PROTOTYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p)
    UNSECURE_HOOK_PROTOTYPE = ctypes.CFUNCTYPE(ctypes.c_int,
                                               ctypes.c_char_p,
                                               ctypes.c_char_p,
                                               ctypes.c_uint32)
    FLASH_PROGRESS_PROTOTYPE = ctypes.CFUNCTYPE(None,
                                                ctypes.c_char_p,
                                                ctypes.c_char_p,
                                                ctypes.c_int)


class JLinkCore(object):
    """Enumeration for the different CPU core identifiers.

    These are the possible cores for targets the J-Link is connected to.
    Note that these are bitfields."""
    NONE = 0x00000000
    ANY = 0xFFFFFFFF
    CORTEX_M1 = 0x010000FF
    COLDFIRE = 0x02FFFFFF
    CORTEX_M3 = 0x030000FF
    CORTEX_M3_R1P0 = 0x03000010
    CORTEX_M3_R1P1 = 0x03000011
    CORTEX_M3_R2P0 = 0x03000020
    SIM = 0x04FFFFFF
    XSCALE = 0x05FFFFFF
    CORTEX_M0 = 0x060000FF
    CORTEX_M_V8BASEL = 0x060100FF
    ARM7 = 0x07FFFFFF
    ARM7TDMI = 0x070000FF
    ARM7TDMI_R3 = 0x0700003F
    ARM7TDMI_R4 = 0x0700004F
    ARM7TDMI_S = 0x070001FF
    ARM7TDMI_S_R3 = 0x0700013F
    ARM7TDMI_S_R4 = 0x0700014F
    CORTEX_A8 = 0x080000FF
    CORTEX_A7 = 0x080800FF
    CORTEX_A9 = 0x080900FF
    CORTEX_A12 = 0x080A00FF
    CORTEX_A15 = 0x080B00FF
    CORTEX_A17 = 0x080C00FF
    ARM9 = 0x09FFFFFF
    ARM9TDMI_S = 0x090001FF
    ARM920T = 0x092000FF
    ARM922T = 0x092200FF
    ARM926EJ_S = 0x092601FF
    ARM946E_S = 0x094601FF
    ARM966E_S = 0x096601FF
    ARM968E_S = 0x096801FF
    ARM11 = 0x0BFFFFFF
    ARM1136 = 0x0B36FFFF
    ARM1136J = 0x0B3602FF
    ARM1136J_S = 0x0B3603FF
    ARM1136JF = 0x0B3606FF
    ARM1136JF_S = 0x0B3607FF
    ARM1156 = 0x0B56FFFF
    ARM1176 = 0x0B76FFFF
    ARM1176J = 0x0B7602FF
    ARM1176J_S = 0x0B7603FF
    ARM1176JF = 0x0B7606FF
    ARM1176JF_S = 0x0B7607FF
    CORTEX_R4 = 0x0C0000FF
    CORTEX_R5 = 0x0C0100FF
    RX = 0x0DFFFFFF
    RX610 = 0x0D00FFFF
    RX62N = 0x0D01FFFF
    RX62T = 0x0D02FFFF
    RX63N = 0x0D03FFFF
    RX630 = 0x0D04FFFF
    RX63T = 0x0D05FFFF
    RX621 = 0x0D06FFFF
    RX62G = 0x0D07FFFF
    RX631 = 0x0D08FFFF
    RX210 = 0x0D10FFFF
    RX21A = 0x0D11FFFF
    RX220 = 0x0D12FFFF
    RX230 = 0x0D13FFFF
    RX231 = 0x0D14FFFF
    RX23T = 0x0D15FFFF
    RX111 = 0x0D20FFFF
    RX110 = 0x0D21FFFF
    RX113 = 0x0D22FFFF
    RX64M = 0x0D30FFFF
    RX71M = 0x0D31FFFF
    CORTEX_M4 = 0x0E0000FF
    CORTEX_M7 = 0x0E0100FF
    CORTEX_M_V8MAINL = 0x0E0200FF
    CORTEX_A5 = 0x0F0000FF
    POWER_PC = 0x10FFFFFF
    POWER_PC_N1 = 0x10FF00FF
    POWER_PC_N2 = 0x10FF01FF
    MIPS = 0x11FFFFFF
    MIPS_M4K = 0x1100FFFF
    MIPS_MICROAPTIV = 0x1101FFFF
    EFM8_UNSPEC = 0x12FFFFFF
    CIP51 = 0x1200FFFF


class JLinkDeviceFamily(object):
    """Enumeration for the difference device families.

    These are the possible device families for targets that the J-Link is
    connected to."""
    AUTO = 0
    CORTEX_M1 = 1
    COLDFIRE = 2
    CORTEX_M3 = 3
    SIMULATOR = 4
    XSCALE = 5
    CORTEX_M0 = 6
    ARM7 = 7
    CORTEX_A8 = 8
    CORTEX_A9 = 8
    ARM9 = 9
    ARM10 = 10
    ARM11 = 11
    CORTEX_R4 = 12
    RX = 13
    CORTEX_M4 = 14
    CORTEX_A5 = 15
    POWERPC = 16
    MIPS = 17
    EFM8 = 18
    ANY = 255


class JLinkFlags(object):
    """Enumeration for the different flags that are passed to the J-Link C SDK
    API methods."""
    GO_OVERSTEP_BP = (1 << 0)

    DLG_BUTTON_YES = (1 << 0)
    DLG_BUTTON_NO = (1 << 1)
    DLG_BUTTON_OK = (1 << 2)
    DLG_BUTTON_CANCEL = (1 << 3)

    HW_PIN_STATUS_LOW = 0
    HW_PIN_STATUS_HIGH = 1
    HW_PIN_STATUS_UNKNOWN = 255


class JLinkSWOInterfaces(object):
    """Serial Wire Output (SWO) interfaces."""
    UART = 0
    MANCHESTER = 1  # DO NOT USE


class JLinkSWOCommands(object):
    """Serial Wire Output (SWO) commands."""
    START = 0
    STOP = 1
    FLUSH = 2
    GET_SPEED_INFO = 3
    GET_NUM_BYTES = 10
    SET_BUFFERSIZE_HOST = 20
    SET_BUFFERSIZE_EMU = 21


class JLinkCPUCapabilities(object):
    """Target CPU Cabilities."""
    READ_MEMORY = (1 << 1)
    WRITE_MEMORY = (1 << 2)
    READ_REGISTERS = (1 << 3)
    WRITE_REGISTERS = (1 << 4)
    GO = (1 << 5)
    STEP = (1 << 6)
    HALT = (1 << 7)
    IS_HALTED = (1 << 8)
    RESET = (1 << 9)
    RUN_STOP = (1 << 10)
    TERMINAL = (1 << 11)
    DCC = (1 << 14)
    HSS = (1 << 15)


class JLinkHaltReasons(object):
    """Halt reasons for the CPU.

    Attributes:
      DBGRQ: CPU has been halted because DBGRQ signal asserted.
      CODE_BREAKPOINT: CPU has been halted because of code breakpoint match.
      DATA_BREAKPOINT: CPU has been halted because of data breakpoint match.
      VECTOR_CATCH: CPU has been halted because of vector catch.
    """
    DBGRQ = 0
    CODE_BREAKPOINT = 1
    DATA_BREAKPOINT = 2
    VECTOR_CATCH = 3


class JLinkVectorCatchCortexM3(object):
    """Vector catch types for the ARM Cortex M3.

    Attributes:
      CORE_RESET: The CPU core reset.
      MEM_ERROR: A memory management error occurred.
      COPROCESSOR_ERROR: Usage fault error accessing the Coprocessor.
      CHECK_ERROR: Usage fault error on enabled check.
      STATE_ERROR: Usage fault state error.
      BUS_ERROR: Normal bus error.
      INT_ERROR: Interrupt or exception service error.
      HARD_ERROR: Hard fault error.
    """
    CORE_RESET = (1 << 0)
    MEM_ERROR = (1 << 4)
    COPROCESSOR_ERROR = (1 << 5)
    CHECK_ERROR = (1 << 6)
    STATE_ERROR = (1 << 7)
    BUS_ERROR = (1 << 8)
    INT_ERROR = (1 << 9)
    HARD_ERROR = (1 << 10)


class JLinkBreakpoint(object):
    """J-Link breakpoint types.

    Attributes:
      SW_RAM: Software breakpont located in RAM.
      SW_FLASH: Software breakpoint located in flash.
      SW: Software breakpoint located in RAM or flash.
      HW: Hardware breakpoint.
      ANY: Allows specifying any time of breakpoint.
      ARM: Breakpoint in ARM mode (only available on ARM 7/9 cores).
      THUMB: Breakpoint in THUMB mode (only available on ARM 7/9 cores).
    """
    SW_RAM = (1 << 4)
    SW_FLASH = (1 << 5)
    SW = (0x000000F0)
    HW = (0xFFFFFF00)
    ANY = (0xFFFFFFF0)
    ARM = (1 << 0)
    THUMB = (2 << 0)


class JLinkBreakpointImplementation(object):
    """J-Link breakpoint implementation types.

    Attributes:
      HARD: Hardware breakpoint using a breakpoint unit.
      SOFT: Software breakpoint using a breakpoint instruction.
      PENDING: Breakpoint has not been set yet.
      FLASH: Breakpoint set in flash.
    """
    HARD = (1 << 0)
    SOFT = (1 << 1)
    PENDING = (1 << 2)
    FLASH = (1 << 4)


class JLinkEventTypes(object):
    """J-Link data event types.

    Attributes:
      BREAKPOINT: breakpoint data event.
    """
    BREAKPOINT = (1 << 0)


class JLinkAccessFlags(object):
    """J-Link access types for data events.

    These access types allow specifying the different types of access events
    that should be monitored.

    Attributes:
      READ: specifies to monitor read accesses.
      WRITE: specifies to monitor write accesses.
      PRIVILEGED: specifies to monitor privileged accesses.
      SIZE_8BIT: specifies to monitor an 8-bit access width.
      SIZE_16BIT: specifies to monitor an 16-bit access width.
      SIZE_32BIT: specifies to monitor an 32-bit access width.
    """
    READ = (0 << 0)
    WRITE = (1 << 0)
    PRIV = (1 << 4)
    SIZE_8BIT = (0 << 1)
    SIZE_16BIT = (1 << 1)
    SIZE_32BIT = (2 << 1)


class JLinkAccessMaskFlags(object):
    """J-Link access mask flags.

    Attributes:
      SIZE: specifies to not care about the access size of the event.
      DIR: specifies to not care about the access direction of the event.
      PRIV: specifies to not care about the access privilege of the event.
    """
    SIZE = (3 << 1)
    DIR = (1 << 0)
    PRIV = (1 << 4)


class JLinkStraceCommand(object):
    """STRACE commmands."""
    TRACE_EVENT_SET = 0
    TRACE_EVENT_CLR = 1
    TRACE_EVENT_CLR_ALL = 2
    SET_BUFFER_SIZE = 3


class JLinkStraceEvent(object):
    """STRACE events."""
    CODE_FETCH = 0
    DATA_ACCESS = 1
    DATA_LOAD = 2
    DATA_STORE = 3


class JLinkStraceOperation(object):
    """STRACE operation specifiers."""
    TRACE_START = 0
    TRACE_STOP = 1
    TRACE_INCLUDE_RANGE = 2
    TRACE_EXCLUDE_RANGE = 3


class JLinkTraceSource(object):
    """Sources for tracing."""
    ETB = 0
    ETM = 1
    MTB = 2


class JLinkTraceCommand(object):
    """J-Link trace commands."""
    START = 0x0
    STOP = 0x1
    FLUSH = 0x2
    GET_NUM_SAMPLES = 0x10
    GET_CONF_CAPACITY = 0x11
    SET_CAPACITY = 0x12
    GET_MIN_CAPACITY = 0x13
    GET_MAX_CAPACITY = 0x14
    SET_FORMAT = 0x20
    GET_FORMAT = 0x21
    GET_NUM_REGIONS = 0x30
    GET_REGION_PROPS = 0x31
    GET_REGION_PROPS_EX = 0x32


class JLinkTraceFormat(object):
    """J-Link trace formats.

    Attributes:
      FORMAT_4BIT: 4-bit data.
      FORMAT_8BIT: 8-bit data.
      FORMAT_16BIT: 16-bit data.
      FORMAT_MULTIPLEXED: multiplexing on ETM / buffer link.
      FORMAT_DEMULTIPLEXED: de-multiplexing on ETM / buffer link.
      FORMAT_DOUBLE_EDGE: clock data on both ETM / buffer link edges.
      FORMAT_ETM7_9: ETM7/ETM9 protocol.
      FORMAT_ETM10: ETM10 protocol.
      FORMAT_1BIT: 1-bit data.
      FORMAT_2BIT: 2-bit data.
    """
    FORMAT_4BIT = 0x1
    FORMAT_8BIT = 0x2
    FORMAT_16BIT = 0x4
    FORMAT_MULTIPLEXED = 0x8
    FORMAT_DEMULTIPLEXED = 0x10
    FORMAT_DOUBLE_EDGE = 0x20
    FORMAT_ETM7_9 = 0x40
    FORMAT_ETM10 = 0x80
    FORMAT_1BIT = 0x100
    FORMAT_2BIT = 0x200


class JLinkROMTable(object):
    """The J-Link ROM tables."""
    NONE = 0x100
    ETM = 0x101
    MTB = 0x102
    TPIU = 0x103
    ITM = 0x104
    DWT = 0x105
    FPB = 0x106
    NVIC = 0x107
    TMC = 0x108
    TF = 0x109
    PTM = 0x10A
    ETB = 0x10B
    DBG = 0x10C
    APBAP = 0x10D
    AHBAP = 0x10E
    SECURE = 0x10F


class JLinkRTTCommand(object):
    """RTT commands."""
    START = 0
    STOP = 1
    GETDESC = 2
    GETNUMBUF = 3
    GETSTAT = 4


class JLinkRTTDirection(object):
    """RTT Direction."""
    UP = 0
    DOWN = 1


class JLinkPowerTraceCommand(object):
    """Power trace commands."""
    SETUP = 0
    START = 1
    FLUSH = 2
    STOP = 3
    GET_CAPS = 4
    GET_CHANNEL_CAPS = 5
    GET_NUM_ITEMS = 6


class JLinkPowerTraceRef(object):
    """Reference values to store on power trace capture.

    Attributes:
        NONE: No reference value is stored.
        BYTES: Number of bytes transferred via SWO is stored since capturing
            started.
        TIME: Number of milliseconds since capturing started.
    """
    NONE = 0
    BYTES = 1
    TIME = 2
