def _repr(node, args=None, nameblacklist=None):
    classname = node.__class__.__name__
    args = args or []
    nameblacklist = nameblacklist or []
    for key, value in filter(
        lambda item: not item[0].startswith("_") and item[0] not in nameblacklist,
        sorted(node.__dict__.items(), key=lambda item: item[0]),
    ):
        args.append("%s=%r" % (key, value))
    return "%s(%s)" % (classname, ", ".join(args))
