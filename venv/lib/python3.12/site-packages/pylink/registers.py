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


class IDCodeRegisterBits(ctypes.LittleEndianStructure):
    """This class holds the different bit masks for the IDCode register.

    Attributes:
      valid: validity bit, should always be ``0``.
      manufactuer: the JEDEC Manufacturer ID.
      part_no: the part number defined by the manufacturer.
      version_code: the version code.
    """
    _fields_ = [
        ('valid',        ctypes.c_uint32, 1),
        ('manufacturer', ctypes.c_uint32, 11),
        ('part_no',      ctypes.c_uint32, 16),
        ('version_code', ctypes.c_uint32, 4)
    ]


class IDCodeRegisterFlags(ctypes.Union):
    """Mask for the IDCode register bits.

    Attributes:
      value: the value stored in the mask.
    """
    _anonymous_ = ('bit',)
    _fields_ = [
        ('bit',   IDCodeRegisterBits),
        ('value', ctypes.c_uint32)
    ]


class AbortRegisterBits(ctypes.LittleEndianStructure):
    """This class holds the different bit mask for the Abort Register.

    Attributes:
      DAPABORT: write ``1`` to trigger a DAP abort.
      STKCMPCLR: write ``1`` to clear the ``STICKYCMP`` sticky compare flag
          (only supported on SW-DP).
      STKERRCLR: write ``1`` to clear the ``STICKYERR`` sticky error flag
          (only supported on SW-DP).
      WDERRCLR: write ``1`` to clear the ``WDATAERR`` write data error flag
          (only supported on SW-DP).
      ORUNERRCLR: write ``1`` to clear the ``STICKYORUN`` overrun error flag
          (only supported on SW-DP).
    """
    _fields_ = [
        ('DAPABORT',   ctypes.c_uint32, 1),
        ('STKCMPCLR',  ctypes.c_uint32, 1),
        ('STKERRCLR',  ctypes.c_uint32, 1),
        ('WDERRCLR',   ctypes.c_uint32, 1),
        ('ORUNERRCLR', ctypes.c_uint32, 1),
        ('RESERVED',   ctypes.c_uint32, 27),
    ]


class AbortRegisterFlags(ctypes.Union):
    """Mask for the abort register bits.

    Attributes:
      value: the value stored in the mask.
    """
    _anonymous_ = ('bit',)
    _fields_ = [
        ('bit',   AbortRegisterBits),
        ('value', ctypes.c_uint32)
    ]


class ControlStatusRegisterBits(ctypes.LittleEndianStructure):
    """This class holds the different bit masks for the DP Control / Status
    Register bit assignments.

    Attributes:
      ORUNDETECT: if set, enables overrun detection.
      STICKYORUN: if overrun is enabled, is set when overrun occurs.
      TRNMODE: transfer mode for acess port operations.
      STICKYCMP: is set when a match occurs on a pushed compare or verify
          operation.
      STICKYERR: is set when an error is returned by an access port
          transaction.
      READOK: is set when the response to a previous access port or ``RDBUFF``
          was ``OK``.
      WDATAERR: set to ``1`` if a Write Data Error occurs.
      MASKLANE: bytes to be masked in pushed compare and verify operations.
      TRNCNT: transaction counter.
      RESERVED: reserved.
      CDBGRSTREQ: debug reset request.
      CDBGRSTACK: debug reset acknowledge.
      CDBGPWRUPREQ: debug power-up request.
      CDBGPWRUPACK: debug power-up acknowledge.
      CSYSPWRUPREQ: system power-up request
      CSYSPWRUPACK: system power-up acknowledge.

    See also:
      See the ARM documentation on the significance of these masks
      `here <http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ihi0031c/index.html>`_.
    """
    _fields_ = [
        ('ORUNDETECT',   ctypes.c_uint32, 1),   # read/write
        ('STICKYORUN',   ctypes.c_uint32, 1),   # read-only
        ('TRNMODE',      ctypes.c_uint32, 2),   # read/write
        ('STICKYCMP',    ctypes.c_uint32, 1),   # read-only
        ('STICKYERR',    ctypes.c_uint32, 1),   # read-only
        ('READOK',       ctypes.c_uint32, 1),   # read-only
        ('WDATAERR',     ctypes.c_uint32, 1),   # read-only
        ('MASKLANE',     ctypes.c_uint32, 4),   # read/write
        ('TRNCNT',       ctypes.c_uint32, 12),  # read/write
        ('RESERVED',     ctypes.c_uint32, 2),   # -
        ('CDBGRSTREQ',   ctypes.c_uint32, 1),   # read/write
        ('CDBGRSTACK',   ctypes.c_uint32, 1),   # read-only
        ('CDBGPWRUPREQ', ctypes.c_uint32, 1),   # read/write
        ('CDBGPWRUPACK', ctypes.c_uint32, 1),   # read-only
        ('CSYSPWRUPREQ', ctypes.c_uint32, 1),   # read/write
        ('CSYSPWRUPACK', ctypes.c_uint32, 1)    # read-only
    ]


class ControlStatusRegisterFlags(ctypes.Union):
    """Mask for the control/status register bits.

    Attributes:
      value: the value stored in the mask.
    """
    _anonymous_ = ('bit',)
    _fields_ = [
        ('bit',   ControlStatusRegisterBits),
        ('value', ctypes.c_uint32)
    ]


class SelectRegisterBits(ctypes.LittleEndianStructure):
    """This class holds the different bit masks for the AP Select Register.

    Attributes:
      CTRLSEL: SW-DP debug port address bank select.
      RESERVED_A: reserved.
      APBANKSEL: selects the active four-word register window on the current
          access port.
      RESERVED_B: reserved.
      APSEL: selects the current access port.
    """
    _fields_ = [
        ('CTRLSEL',    ctypes.c_uint32, 1),
        ('RESERVED_A', ctypes.c_uint32, 3),
        ('APBANKSEL',  ctypes.c_uint32, 4),
        ('RESERVED_B', ctypes.c_uint32, 16),
        ('APSEL',      ctypes.c_uint32, 8)
    ]


class SelectRegisterFlags(ctypes.Union):
    """Mask for the select register bits.

    Attributes:
      value: the value stored in the mask.
    """
    _anonymous_ = ('bit',)
    _fields_ = [
        ('bit',   SelectRegisterBits),
        ('value', ctypes.c_uint32)
    ]


class MDMAPControlRegisterBits(ctypes.LittleEndianStructure):
    """This class holds the different bit masks for the MDM-AP Control
    Register.

    Attributes:
      flash_mass_erase: set to cause a mass erase, this is cleared
          automatically when a mass erase finishes.
      debug_disable: set to disable debug, clear to allow debug.
      debug_request: set to force the core to halt.
      sys_reset_request: set to force a system reset.
      core_hold_reset: set to suspend the core in reset at the end of reset
          sequencing.
      VLLDBGREQ: set to hold the system in reset after the next recovery from
          VLLSx (Very Low Leakage Stop).
      VLLDBGACK: set to release a system held in reset following a VLLSx
          (Very Low Leakage Stop) recovery.
      VLLSTATACK: set to acknowledge that the DAP LLS (Low Leakage Stop) and
          VLLS (Very Low Leakage Stop) status bits have read.
    """
    _fields_ = [
        ('flash_mass_erase',  ctypes.c_uint8, 1),
        ('debug_disable',     ctypes.c_uint8, 1),
        ('debug_request',     ctypes.c_uint8, 1),
        ('sys_reset_request', ctypes.c_uint8, 1),
        ('core_hold_reset',   ctypes.c_uint8, 1),
        ('VLLDBGREQ',         ctypes.c_uint8, 1),
        ('VLLDBGACK',         ctypes.c_uint8, 1),
        ('VLLSTATACK',        ctypes.c_uint8, 1)
    ]


class MDMAPControlRegisterFlags(ctypes.Union):
    """Mask for the MDM-AP control register bits.

    Attributes:
      value: the value stored in the mask.
    """
    _anonymous_ = ('bit',)
    _fields_ = [
        ('bit',   MDMAPControlRegisterBits),
        ('value', ctypes.c_uint8)
    ]


class MDMAPStatusRegisterBits(ctypes.LittleEndianStructure):
    """Holds the bit masks for the MDM-AP Status Register.

    Attributes:
      flash_mass_erase_ack: cleared after a system reset, indicates that a
          flash mass erase was acknowledged.
      flash_ready: indicates that flash has been initialized and can be
          configured.
      system_security: if set, system is secure and debugger cannot access the
          memory or system bus.
      system_reset: ``1`` if system is in reset, otherwise ``0``.
      mass_erase_enabled: ``1`` if MCU can be mass erased, otherwise ``0``.
      low_power_enabled: ``1`` if low power stop mode is enabled, otherwise ``0``.
      very_low_power_mode: ``1`` if device is in very low power mode.
      LLSMODEEXIT: indicates an exit from LLS mode has occurred.
      VLLSxMODEEXIT: indicates an exit from VLLSx mode has occured.
      core_halted; indicates core has entered debug halt mode.
      core_deep_sleep: indicates core has entered a low power mode.
      core_sleeping: indicates the core has entered a low power mode.

    Note:
      if ``core_sleeping & !core_deep_sleep``, then the core is in VLPW (very
      low power wait) mode, otherwise if ``core_sleeping & core_deep_sleep``,
      then it is in VLPS (very low power stop) mode.
    """
    _fields_ = [
        ('flash_mass_erase_ack',    ctypes.c_uint32, 1),
        ('flash_ready',             ctypes.c_uint32, 1),
        ('system_security',         ctypes.c_uint32, 1),
        ('system_reset',            ctypes.c_uint32, 1),
        ('RESERVED_A',              ctypes.c_uint32, 1),
        ('mass_erase_enabled',      ctypes.c_uint32, 1),
        ('backdoor_access_enabled', ctypes.c_uint32, 1),
        ('low_power_enabled',       ctypes.c_uint32, 1),
        ('very_low_power_mode',     ctypes.c_uint32, 1),
        ('LLSMODEEXIT',             ctypes.c_uint32, 1),
        ('VLLSxMODEEXIT',           ctypes.c_uint32, 1),
        ('RESERVED_B',              ctypes.c_uint32, 5),
        ('core_halted',             ctypes.c_uint32, 1),
        ('core_deep_sleep',         ctypes.c_uint32, 1),
        ('core_sleeping',           ctypes.c_uint32, 1),
        ('RESERVED_C',              ctypes.c_uint32, 13)
    ]


class MDMAPStatusRegisterFlags(ctypes.Union):
    """Mask for the MDM-AP status register bits.

    Attributes:
      value: the value stored in the mask.
    """
    _anonymous_ = ('bit',)
    _fields_ = [
        ('bit',   MDMAPStatusRegisterBits),
        ('value', ctypes.c_uint32)
    ]
