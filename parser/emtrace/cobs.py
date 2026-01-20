from typing import Callable, Any
import sys


def cobs_get_frame(
    istream: Callable[[int], bytes], passthrough: Callable[[bytes], Any] | None = None
) -> bytes | None:
    """
    Decodes and returns the next COBS frame in the given input stream.
    The stream is queried until a full COBS frame was received or the stream is empty.
    Optionally provide a passthrough function to decode according to COBS "passthrough" rules.
    In that case frames have to start with a null-byte as well as ending in one,
    and any bytes before the first null-byte, are "passed through" to the provided callback.
    Since a COBS frame cannot consist of just a single null-byte,
    a pair of null-bytes (which would otherwise mean starting and immediately ending a frame)
    is interpreted as an escaped null-byte in the passthrough stream.
    """

    buffer = bytearray()
    data = istream(1)

    if passthrough is not None:
        while len(data) != 0:
            if data[0] == 0:
                data = istream(1)
                if len(data) == 0:
                    return None
                # two successive null-bytes in input are interpreted as an escaped null-byte
                # in the passthrough stream
                if data[0] != 0:
                    break
            passthrough(data)
            data = istream(1)

    if len(data) == 0:
        return None
    if data[0] == 0:
        print("COBS decoding error: zero byte at start of frame", file=sys.stderr)
        return bytes(buffer)

    while True:
        block_prefix = data[0]

        if block_prefix == 0xFF:
            block_size = 254
        else:
            block_size = block_prefix - 1

        data = istream(block_size)
        if 0 in data:
            print(
                f"COBS decoding error: zero byte in data block: {data}", file=sys.stderr
            )
        buffer.extend(data)
        if len(data) < block_size:
            if len(buffer) == 0:
                return None
            return bytes(buffer)
        assert len(data) == block_size

        data = istream(1)
        if len(data) == 0 or data[0] == 0:
            return bytes(buffer)
        if block_prefix != 0xFF:
            buffer.append(0)
        block_prefix = -1
