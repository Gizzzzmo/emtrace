# Trace Format

This document described the format of the binary data in `.emtrace` sections of executables.

The trace format consists of multiple records. Each record starts with the ascii-encoded string
`EMT`, follow by three single-byte numbers. Each of these is an offset pointer relative to the start
of the record:

- The first byte points to the record type, a four-byte string.
- The second byte points to an `size_t` value indicating the total size of the record (starting from
  `EMT`) in bytes.
- The third byte points to the payload data of the record.

## Record Types

Currently there exist two record types, but the uniform `EMT`-prefix scheme described above allows
for future extensions in a way where older parsers can skip over unknown record types.

### Magic Record `MGIC`

The magic record contains metadata about all the other records. In a normal file format this would
be specified to be at the start of the file. But since the only way to force most compilers to do
this is with a custom linker script, this record may appear *anywhere*, but must contain a long (32
byte) *magic* byte-string that is unlikely to appear anywhere else.

In other words, to parse trace data, first search for these magic bytes, then parse the record it is
contained in, and then use the parsed metadata to parse the rest of the records.

In hexadecimal the magic string is:

`d197f522 d9269fd1 ad703392 f659dfd0 fbecbd60 971325e8 9201b25a 385d9ec7`

It must lie at the beginning of the magic record's payload.

The metadata is structured as follows: Immediately after the magic string there are seven more
single-byte integers:

- The first two contain a single version number in little-endian format.
- The third is an offset pointer relative to the start of the record, pointing to several more
  `size_t` values (`size_t_meta`).
- The fourth describes how many bytes make up a `size_t`.
- The fifth describes hoy many bytes make up an `emt_ptr_t`.
- The sixth contains the `EMT_ALIGNMENT_POWER`. All output `emt_ptr_t` will need to be multiplied by
  `2^EMT_ALIGNMENT_POWER` to get the actual pointer value.

`size_t_meta` contains two more `size_t` values:

- `0x0706050403020100` (or whichever part of that fits in a `size_t`) in native endianness. From
  this the endianness of the file can be determined.
- an encoding id, currently one of:
  - `0`: trace stream consists of raw binary data.
  - `1`: trace stream consists of COBS-packets.
  - `2`: trace stream consists of COBS-packets interleaved with raw binary *passthrough* data.

### Trace Record `TRCE`

Each `TRCE` record describes a single trace point in the program. Its payload is a layout array of
`size_t` values followed by string data.

The layout array starts with two header entries:

- `layout[0]`: number of arguments (`num_args`)
- `layout[1]`: offset from the record start to the format string (a null-terminated UTF-8 string)

After the two header entries, `num_args` argument descriptors follow. Each descriptor occupies
either 4 or 8 `size_t` values depending on whether it has children.

**Leaf argument descriptor (4 `size_t` values):**

- `[0]`: offset from the record start to the argument's null-terminated type-id string
- `[1]`: size in bytes (`0` for dynamically-sized arguments)
- `[2]`: flag — one of:
  - `0` (`EMT_FLAG_STATIC`): argument has a fixed byte size given by field `[1]`
  - `1` (`EMT_FLAG_NULL_TERMINATED`): argument is a null-terminated byte string
  - `2` (`EMT_FLAG_LENGTH_PREFIXED`): argument is prefixed in the output stream by an `emt_size_t`
    element count
- `[3]`: number of child descriptors (`0` for leaf types such as `int`, `double`, etc.)

**Compound argument descriptor (4 + 4×`num_children` `size_t` values):**

For compound types (arrays, slices, structs) `num_children` is non-zero. Each child occupies 4 more
`size_t` entries immediately following the parent's 4-entry block:

- `[0]`: offset from the record start to the child's null-terminated name string
- `[1]`: child element size in bytes
- `[2]`: child flag (same values as the parent flag field)
- `[3]`: offset from the record start to the child's null-terminated type-id string

After all argument descriptors, three more `size_t` values close the layout array:

- `formatter_id`: which formatter to use (e.g. `0` = Python `str.format`, `1` = no format, `2` =
  C-style `printf`)
- `file_offset`: offset from the record start to the null-terminated source file path string
- `line`: source line number

All string data (format string, type-id strings, child name strings, file path) is stored as
null-terminated UTF-8 immediately following the layout array, in no guaranteed order. Offsets stored
in the layout array are all relative to the start of the `TRCE` record (i.e. to the `E` of `EMT`).

#### Runtime stream format

At runtime, each time a trace point fires it writes:

1. An `emt_ptr_t` containing the address of the corresponding `TRCE` record, right-shifted by
   `EMT_ALIGNMENT_POWER` bits. The parser recovers the section offset by computing
   `address × 2^EMT_ALIGNMENT_POWER + aslr_offset`, where `aslr_offset` is the difference between
   the section offset of the `MGIC` record and the runtime address of `MGIC` reported in the
   initialization step.
2. For each argument, the raw bytes of the argument value:
   - For `EMT_FLAG_STATIC` arguments: exactly `size` bytes.
   - For `EMT_FLAG_NULL_TERMINATED` arguments: bytes up to and including the terminating `\0`.
   - For `EMT_FLAG_LENGTH_PREFIXED` arguments: an `emt_size_t` element count followed by
     `count × child_size` bytes of element data.

Before the first trace record the program writes the runtime address of the `MGIC` record (also as
an `emt_ptr_t`) so the parser can determine the ASLR offset.
