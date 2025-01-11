# -*- coding: utf-8 -*-

""" pyKwalify - errors.py """

# python stdlib
from pykwalify.compat import basestring

retcodes = {
    # PyKwalifyExit
    0: 'noerror',

    # UnknownError
    1: 'unknownerror',

    # SchemaError
    # e.g. when a rule or the core finds an error
    2: 'schemaerror',

    # CoreError
    # e.g. when the core finds an error that is not a SchemaError
    3: 'coreerror',

    # RuleError
    # e.g. when the rule class finds an error that is not a SchemaError, similar to CoreError
    4: 'ruleerror',

    # SchemaConflict
    # e.g. when a schema conflict occurs
    5: 'schemaconflict',

    # NotMappingError
    # e.g. when a value is not a mapping when it was expected it should be
    6: 'notmaperror',

    # NotSequenceError
    # e.g. when a value is not a sequence when it was expected it should be
    7: 'notsequenceerror',
}


retnames = dict((v, k) for (k, v) in retcodes.items())


class PyKwalifyException(RuntimeError):
    """
    """

    def __init__(self, msg=None, error_key=None, retcode=None, path=None):
        """
        Arguments:
        - `msg`: a string
        - `error_key`: a unique string that makes it easier to identify what error it is
        - `retcode`: an integer, defined in PyKwalify.errors.retcodes
        """
        self.msg = msg or ""
        self.retcode = retcode or retnames['unknownerror']
        self.retname = retcodes[retcode]
        self.error_key = error_key
        self.path = path or "/"

    def __str__(self):
        """
        """
        # <PyKwalifyException msg='foo bar' retcode=1>
        # kwargs = []
        # if self.msg:
        #        kwargs.append("msg='{0}'".format(self.msg))
        # if self.retcode != retnames['noerror']:
        #        kwargs.append("retcode=%d" % self.retcode)
        # if kwargs:
        #        kwargs.insert(0, '')
        # return "<{0}{1}>".format(self.__class__.__name__, ' '.join(kwargs))

        # <PyKwalifyException: error code 1: foo bar>
        kwargs = []
        if self.retcode != retnames['noerror']:
            kwargs.append("error code {0}".format(self.retcode))
        if self.msg:
            kwargs.append(self.msg)
        if kwargs:
            kwargs.insert(0, '')
        if self.path:
            kwargs.append("Path: '{0}'".format(self.path))
        return "<{0}{1}>".format(self.__class__.__name__, ': '.join(kwargs))

    def __repr__(self):
        """
        """
        kwargs = []
        if self.msg:
            kwargs.append("msg='{0}'".format(self.msg))
        return "{0}({1})".format(self.__class__.__name__, ', '.join(kwargs))

    def msg():
        doc = """ """

        def fget(self):
            return self._msg

        def fset(self, value):
            assert isinstance(value, basestring), "argument is not string"
            self._msg = value

        return locals()
    msg = property(**msg())

    def retcode():
        doc = """ """

        def fget(self):
            return self._retcode

        def fset(self, value):
            assert isinstance(value, int), "argument is not integer"
            self._retcode = value

        return locals()
    retcode = property(**retcode())

    def retname():
        doc = """ """

        def fget(self):
            return self._retname

        def fset(self, value):
            assert isinstance(value, str), "argument is not string"
            self._retname = value

        return locals()
    retname = property(**retname())


class UnknownError(PyKwalifyException):
    """
    """
    def __init__(self, *args, **kwargs):
        """
        """
        assert 'retcode' not in kwargs, "keyword retcode implicitly defined"
        super(self.__class__, self).__init__(
            retcode=retnames['unknownerror'],
            *args, **kwargs
        )


class SchemaError(PyKwalifyException):
    """
    """
    class SchemaErrorEntry(object):
        """
        """
        def __init__(self, msg, path, value, **kwargs):
            """
            """
            self.msg = msg
            self.path = path
            self.value = value
            for key, value in kwargs.items():
                self.__setattr__(key, value)

        def __repr__(self):
            return self.msg.format(**self.__dict__)

    def __init__(self, *args, **kwargs):
        """
        """
        assert "retcode" not in kwargs, "keyword retcode implicitly defined"
        super(self.__class__, self).__init__(
            retcode=retnames["schemaerror"],
            *args, **kwargs
        )


class CoreError(PyKwalifyException):
    """
    """
    def __init__(self, *args, **kwargs):
        """
        """
        assert "retcode" not in kwargs, "keyword retcode implicitly defined"
        super(self.__class__, self).__init__(
            retcode=retnames["coreerror"],
            *args, **kwargs
        )


class NotMappingError(PyKwalifyException):
    """
    """
    def __init__(self, *args, **kwargs):
        """
        """
        assert "retcode" not in kwargs, "keyword retcode implicitly defined"
        super(self.__class__, self).__init__(
            retcode=retnames['notmaperror'],
            *args, **kwargs
        )


class NotSequenceError(PyKwalifyException):
    """
    """
    def __init__(self, *args, **kwargs):
        """
        """
        assert "retcode" not in kwargs, "keyword retcode implicitly defined"
        super(self.__class__, self).__init__(
            retcode=retnames['notsequenceerror'],
            *args, **kwargs
        )


class RuleError(PyKwalifyException):
    """
    """
    def __init__(self, *args, **kwargs):
        """
        """
        assert "retcode" not in kwargs, "keyword retcode implicitly defined"
        super(self.__class__, self).__init__(
            retcode=retnames["ruleerror"],
            *args, **kwargs
        )


class SchemaConflict(PyKwalifyException):
    """
    """
    def __init__(self, *args, **kwargs):
        """
        """
        assert "retcode" not in kwargs, "keyword retcode implicitly defined"
        super(self.__class__, self).__init__(
            retcode=retnames["schemaconflict"],
            *args, **kwargs
        )
