import argparse
import errno
import re
import sys
from datetime import datetime
from typing import (
    TYPE_CHECKING,
    Any,
    Dict,
    List,
    Optional,
    Sequence,
    Tuple,
    Union,
)

import can
from can import Bus, BusState, Logger, SizedRotatingLogger
from can.typechecking import TAdditionalCliArgs
from can.util import _dict2timing, cast_from_string

if TYPE_CHECKING:
    from can.io import BaseRotatingLogger
    from can.io.generic import MessageWriter
    from can.typechecking import CanFilter


def _create_base_argument_parser(parser: argparse.ArgumentParser) -> None:
    """Adds common options to an argument parser."""

    parser.add_argument(
        "-c",
        "--channel",
        help=r"Most backend interfaces require some sort of channel. For "
        r"example with the serial interface the channel might be a rfcomm"
        r' device: "/dev/rfcomm0". With the socketcan interface valid '
        r'channel examples include: "can0", "vcan0".',
    )

    parser.add_argument(
        "-i",
        "--interface",
        dest="interface",
        help="""Specify the backend CAN interface to use. If left blank,
                        fall back to reading from configuration files.""",
        choices=sorted(can.VALID_INTERFACES),
    )

    parser.add_argument(
        "-b", "--bitrate", type=int, help="Bitrate to use for the CAN bus."
    )

    parser.add_argument("--fd", help="Activate CAN-FD support", action="store_true")

    parser.add_argument(
        "--data_bitrate",
        type=int,
        help="Bitrate to use for the data phase in case of CAN-FD.",
    )

    parser.add_argument(
        "--timing",
        action=_BitTimingAction,
        nargs=argparse.ONE_OR_MORE,
        help="Configure bit rate and bit timing. For example, use "
        "`--timing f_clock=8_000_000 tseg1=5 tseg2=2 sjw=2 brp=2 nof_samples=1` for classical CAN "
        "or `--timing f_clock=80_000_000 nom_tseg1=119 nom_tseg2=40 nom_sjw=40 nom_brp=1 "
        "data_tseg1=29 data_tseg2=10 data_sjw=10 data_brp=1` for CAN FD. "
        "Check the python-can documentation to verify whether your "
        "CAN interface supports the `timing` argument.",
        metavar="TIMING_ARG",
    )

    parser.add_argument(
        "extra_args",
        nargs=argparse.REMAINDER,
        help="The remaining arguments will be used for the interface and "
        "logger/player initialisation. "
        "For example, `-i vector -c 1 --app-name=MyCanApp` is the equivalent "
        "to opening the bus with `Bus('vector', channel=1, app_name='MyCanApp')",
    )


def _append_filter_argument(
    parser: Union[argparse.ArgumentParser, argparse._ArgumentGroup],
    *args: str,
    **kwargs: Any,
) -> None:
    """Adds the ``filter`` option to an argument parser."""

    parser.add_argument(
        *args,
        "--filter",
        help="R|Space separated CAN filters for the given CAN interface:"
        "\n      <can_id>:<can_mask> (matches when <received_can_id> & mask =="
        " can_id & mask)"
        "\n      <can_id>~<can_mask> (matches when <received_can_id> & mask !="
        " can_id & mask)"
        "\nFx to show only frames with ID 0x100 to 0x103 and 0x200 to 0x20F:"
        "\n      python -m can.viewer --filter 100:7FC 200:7F0"
        "\nNote that the ID and mask are always interpreted as hex values",
        metavar="{<can_id>:<can_mask>,<can_id>~<can_mask>}",
        nargs=argparse.ONE_OR_MORE,
        action=_CanFilterAction,
        dest="can_filters",
        **kwargs,
    )


def _create_bus(parsed_args: argparse.Namespace, **kwargs: Any) -> can.BusABC:
    logging_level_names = ["critical", "error", "warning", "info", "debug", "subdebug"]
    can.set_logging_level(logging_level_names[min(5, parsed_args.verbosity)])

    config: Dict[str, Any] = {"single_handle": True, **kwargs}
    if parsed_args.interface:
        config["interface"] = parsed_args.interface
    if parsed_args.bitrate:
        config["bitrate"] = parsed_args.bitrate
    if parsed_args.fd:
        config["fd"] = True
    if parsed_args.data_bitrate:
        config["data_bitrate"] = parsed_args.data_bitrate
    if getattr(parsed_args, "can_filters", None):
        config["can_filters"] = parsed_args.can_filters
    if parsed_args.timing:
        config["timing"] = parsed_args.timing

    return Bus(parsed_args.channel, **config)


class _CanFilterAction(argparse.Action):
    def __call__(
        self,
        parser: argparse.ArgumentParser,
        namespace: argparse.Namespace,
        values: Union[str, Sequence[Any], None],
        option_string: Optional[str] = None,
    ) -> None:
        if not isinstance(values, list):
            raise argparse.ArgumentError(None, "Invalid filter argument")

        print(f"Adding filter(s): {values}")
        can_filters: List[CanFilter] = []

        for filt in values:
            if ":" in filt:
                parts = filt.split(":")
                can_id = int(parts[0], base=16)
                can_mask = int(parts[1], base=16)
            elif "~" in filt:
                parts = filt.split("~")
                can_id = int(parts[0], base=16) | 0x20000000  # CAN_INV_FILTER
                can_mask = int(parts[1], base=16) & 0x20000000  # socket.CAN_ERR_FLAG
            else:
                raise argparse.ArgumentError(None, "Invalid filter argument")
            can_filters.append({"can_id": can_id, "can_mask": can_mask})

        setattr(namespace, self.dest, can_filters)


class _BitTimingAction(argparse.Action):
    def __call__(
        self,
        parser: argparse.ArgumentParser,
        namespace: argparse.Namespace,
        values: Union[str, Sequence[Any], None],
        option_string: Optional[str] = None,
    ) -> None:
        if not isinstance(values, list):
            raise argparse.ArgumentError(None, "Invalid --timing argument")

        timing_dict: Dict[str, int] = {}
        for arg in values:
            try:
                key, value_string = arg.split("=")
                value = int(value_string)
                timing_dict[key] = value
            except ValueError:
                raise argparse.ArgumentError(
                    None, f"Invalid timing argument: {arg}"
                ) from None

        if not (timing := _dict2timing(timing_dict)):
            err_msg = "Invalid --timing argument. Incomplete parameters."
            raise argparse.ArgumentError(None, err_msg)

        setattr(namespace, self.dest, timing)
        print(timing)


def _parse_additional_config(unknown_args: Sequence[str]) -> TAdditionalCliArgs:
    for arg in unknown_args:
        if not re.match(r"^--[a-zA-Z][a-zA-Z0-9\-]*=\S*?$", arg):
            raise ValueError(f"Parsing argument {arg} failed")

    def _split_arg(_arg: str) -> Tuple[str, str]:
        left, right = _arg.split("=", 1)
        return left.lstrip("-").replace("-", "_"), right

    args: Dict[str, Union[str, int, float, bool]] = {}
    for key, string_val in map(_split_arg, unknown_args):
        args[key] = cast_from_string(string_val)
    return args


def _parse_logger_args(
    args: List[str],
) -> Tuple[argparse.Namespace, TAdditionalCliArgs]:
    """Parse command line arguments for logger script."""

    parser = argparse.ArgumentParser(
        description="Log CAN traffic, printing messages to stdout or to a "
        "given file.",
    )

    # Generate the standard arguments:
    # Channel, bitrate, data_bitrate, interface, app_name, CAN-FD support
    _create_base_argument_parser(parser)

    parser.add_argument(
        "-f",
        "--file_name",
        dest="log_file",
        help="Path and base log filename, for supported types see can.Logger.",
        default=None,
    )

    parser.add_argument(
        "-a",
        "--append",
        dest="append",
        help="Append to the log file if it already exists.",
        action="store_true",
    )

    parser.add_argument(
        "-s",
        "--file_size",
        dest="file_size",
        type=int,
        help="Maximum file size in bytes. Rotate log file when size threshold "
        "is reached. (The resulting file sizes will be consistent, but are not "
        "guaranteed to be exactly what is specified here due to the rollover "
        "conditions being logger implementation specific.)",
        default=None,
    )

    parser.add_argument(
        "-v",
        action="count",
        dest="verbosity",
        help="""How much information do you want to see at the command line?
                        You can add several of these e.g., -vv is DEBUG""",
        default=2,
    )

    _append_filter_argument(parser)

    state_group = parser.add_mutually_exclusive_group(required=False)
    state_group.add_argument(
        "--active",
        help="Start the bus as active, this is applied by default.",
        action="store_true",
    )
    state_group.add_argument(
        "--passive", help="Start the bus as passive.", action="store_true"
    )

    # print help message when no arguments were given
    if not args:
        parser.print_help(sys.stderr)
        raise SystemExit(errno.EINVAL)

    results, unknown_args = parser.parse_known_args(args)
    additional_config = _parse_additional_config([*results.extra_args, *unknown_args])
    return results, additional_config


def main() -> None:
    results, additional_config = _parse_logger_args(sys.argv[1:])
    bus = _create_bus(results, **additional_config)

    if results.active:
        bus.state = BusState.ACTIVE
    elif results.passive:
        bus.state = BusState.PASSIVE

    print(f"Connected to {bus.__class__.__name__}: {bus.channel_info}")
    print(f"Can Logger (Started on {datetime.now()})")

    logger: Union[MessageWriter, BaseRotatingLogger]
    if results.file_size:
        logger = SizedRotatingLogger(
            base_filename=results.log_file,
            max_bytes=results.file_size,
            append=results.append,
            **additional_config,
        )
    else:
        logger = Logger(
            filename=results.log_file,
            append=results.append,
            **additional_config,
        )

    try:
        while True:
            msg = bus.recv(1)
            if msg is not None:
                logger(msg)
    except KeyboardInterrupt:
        pass
    finally:
        bus.shutdown()
        logger.stop()


if __name__ == "__main__":
    main()
