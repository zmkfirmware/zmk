

class SdoError(Exception):
    pass


class SdoAbortedError(SdoError):
    """SDO abort exception."""

    CODES = {
        0x05030000: "SDO toggle bit error",
        0x05040000: "Timeout of transfer communication detected",
        0x05040001: "Unknown SDO command specified",
        0x05040002: "Invalid block size",
        0x05040003: "Invalid sequence number",
        0x05040004: "CRC error",
        0x05040005: "Out of memory",
        0x06010000: "Unsupported access to an object",
        0x06010001: "Attempt to read a write only object",
        0x06010002: "Attempt to write a read only object",
        0x06020000: "Object does not exist",
        0x06040041: "Object cannot be mapped to the PDO",
        0x06040042: "PDO length exceeded",
        0x06040043: "General parameter incompatibility reason",
        0x06040047: "General internal incompatibility in the device",
        0x06060000: "Access failed due to a hardware error",
        0x06070010: "Data type and length code do not match",
        0x06070012: "Data type does not match, length of service parameter too high",
        0x06070013: "Data type does not match, length of service parameter too low",
        0x06090011: "Subindex does not exist",
        0x06090030: "Value range of parameter exceeded",
        0x06090031: "Value of parameter written too high",
        0x06090032: "Value of parameter written too low",
        0x06090036: "Maximum value is less than minimum value",
        0x060A0023: "Resource not available",
        0x08000000: "General error",
        0x08000020: "Data cannot be transferred or stored to the application",
        0x08000021: ("Data can not be transferred or stored to the application "
                     "because of local control"),
        0x08000022: ("Data can not be transferred or stored to the application "
                     "because of the present device state"),
        0x08000023: ("Object dictionary dynamic generation fails or no object "
                     "dictionary is present"),
        0x08000024: "No data available",
    }

    def __init__(self, code: int):
        #: Abort code
        self.code = code

    def __str__(self):
        text = f"Code 0x{self.code:08X}"
        if self.code in self.CODES:
            text = text + ", " + self.CODES[self.code]
        return text

    def __eq__(self, other):
        """Compare two exception objects based on SDO abort code."""
        return self.code == other.code


class SdoCommunicationError(SdoError):
    """No or unexpected response from slave."""
