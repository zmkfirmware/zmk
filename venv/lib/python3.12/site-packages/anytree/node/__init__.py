"""
Node Classes.

* :any:`AnyNode`: a generic tree node with any number of attributes.
* :any:`Node`: a simple tree node with at least a name attribute and any number of additional attributes.
* :any:`NodeMixin`: extends any python class to a tree node.
* :any:`SymlinkNode`: Tree node which references to another tree node.
* :any:`SymlinkNodeMixin`: extends any Python class to a symbolic link to a tree node.
* :any:`LightNodeMixin`: A :any:`NodeMixin` using slots.
"""

from .anynode import AnyNode  # noqa
from .exceptions import LoopError  # noqa
from .exceptions import TreeError  # noqa
from .lightnodemixin import LightNodeMixin  # noqa
from .node import Node  # noqa
from .nodemixin import NodeMixin  # noqa
from .symlinknode import SymlinkNode  # noqa
from .symlinknodemixin import SymlinkNodeMixin  # noqa
