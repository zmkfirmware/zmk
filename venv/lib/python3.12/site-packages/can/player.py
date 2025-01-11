"""
Replays CAN traffic saved with can.logger back
to a CAN bus.

Similar to canplayer in the can-utils package.
"""

import argparse
import errno
import sys
from datetime import datetime
from typing import Iterable, cast

from can import LogReader, Message, MessageSync

from .logger import _create_base_argument_parser, _create_bus, _parse_additional_config


def main() -> None:
    parser = argparse.ArgumentParser(description="Replay CAN traffic.")

    _create_base_argument_parser(parser)

    parser.add_argument(
        "-f",
        "--file_name",
        dest="log_file",
        help="Path and base log filename, for supported types see can.LogReader.",
        default=None,
    )

    parser.add_argument(
        "-v",
        action="count",
        dest="verbosity",
        help="""Also print can frames to stdout.
                        You can add several of these to enable debugging""",
        default=2,
    )

    parser.add_argument(
        "--ignore-timestamps",
        dest="timestamps",
        help="""Ignore timestamps (send all frames immediately with minimum gap between frames)""",
        action="store_false",
    )

    parser.add_argument(
        "--error-frames",
        help="Also send error frames to the interface.",
        action="store_true",
    )

    parser.add_argument(
        "-g",
        "--gap",
        type=float,
        help="<s> minimum time between replayed frames",
        default=0.0001,
    )
    parser.add_argument(
        "-s",
        "--skip",
        type=float,
        default=60 * 60 * 24,
        help="<s> skip gaps greater than 's' seconds",
    )

    parser.add_argument(
        "infile",
        metavar="input-file",
        type=str,
        help="The file to replay. For supported types see can.LogReader.",
    )

    # print help message when no arguments were given
    if len(sys.argv) < 2:
        parser.print_help(sys.stderr)
        raise SystemExit(errno.EINVAL)

    results, unknown_args = parser.parse_known_args()
    additional_config = _parse_additional_config([*results.extra_args, *unknown_args])

    verbosity = results.verbosity

    error_frames = results.error_frames

    with _create_bus(results, **additional_config) as bus:
        with LogReader(results.infile, **additional_config) as reader:
            in_sync = MessageSync(
                cast(Iterable[Message], reader),
                timestamps=results.timestamps,
                gap=results.gap,
                skip=results.skip,
            )

            print(f"Can LogReader (Started on {datetime.now()})")

            try:
                for message in in_sync:
                    if message.is_error_frame and not error_frames:
                        continue
                    if verbosity >= 3:
                        print(message)
                    bus.send(message)
            except KeyboardInterrupt:
                pass


if __name__ == "__main__":
    main()
