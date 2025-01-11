# Copyright (c) 2019, Nordic Semiconductor ASA
#
# Don't put anything else in here!
#
# This is the Python 3 version of option 3 in:
# https://packaging.python.org/guides/single-sourcing-package-version/#single-sourcing-the-version

import importlib.metadata

__version__ = importlib.metadata.version("west")
#
# MAINTAINERS:
#
# Make sure to update west.manifest.SCHEMA_VERSION if there have been
# manifest schema version changes since the last release.
#
# Note that this is the "logical" west manifest schema, and that the
# pykwalify schema doesn't capture everything. E.g. the map in an
# "import: <map>" is validated in west.manifest without pykwalify's
# help.
