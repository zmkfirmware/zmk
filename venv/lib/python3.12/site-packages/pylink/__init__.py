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

__version__ = '1.4.0'
__title__ = 'pylink'
__author__ = 'Square Embedded Software Team'
__author_email__ = 'esw-team@squareup.com'
__copyright__ = 'Copyright 2017 Square, Inc.'
__license__ = 'Apache 2.0'
__url__ = 'http://www.github.com/Square/pylink'
__description__ = 'Python interface for SEGGER J-Link.'
__long_description__ = '''This module provides a Python implementation of the
J-Link SDK by leveraging the SDK's DLL.
'''

from .enums import *
from .errors import *
from .jlink import *
from .library import *
from .structs import *
from .unlockers import *
