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

import threading


class ThreadReturn(threading.Thread):
    """Implementation of a thread with a return value.

    See also:
      `StackOverflow <http://stackoverflow.com/questions/6893968/>`__.
    """

    def __init__(self, daemon=False, *args, **kwargs):
        """Initializes the thread.

        Args:
          self (ThreadReturn): the ``ThreadReturn`` instance
          daemon (bool): if the thread should be spawned as a daemon
          args: optional list of arguments
          kwargs: optional key-word arguments

        Returns:
          ``None``
        """
        super(ThreadReturn, self).__init__(*args, **kwargs)
        self.daemon = daemon
        self._return = None

    def run(self):
        """Runs the thread.

        Args:
          self (ThreadReturn): the ``ThreadReturn`` instance

        Returns:
          ``None``
        """
        target = getattr(self, '_Thread__target', getattr(self, '_target', None))
        args = getattr(self, '_Thread__args', getattr(self, '_args', None))
        kwargs = getattr(self, '_Thread__kwargs', getattr(self, '_kwargs', None))
        if target is not None:
            self._return = target(*args, **kwargs)

        return None

    def join(self, *args, **kwargs):
        """Joins the thread.

        Args:
          self (ThreadReturn): the ``ThreadReturn`` instance
          args: optional list of arguments
          kwargs: optional key-word arguments

        Returns:
          The return value of the exited thread.
        """
        super(ThreadReturn, self).join(*args, **kwargs)
        return self._return
