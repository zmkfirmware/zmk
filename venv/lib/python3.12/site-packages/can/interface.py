"""
This module contains the base implementation of :class:`can.BusABC` as well
as a list of all available backends and some implemented
CyclicSendTasks.
"""

import importlib
import logging
from typing import Any, Iterable, List, Optional, Type, Union, cast

from . import util
from .bus import BusABC
from .exceptions import CanInterfaceNotImplementedError
from .interfaces import BACKENDS
from .typechecking import AutoDetectedConfig, Channel

log = logging.getLogger("can.interface")
log_autodetect = log.getChild("detect_available_configs")


def _get_class_for_interface(interface: str) -> Type[BusABC]:
    """
    Returns the main bus class for the given interface.

    :raises:
        NotImplementedError if the interface is not known
    :raises CanInterfaceNotImplementedError:
         if there was a problem while importing the interface or the bus class within that
    """
    # Find the correct backend
    try:
        module_name, class_name = BACKENDS[interface]
    except KeyError:
        raise NotImplementedError(
            f"CAN interface '{interface}' not supported"
        ) from None

    # Import the correct interface module
    try:
        module = importlib.import_module(module_name)
    except Exception as e:
        raise CanInterfaceNotImplementedError(
            f"Cannot import module {module_name} for CAN interface '{interface}': {e}"
        ) from None

    # Get the correct class
    try:
        bus_class = getattr(module, class_name)
    except Exception as e:
        raise CanInterfaceNotImplementedError(
            f"Cannot import class {class_name} from module {module_name} for CAN interface "
            f"'{interface}': {e}"
        ) from None

    return cast(Type[BusABC], bus_class)


@util.deprecated_args_alias(
    deprecation_start="4.2.0",
    deprecation_end="5.0.0",
    bustype="interface",
    context="config_context",
)
def Bus(  # noqa: N802
    channel: Optional[Channel] = None,
    interface: Optional[str] = None,
    config_context: Optional[str] = None,
    ignore_config: bool = False,
    **kwargs: Any,
) -> BusABC:
    """Create a new bus instance with configuration loading.

    Instantiates a CAN Bus of the given ``interface``, falls back to reading a
    configuration file from default locations.

    .. note::
        Please note that while the arguments provided to this class take precedence
        over any existing values from configuration, it is possible that other parameters
        from the configuration may be added to the bus instantiation.
        This could potentially have unintended consequences. To prevent this,
        you may use the *ignore_config* parameter to ignore any existing configurations.

    :param channel:
        Channel identification. Expected type is backend dependent.
        Set to ``None`` to let it be resolved automatically from the default
        :ref:`configuration`.

    :param interface:
        See :ref:`interface names` for a list of supported interfaces.
        Set to ``None`` to let it be resolved automatically from the default
        :ref:`configuration`.

    :param config_context:
        Extra 'context', that is passed to config sources.
        This can be used to select a section other than 'default' in the configuration file.

    :param ignore_config:
        If ``True``, only the given arguments will be used for the bus instantiation. Existing
        configuration sources will be ignored.

    :param kwargs:
        ``interface`` specific keyword arguments.

    :raises ~can.exceptions.CanInterfaceNotImplementedError:
        if the ``interface`` isn't recognized or cannot be loaded

    :raises ~can.exceptions.CanInitializationError:
        if the bus cannot be instantiated

    :raises ValueError:
        if the ``channel`` could not be determined
    """

    # figure out the rest of the configuration; this might raise an error
    if interface is not None:
        kwargs["interface"] = interface
    if channel is not None:
        kwargs["channel"] = channel

    if not ignore_config:
        kwargs = util.load_config(config=kwargs, context=config_context)

    # resolve the bus class to use for that interface
    cls = _get_class_for_interface(kwargs["interface"])

    # remove the "interface" key, so it doesn't get passed to the backend
    del kwargs["interface"]

    # make sure the bus can handle this config format
    channel = kwargs.pop("channel", channel)
    if channel is None:
        # Use the default channel for the backend
        bus = cls(**kwargs)
    else:
        bus = cls(channel, **kwargs)

    return bus


def detect_available_configs(
    interfaces: Union[None, str, Iterable[str]] = None
) -> List[AutoDetectedConfig]:
    """Detect all configurations/channels that the interfaces could
    currently connect with.

    This might be quite time-consuming.

    Automated configuration detection may not be implemented by
    every interface on every platform. This method will not raise
    an error in that case, but with rather return an empty list
    for that interface.

    :param interfaces: either
        - the name of an interface to be searched in as a string,
        - an iterable of interface names to search in, or
        - `None` to search in all known interfaces.
    :rtype: list[dict]
    :return: an iterable of dicts, each suitable for usage in
             the constructor of :class:`can.BusABC`.
    """

    # Figure out where to search
    if interfaces is None:
        interfaces = BACKENDS
    elif isinstance(interfaces, str):
        interfaces = (interfaces,)
    # else it is supposed to be an iterable of strings

    result = []
    for interface in interfaces:
        try:
            bus_class = _get_class_for_interface(interface)
        except CanInterfaceNotImplementedError:
            log_autodetect.debug(
                'interface "%s" cannot be loaded for detection of available configurations',
                interface,
            )
            continue

        # get available channels
        try:
            available = list(
                bus_class._detect_available_configs()  # pylint: disable=protected-access
            )
        except NotImplementedError:
            log_autodetect.debug(
                'interface "%s" does not support detection of available configurations',
                interface,
            )
        else:
            log_autodetect.debug(
                'interface "%s" detected %i available configurations',
                interface,
                len(available),
            )

            # add the interface name to the configs if it is not already present
            for config in available:
                if "interface" not in config:
                    config["interface"] = interface

            # append to result
            result += available

    return result
