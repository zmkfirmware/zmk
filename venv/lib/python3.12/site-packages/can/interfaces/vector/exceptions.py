"""Exception/error declarations for the vector interface."""

from can import CanError, CanInitializationError, CanOperationError


class VectorError(CanError):
    def __init__(self, error_code, error_string, function):
        super().__init__(
            message=f"{function} failed ({error_string})", error_code=error_code
        )

        # keep reference to args for pickling
        self._args = error_code, error_string, function

    def __reduce__(self):
        return type(self), self._args, {}


class VectorInitializationError(VectorError, CanInitializationError):
    @staticmethod
    def from_generic(error: VectorError) -> "VectorInitializationError":
        return VectorInitializationError(*error._args)


class VectorOperationError(VectorError, CanOperationError):
    @staticmethod
    def from_generic(error: VectorError) -> "VectorOperationError":
        return VectorOperationError(*error._args)
