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

from .unlock_kinetis import unlock_kinetis


def unlock(jlink, name):
    """Unlocks a J-Link's target device.

    Args:
      jlink (JLink): the connected J-Link device
      name (str): the MCU name (e.g. Kinetis)

    Supported Names:
      - Kinetis

    Returns:
      ``True`` if the device was unlocked, otherwise ``False``.

    Raises:
      NotImplementedError: if no unlock method exists for the MCU.
    """
    if name.lower() in ['kinetis', 'freescale', 'nxp']:
        return unlock_kinetis(jlink)
    raise NotImplementedError('No unlock method for %s' % name)
