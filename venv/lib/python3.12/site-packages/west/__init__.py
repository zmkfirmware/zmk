# Copyright (c) 2021 Nordic Semiconductor ASA

import logging

# https://docs.python.org/3/howto/logging.html#configuring-logging-for-a-library
logging.getLogger('west').addHandler(logging.NullHandler())
