from collections.abc import Callable


def cobs_get_frame(
    istream: Callable[[int], bytes], passthrough: bool = False
) -> tuple[bytes, bytes, str | None]:
    """
    Decodes and returns the next COBS frame in the given input stream.
    The stream is queried until a full COBS frame was received or the stream is empty.
    Returns the frame, the bytes that were passed through, and an error message.
    An empty frame without an error signals the end of the stream.
    Optionally set *passthrough* to decode according to COBS "passthrough" rules.
    In that case frames have to start with a null-byte as well as ending in one,
    and any bytes before the first null-byte, are "passed through".
    Since a COBS frame cannot consist of just a single null-byte,
    a pair of null-bytes (which would otherwise mean starting and immediately ending a frame)
    is interpreted as an escaped null-byte in the passthrough stream.
    """

    buffer = bytearray()
    data = istream(1)

    passed_through: bytes = b""
    if passthrough:
        while len(data) != 0:
            if data[0] == 0:
                data = istream(1)
                if len(data) == 0:
                    return b"", passed_through, None
                # two successive null-bytes in input are interpreted as an escaped null-byte
                # in the passthrough stream
                if data[0] != 0:
                    break
            passed_through += data
            data = istream(1)

    if len(data) == 0:
        return b"", passed_through, None
    if data[0] == 0:
        return bytes(buffer), passed_through, "zero byte at start of frame"

    zero_in_block_error: str | None = None
    while True:
        block_prefix = data[0]

        block_size = 254 if block_prefix == 0xFF else block_prefix - 1

        data = istream(block_size)
        if 0 in data:
            zero_in_block_error = "zero in block"
        buffer.extend(data)

        if len(data) < block_size:
            if len(buffer) == 0:
                return (
                    b"",
                    passed_through,
                    zero_in_block_error or "frame incomplete",
                )
            return bytes(buffer), passed_through, zero_in_block_error
        assert len(data) == block_size

        data = istream(1)
        if len(data) == 0 or data[0] == 0:
            return bytes(buffer), passed_through, zero_in_block_error

        if block_prefix != 0xFF:
            buffer.append(0)

        block_prefix = -1
