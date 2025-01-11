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

from . import threads

import functools


def async_decorator(func):
    """Asynchronous function decorator.  Interprets the function as being
    asynchronous, so returns a function that will handle calling the
    Function asynchronously.

    Args:
      func (function): function to be called asynchronously

    Returns:
      The wrapped function.

    Raises:
      AttributeError: if ``func`` is not callable
    """

    @functools.wraps(func)
    def async_wrapper(*args, **kwargs):
        """Wraps up the call to ``func``, so that it is called from a separate
        thread.

        The callback, if given, will be called with two parameters,
        ``exception`` and ``result`` as ``callback(exception, result)``.  If
        the thread ran to completion without error, ``exception`` will be
        ``None``, otherwise ``exception`` will be the generated exception that
        stopped the thread.  Result is the result of the exected function.

        Args:
          callback (function): the callback to ultimately be called
          args: list of arguments to pass to ``func``
          kwargs: key-word arguments dictionary to pass to ``func``

        Returns:
          A thread if the call is asynchronous, otherwise the the return value
          of the wrapped function.

        Raises:
          TypeError: if ``callback`` is not callable or is missing
        """
        if 'callback' not in kwargs or not kwargs['callback']:
            return func(*args, **kwargs)

        callback = kwargs.pop('callback')

        if not callable(callback):
            raise TypeError('Expected \'callback\' is not callable.')

        def thread_func(*args, **kwargs):
            """Thread function on which the given ``func`` and ``callback``
            are executed.

            Args:
              args: list of arguments to pass to ``func``
              kwargs: key-word arguments dictionary to pass to ``func``

            Returns:
              Return value of the wrapped function.
            """
            exception, res = None, None
            try:
                res = func(*args, **kwargs)
            except Exception as e:
                exception = e
            return callback(exception, res)

        thread = threads.ThreadReturn(target=thread_func,
                                      args=args,
                                      kwargs=kwargs)
        thread.daemon = True
        thread.start()
        return thread

    return async_wrapper
