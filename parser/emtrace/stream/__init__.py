"""Everything about the runtime trace stream.

- :mod:`.cobs`: COBS frame decoding
- :mod:`.framing`: stage 1, splitting the bytes of an event into byte trees
- :mod:`.decoding`: stage 2, decoding byte trees into Python values
- :mod:`.values`: the value types produced by the builtin decoders
- :mod:`.formatters`: combining format strings with decoded values
- :mod:`.engine`: the ``run`` command
"""
