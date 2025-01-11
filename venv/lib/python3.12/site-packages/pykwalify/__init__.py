# -*- coding: utf-8 -*-

""" pykwalify """

# python stdlib
import logging
import logging.config
import os

__author__ = 'Grokzen <Grokzen@gmail.com>'
__version_info__ = (1, 8, 0)
__version__ = '.'.join(map(str, __version_info__))


log_level_to_string_map = {
    5: "DEBUG",
    4: "INFO",
    3: "WARNING",
    2: "ERROR",
    1: "CRITICAL",
    0: "INFO"
}


def init_logging(log_level):
    """
    Init logging settings with default set to INFO
    """
    log_level = log_level_to_string_map[min(log_level, 5)]

    msg = "%(levelname)s - %(name)s:%(lineno)s - %(message)s" if log_level in os.environ else "%(levelname)s - %(message)s"

    logging_conf = {
        "version": 1,
        "root": {
            "level": log_level,
            "handlers": ["console"]
        },
        "handlers": {
            "console": {
                "class": "logging.StreamHandler",
                "level": log_level,
                "formatter": "simple",
                "stream": "ext://sys.stdout"
            }
        },
        "formatters": {
            "simple": {
                "format": " {0}".format(msg)
            }
        }
    }

    logging.config.dictConfig(logging_conf)


partial_schemas = {}
