from canopen.network import Network, NodeScanner
from canopen.node import RemoteNode, LocalNode
from canopen.sdo import SdoCommunicationError, SdoAbortedError
from canopen.objectdictionary import import_od, export_od, ObjectDictionary, ObjectDictionaryError
from canopen.profiles.p402 import BaseNode402
try:
    from canopen._version import version as __version__
except ImportError:
    # package is not installed
    __version__ = "unknown"

Node = RemoteNode

__pypi_url__ = "https://pypi.org/project/canopen/"
