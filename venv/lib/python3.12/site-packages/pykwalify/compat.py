# -*- coding: utf-8 -*-

# python stdlib
import sys

# 3rd party imports
from ruamel import yaml  # NOQA: F401

# Build our global yml object that will be used in all other operations in the code
yml = yaml.YAML(typ='safe', pure=True)


if sys.version_info[0] < 3:
    # Python 2.x.x series
    basestring = basestring  # NOQA: F821
    unicode = unicode    # NOQA: F821
    bytes = str    # NOQA: F821

    def u(x):
        """ """
        return x.decode()

    def b(x):
        """ """
        return x

    def nativestr(x):
        """ """
        return x if isinstance(x, str) else x.encode('utf-8', 'replace')
else:
    # Python 3.x.x series
    basestring = str  # NOQA: F821
    unicode = str  # NOQA: F821
    bytes = bytes  # NOQA: F821

    def u(x):
        """ """
        return x

    def b(x):
        """ """
        return x.encode('latin-1') if not isinstance(x, bytes) else x

    def nativestr(x):
        """ """
        return x if isinstance(x, str) else x.decode('utf-8', 'replace')
