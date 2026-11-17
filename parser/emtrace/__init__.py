"""emtrace: parse and format emtrace trace data (pure-python library).

The command-line interface lives in :mod:`emtrace.cli` and is intentionally
*not* imported here, so ``import emtrace`` stays lightweight and free of
CLI side effects.

Subpackages:

- :mod:`emtrace.section`: trace information in emtrace sections (records)
- :mod:`emtrace.stream`: decoding the runtime trace stream
- :mod:`emtrace.exe`: reading and editing executables
"""

from ._version import __version__
from .errors import Issue, Report
from .exe.edit import nonload_binary, strip_binary
from .exe.exemod import (
    ExeError,
    ExeImage,
    NonloadReport,
    NotSupportedError,
    SectionNotFoundError,
    StripReport,
)
from .section.export import ExportReport, export_section, load_export
from .section.inspector import (
    ArchiveDetails,
    BinaryDetails,
    CheckReport,
    InspectReport,
    RawDetails,
    check_section,
    inspect_path,
)
from .section.loading import FormatInfoResult, TraceInfoFormat, load_format_info
from .section.records import MgicRecord, ParsedRecord, TraceRecord, iter_records
from .section.type_tree import Size, TypeNode
from .stream.decoding import (
    DecodeContext,
    Decoder,
    TypeRegistry,
    default_registry,
)
from .stream.engine import RunReport, emtrace
from .stream.framing import ByteNode
from .stream.values import MyDict, MyList, Placeholder

__all__ = [
    "__version__",
    "ArchiveDetails",
    "BinaryDetails",
    "ByteNode",
    "CheckReport",
    "DecodeContext",
    "Decoder",
    "ExeError",
    "ExeImage",
    "ExportReport",
    "FormatInfoResult",
    "InspectReport",
    "Issue",
    "MgicRecord",
    "MyDict",
    "MyList",
    "NonloadReport",
    "NotSupportedError",
    "ParsedRecord",
    "Placeholder",
    "RawDetails",
    "Report",
    "RunReport",
    "SectionNotFoundError",
    "Size",
    "StripReport",
    "TraceInfoFormat",
    "TraceRecord",
    "TypeNode",
    "TypeRegistry",
    "check_section",
    "default_registry",
    "emtrace",
    "export_section",
    "inspect_path",
    "iter_records",
    "load_export",
    "load_format_info",
    "nonload_binary",
    "strip_binary",
]
