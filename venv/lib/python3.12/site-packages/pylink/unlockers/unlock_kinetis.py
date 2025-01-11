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

from .. import decorators
from .. import enums
from ..protocols import swd
from .. import registers

import collections
import time


Identity = collections.namedtuple('Identity', ['version_code', 'part_no'])

UNLOCK_METHODS = {}


class KinetisException(Exception):
    """Exception generated when polling fails."""
    pass


def unlock_kinetis_identified(identity, flags):
    """Checks whether the given flags are a valid identity.

    Args:
      identity (Identity): the identity to validate against
      flags (register.IDCodeRegisterFlags): the set idcode flags

    Returns:
      ``True`` if the given ``flags`` correctly identify the the debug
      interface, otherwise ``False``.
    """
    if flags.version_code != identity.version_code:
        return False

    if flags.part_no != identity.part_no:
        return False

    return flags.valid


def unlock_kinetis_abort_clear():
    """Returns the abort register clear code.

    Returns:
      The abort register clear code.
    """
    flags = registers.AbortRegisterFlags()
    flags.STKCMPCLR = 1
    flags.STKERRCLR = 1
    flags.WDERRCLR = 1
    flags.ORUNERRCLR = 1
    return flags.value


def unlock_kinetis_read_until_ack(jlink, address):
    """Polls the device until the request is acknowledged.

    Sends a read request to the connected device to read the register at the
    given 'address'.  Polls indefinitely until either the request is ACK'd or
    the request ends in a fault.

    Args:
      jlink (JLink): the connected J-Link
      address (int) the address of the register to poll

    Returns:
      ``SWDResponse`` object on success.

    Raises:
      KinetisException: when read exits with non-ack or non-wait status.

    Note:
      This function is required in order to avoid reading corrupt or otherwise
      invalid data from registers when communicating over SWD.
    """
    request = swd.ReadRequest(address, ap=True)
    response = None
    while True:
        response = request.send(jlink)
        if response.ack():
            break
        elif response.wait():
            continue
        raise KinetisException('Read exited with status: %s', response.status)

    return response


def unlock_kinetis_swd(jlink):
    """Unlocks a Kinetis device over SWD.

    Steps Involved in Unlocking:
      1.  Verify that the device is configured to read/write from the CoreSight
          registers; this is done by reading the Identification Code Register
          and checking its validity.  This register is always at 0x0 on reads.
      2.  Check for errors in the status register.  If there are any errors,
          they must be cleared by writing to the Abort Register (this is always
          0x0 on writes).
      3.  Turn on the device power and debug power so that the target is
          powered by the J-Link as more power is required during an unlock.
      4.  Assert the ``RESET`` pin to force the target to hold in a reset-state
          as to avoid interrupts and other potentially breaking behaviour.
      5.  At this point, SWD is configured, so send a request to clear the
          errors, if any, that may currently be set.
      6.  Our next SWD request selects the MDM-AP register so that we can start
          sending unlock instructions.  ``SELECT[31:24] = 0x01`` selects it.
      7.  Poll the MDM-AP Status Register (AP[1] bank 0, register 0) until the
          flash ready bit is set to indicate we can flash.
      8.  Write to the MDM-AP Control Register (AP[1] bank 0, register 1) to
          request a flash mass erase.
      9.  Poll the system until the flash mass erase bit is acknowledged in the
          MDM-AP Status Register.
      10. Poll the control register until it clears it's mass erase bit to
          indicate that it finished mass erasing, and therefore the system is
          now unsecure.

    Args:
      jlink (JLink): the connected J-Link

    Returns:
      ``True`` if the device was unlocked successfully, otherwise ``False``.

    Raises:
      KinetisException: when the device cannot be unlocked or fails to unlock.

    See Also:
      `NXP Forum <https://community.nxp.com/thread/317167>`_.

    See Also:
      `Kinetis Docs <nxp.com/files/32bit/doc/ref_manual/K12P48M50SF4RM.pdf>`
    """
    SWDIdentity = Identity(0x2, 0xBA01)
    jlink.power_on()
    jlink.coresight_configure()

    # 1. Verify that the device is configured properly.
    flags = registers.IDCodeRegisterFlags()
    flags.value = jlink.coresight_read(0x0, False)
    if not unlock_kinetis_identified(SWDIdentity, flags):
        return False

    # 2. Check for errors.
    flags = registers.ControlStatusRegisterFlags()
    flags.value = jlink.coresight_read(0x01, False)
    if flags.STICKYORUN or flags.STICKYCMP or flags.STICKYERR or flags.WDATAERR:
        jlink.coresight_write(0x0, unlock_kinetis_abort_clear(), False)

    # 3. Turn on device power and debug.
    flags = registers.ControlStatusRegisterFlags()
    flags.value = 0
    flags.CSYSPWRUPREQ = 1  # System power-up request
    flags.CDBGPWRUPREQ = 1  # Debug power-up request
    jlink.coresight_write(0x01, flags.value, False)

    # 4. Assert the reset pin.
    jlink.set_reset_pin_low()
    time.sleep(1)

    # 5. Send a SWD Request to clear any errors.
    request = swd.WriteRequest(0x0, False, unlock_kinetis_abort_clear())
    request.send(jlink)

    # 6. Send a SWD Request to select the MDM-AP register, SELECT[31:24] = 0x01
    request = swd.WriteRequest(0x2, False, (1 << 24))
    request.send(jlink)

    try:
        # 7. Poll until the Flash-ready bit is set in the status register flags.
        #    Have to read first to ensure the data is valid.
        unlock_kinetis_read_until_ack(jlink, 0x0)
        flags = registers.MDMAPStatusRegisterFlags()
        flags.flash_ready = 0
        while not flags.flash_ready:
            flags.value = unlock_kinetis_read_until_ack(jlink, 0x0).data

        # 8. System may still be secure at this point, so request a mass erase.
        #    AP[1] bank 0, register 1 is the MDM-AP Control Register.
        flags = registers.MDMAPControlRegisterFlags()
        flags.flash_mass_erase = 1
        request = swd.WriteRequest(0x1, True, flags.value)
        request.send(jlink)

        # 9. Poll the status register until the mass erase command has been
        #    accepted.
        unlock_kinetis_read_until_ack(jlink, 0x0)
        flags = registers.MDMAPStatusRegisterFlags()
        flags.flash_mass_erase_ack = 0
        while not flags.flash_mass_erase_ack:
            flags.value = unlock_kinetis_read_until_ack(jlink, 0x0).data

        # 10. Poll the control register until the ``flash_mass_erase`` bit is
        #     cleared, which is done automatically when the mass erase
        #     finishes.
        unlock_kinetis_read_until_ack(jlink, 0x1)
        flags = registers.MDMAPControlRegisterFlags()
        flags.flash_mass_erase = 1
        while flags.flash_mass_erase:
            flags.value = unlock_kinetis_read_until_ack(jlink, 0x1).data

    except KinetisException as e:
        jlink.set_reset_pin_high()
        return False

    jlink.set_reset_pin_high()
    time.sleep(1)

    jlink.reset()

    return True


UNLOCK_METHODS[enums.JLinkInterfaces.SWD] = unlock_kinetis_swd


def unlock_kinetis_jtag(jlink):
    """Unlocks a Kinetis device over JTAG.

    Note:
      Currently not implemented.

    Args:
      jlink (JLink): the connected J-Link

    Returns:
      ``True`` if the device was unlocked successfully, otherwise ``False``.

    Raises:
      NotImplementedError: always.
    """
    JTAGIdentity = Identity(0x4, 0xBA0)
    raise NotImplementedError('Unlock Kinetis over JTAG is not implemented.')


UNLOCK_METHODS[enums.JLinkInterfaces.JTAG] = unlock_kinetis_jtag


@decorators.async_decorator
def unlock_kinetis(jlink):
    """Unlock for Freescale Kinetis K40 or K60 device.

    Args:
      jlink (JLink): an instance of a J-Link that is connected to a target.

    Returns:
      ``True`` if the device was successfully unlocked, otherwise ``False``.

    Raises:
      ValueError: if the J-Link is not connected to a target.
    """
    if not jlink.connected():
        raise ValueError('No target to unlock.')

    method = UNLOCK_METHODS.get(jlink.tif, None)
    if method is None:
        raise NotImplementedError('Unsupported target interface for unlock.')

    return method(jlink)


__all__ = ['unlock_kinetis']
