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
from . import util


class JLinkException(enums.JLinkGlobalErrors, Exception):
    """Generic J-Link exception."""

    def __init__(self, code):
        """Generates an exception by coercing the given ``code`` to an error
        string if is a number, otherwise assumes it is the message.

        Args:
          self (JLinkException): the 'JLinkException' instance
          code (object): message or error code

        Returns:
          ``None``
        """
        message = code

        self.code = None

        if util.is_integer(code):
            message = self.to_string(code)
            self.code = code

        super(JLinkException, self).__init__(message)
        self.message = message


class JLinkEraseException(enums.JLinkEraseErrors, JLinkException):
    """J-Link erase exception."""
    pass


class JLinkFlashException(enums.JLinkFlashErrors, JLinkException):
    """J-Link flash exception."""
    pass


class JLinkWriteException(enums.JLinkWriteErrors, JLinkException):
    """J-Link write exception."""
    pass


class JLinkReadException(enums.JLinkReadErrors, JLinkException):
    """J-Link read exception."""
    pass


class JLinkDataException(enums.JLinkDataErrors, JLinkException):
    """J-Link data event exception."""
    pass


class JLinkRTTException(enums.JLinkRTTErrors, JLinkException):
    """J-Link RTT exception."""
    pass
