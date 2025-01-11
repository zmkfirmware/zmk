"""
Convert a log file from one format to another.
"""

import argparse
import errno
import sys

from can import Logger, LogReader, SizedRotatingLogger


class ArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        self.print_help(sys.stderr)
        self.exit(errno.EINVAL, f"{self.prog}: error: {message}\n")


def main():
    parser = ArgumentParser(
        description="Convert a log file from one format to another.",
    )

    parser.add_argument(
        "-s",
        "--file_size",
        dest="file_size",
        type=int,
        help="Maximum file size in bytes. Rotate log file when size threshold is reached.",
        default=None,
    )

    parser.add_argument(
        "input",
        metavar="INFILE",
        type=str,
        help="Input filename. The type is dependent on the suffix, see can.LogReader.",
    )

    parser.add_argument(
        "output",
        metavar="OUTFILE",
        type=str,
        help="Output filename. The type is dependent on the suffix, see can.Logger.",
    )

    args = parser.parse_args()

    with LogReader(args.input) as reader:
        if args.file_size:
            logger = SizedRotatingLogger(
                base_filename=args.output, max_bytes=args.file_size
            )
        else:
            logger = Logger(filename=args.output)

        with logger:
            try:
                for m in reader:
                    logger(m)
            except KeyboardInterrupt:
                sys.exit(1)


if __name__ == "__main__":
    main()
