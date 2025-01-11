from typing import TextIO, Union
from canopen.objectdictionary import ObjectDictionary, import_od


class BaseNode:
    """A CANopen node.

    :param node_id:
        Node ID (set to None or 0 if specified by object dictionary)
    :param object_dictionary:
        Object dictionary as either a path to a file, an ``ObjectDictionary``
        or a file like object.
    """

    def __init__(
        self,
        node_id: int,
        object_dictionary: Union[ObjectDictionary, str, TextIO],
    ):
        self.network = None

        if not isinstance(object_dictionary, ObjectDictionary):
            object_dictionary = import_od(object_dictionary, node_id)
        self.object_dictionary = object_dictionary

        self.id = node_id or self.object_dictionary.node_id
