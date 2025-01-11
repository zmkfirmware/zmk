import warnings

from anytree.exporter.dotexporter import DotExporter


class RenderTreeGraph(DotExporter):
    def __init__(self, *args, **kwargs):
        """Legacy. Use :any:`anytree.exporter.DotExporter` instead."""
        warnings.warn(
            ("anytree.RenderTreeGraph has moved. Use anytree.exporter.DotExporter instead"), DeprecationWarning
        )
        super(RenderTreeGraph, self).__init__(*args, **kwargs)
