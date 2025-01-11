"""Additional utility functions for canopen."""
from typing import Optional, Union


def pretty_index(index: Optional[Union[int, str]],
                 sub: Optional[Union[int, str]] = None):
    """Format an index and subindex as a string."""

    index_str = ""
    if isinstance(index, int):
        index_str = f"0x{index:04X}"
    elif index:
        index_str = f"{index!r}"

    sub_str = ""
    if isinstance(sub, int):
        # Need 0x prefix if index is not present
        sub_str = f"{'0x' if not index_str else ''}{sub:02X}"
    elif sub:
        sub_str = f"{sub!r}"

    return ":".join(s for s in (index_str, sub_str) if s)
