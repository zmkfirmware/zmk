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

import psutil

import errno
import tempfile
import os


class JLock(object):
    """Lockfile for accessing a particular J-Link.

    The J-Link SDK does not prevent accessing the same J-Link multiple times
    from the same process or multiple processes.  As a result, a user can
    have the same J-Link being accessed by multiple processes.  This class
    provides an interface to a lock-file like structure for the physical
    J-Links to ensure that any instance of a ``JLink`` with an open emulator
    connection will be the only one accessing that emulator.

    This class uses a PID-style lockfile to allow acquiring of the lockfile in
    the instances where the lockfile exists, but the process which created it
    is no longer running.

    To share the same emulator connection between multiple threads, processes,
    or functions, a single instance of a ``JLink`` should be created and passed
    between the threads and processes.

    Attributes:
      name: the name of the lockfile.
      path: full path to the lockfile.
      fd: file description of the lockfile.
      acquired: boolean indicating if the lockfile lock has been acquired.
    """

    SERIAL_NAME_FMT = '.pylink-usb-{}.lck'
    IPADDR_NAME_FMT = '.pylink-ip-{}.lck'

    def __init__(self, serial_no):
        """Creates an instance of a ``JLock`` and populates the name.

        Note:
          This method may fail if there is no temporary directory in which to
          have the lockfile placed.

        Args:
          self (JLock): the ``JLock`` instance
          serial_no (int): the serial number of the J-Link

        Returns:
          ``None``
        """
        self.name = self.SERIAL_NAME_FMT.format(serial_no)
        self.acquired = False
        self.fd = None
        self.path = None
        self.path = os.path.join(tempfile.gettempdir(), self.name)

    def __del__(self):
        """Cleans up the lockfile instance if it was acquired.

        Args:
          self (JLock): the ``JLock`` instance

        Returns:
          ``None``
        """
        self.release()

    def acquire(self):
        """Attempts to acquire a lock for the J-Link lockfile.

        If the lockfile exists but does not correspond to an active process,
        the lockfile is first removed, before an attempt is made to acquire it.

        Args:
          self (Jlock): the ``JLock`` instance

        Returns:
          ``True`` if the lock was acquired, otherwise ``False``.

        Raises:
          OSError: on file errors.
        """
        if os.path.exists(self.path):
            try:
                pid = None

                with open(self.path, 'r') as f:
                    line = f.readline().strip()
                    pid = int(line)

                # In the case that the lockfile exists, but the pid does not
                # correspond to a valid process, remove the file.
                if not psutil.pid_exists(pid):
                    os.remove(self.path)

            except ValueError as e:
                # Pidfile is invalid, so just delete it.
                os.remove(self.path)

            except IOError as e:
                # Something happened while trying to read/remove the file, so
                # skip trying to read/remove it.
                pass

        try:
            self.fd = os.open(self.path, os.O_CREAT | os.O_EXCL | os.O_RDWR)

            # PID is written to the file, so that if a process exits wtihout
            # cleaning up the lockfile, we can still acquire the lock.
            to_write = '%s%s' % (os.getpid(), os.linesep)
            os.write(self.fd, to_write.encode())

        except OSError as e:
            if not os.path.exists(self.path):
                raise
            return False

        self.acquired = True
        return True

    def release(self):
        """Cleans up the lockfile if it was acquired.

        Args:
          self (JLock): the ``JLock`` instance

        Returns:
          ``False`` if the lock was not released or the lock is not acquired,
          otherwise ``True``.
        """
        if not self.acquired:
            return False

        os.close(self.fd)

        if os.path.exists(self.path):
            os.remove(self.path)

        self.acquired = False
        return True
