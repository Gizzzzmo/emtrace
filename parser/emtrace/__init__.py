"""emtrace: parse and format emtrace trace data (pure-python library).

The command-line interface lives in :mod:`emtrace.cli` and is intentionally
*not* imported here, so ``import emtrace`` stays lightweight and free of
CLI side effects.
"""

__version__ = "0.2.0"

from .edit import nonload_binary, strip_binary
from .engine import (
    FormatInfoResult,
    RunReport,
    TraceInfoFormat,
    emtrace,
    load_format_info,
)
from .errors import Issue, Report
from .exemod import (
    ExeError,
    ExeImage,
    NonloadReport,
    NotSupportedError,
    SectionNotFoundError,
    StripReport,
)
from .inspector import (
    ArchiveDetails,
    BinaryDetails,
    CheckReport,
    ExportReport,
    InspectReport,
    RawDetails,
    check_section,
    export_section,
    inspect_path,
)

__all__ = [
    "ArchiveDetails",
    "BinaryDetails",
    "CheckReport",
    "ExeError",
    "ExeImage",
    "ExportReport",
    "FormatInfoResult",
    "InspectReport",
    "Issue",
    "NonloadReport",
    "NotSupportedError",
    "RawDetails",
    "Report",
    "RunReport",
    "SectionNotFoundError",
    "StripReport",
    "TraceInfoFormat",
    "check_section",
    "emtrace",
    "export_section",
    "inspect_path",
    "load_format_info",
    "nonload_binary",
    "strip_binary",
]
