# Trace Format

This document described the format of the binary data in `.emtrace` sections of
executables.

The trace format consists of multiple records. Each record starts with the
ascii-encoded string `EMT`, followed by three single-byte numbers. Each of these
is an offset pointer relative to the start of the record:

- The first byte points to the record type, a four-byte string.
- The second byte points to an `size_t` value indicating the total size of the
  record (starting from `EMT`) in bytes.
- The third byte points to the payload data of the record.
- These pointed-to pieces of data *must not overlap* with either each other, or
  with the first six bytes (`EMT` plus pointers)

**Example (where size_t is four-byte little-endian)**

```txt
Byte position (hex):  0    2    4    6    8    a    c    e    10   12   14   16
hexadecimal:          454d 5406 0a0e 424c 5542 1700 0000 4d79 5061 796c 6f61 6f
ASCII (where appl.):   E M  T         B L  U B            M y  P a  y l  o a  d
                             ^^ ^^^^           ^^^^ ^^^^
   pointers to type, length, and payload    total size of the record, 0x17 bytes
                                        
```

This record has type `BLUB`, and contains the ASCII-encoded payload `MyPayload`.
Note that `BLUB` isn't (currently) a valid record type. The point is that this
outer framing is generic, and can therefore be extended with new record types in
the future.

There may be arbitrary internal padding bytes between pointed-to data *inside* a
record that don't affect the record-type or -payload, and the order in which
record-type, -size, and -payload may be chosen arbitrarily.

*Between* two records any bytes may appear, except the `EMT` sequence which
always signifies the beginning of a new record.

## Record Types

Currently there exist two record types, but the uniform `EMT`-prefix scheme
described above allows for future extensions in a way where older parsers can
skip over unknown record types.

### Magic Record `MGIC`

The magic record contains metadata about all the other records. In a normal file
format this would be specified to be at the start of the file. But since the
only way to force most compilers to do this is with a custom linker script, this
record may appear *anywhere*, but must contain a long (32 byte) *magic*
byte-string that is unlikely to appear anywhere else.

In other words, to parse trace data, first search for these magic bytes, then
parse the record it is contained in, and then use the parsed metadata to parse
the rest of the records.

In hexadecimal the magic string is:

`d197f522 d9269fd1 ad703392 f659dfd0 fbecbd60 971325e8 9201b25a 385d9ec7`

It must lie at the beginning of the magic record's payload.

The metadata is structured as follows: Immediately after the magic string there
are six more single-byte integers:

- The first two contain a single version number in little-endian format.
- The third is an offset pointer relative to the start of the record, pointing
  to several more `size_t` values (`size_t_meta`).
- The fourth describes how many bytes make up a `size_t`.
- The fifth describes hoy many bytes make up an `emt_ptr_t`.
- The sixth contains the `EMT_ALIGNMENT_POWER`. All output `emt_ptr_t` will need
  to be multiplied by `2^EMT_ALIGNMENT_POWER` to get the actual pointer value.

`size_t_meta` contains two more `size_t` values:

- `0x0706050403020100` (or whichever part of that fits in a `size_t`) in native
  endianness. From this the endianness of the file can be determined.
- an encoding id, currently one of:
  - `0`: trace stream consists of raw binary data.
  - `1`: trace stream consists of COBS-packets.
  - `2`: trace stream consists of COBS-packets interleaved with raw binary
    *passthrough* data.

### Trace Record `TRCE`

Each `TRCE` record describes a single trace point in the program. It contains a
format string, a formatter id, and a tree-like description of the types of all
values that are later emitted at runtime when `EMTRACE()` is invoked, thus
enabling decoding the runtime stream.

The type information takes the form of a tree because types can be nested. For
example to decode a sequence (`list`-type) of values we also need to know what
the type of the elements *in the sequence* is. This nesting can be arbitrarily
deep, allowing e.g. lists of lists of numbers .

Each type is thus described by:

- its type identifier, an ASCII-string
- its size, including a flag whether this size depends on runtime values
- a description of all its children

The `TRCE` record's payload serializes all this information as follows (all
"pointers" are really offsets relative to the start of the record, *not the
start of the payload*!!):

- it begins with two `size_t` values:
  - the first describes the number of types (without their children) whose type
    description follows
  - the second is a pointer to the format string
- more `size_t` values follow that describe all types and their children; any
  single type consists of four or five `size_t` values:
  - *only for types that have a parent*: a pointer to the name of the child
  - a pointer to its type id
  - its size
  - its size flag (determines how size to interpret runtime-sizes)
  - the number of child-types
  - the full type-information is serialized, by walking the trees of top-level
    types in pre-order, one after the other
- three more `size_t` values:
  - the formatter identifier
  - a pointer to a source-location string (usually a file path)
  - a source-location line number
- finally all the pointed-to data (format string, type-ids, child-names, source
  locations) follows

#### Runtime stream format

At runtime, each time a trace point fires it writes:

1. An `emt_ptr_t` containing the address of the corresponding `TRCE` record,
   right-shifted by `EMT_ALIGNMENT_POWER` bits. The parser recovers the section
   offset by computing `address × 2^EMT_ALIGNMENT_POWER + aslr_offset`, where
   `aslr_offset` is the difference between the section offset of the `MGIC`
   record and the runtime address of `MGIC` reported in the initialization step.
2. For each argument, the raw bytes of the argument value:
   - For `EMT_FLAG_STATIC` scalar arguments: exactly `size` bytes.
   - For `EMT_FLAG_STATIC` list arguments: the concatenation of the streams of
     `size` elements, where each element is encoded as described by the type's
     child (i.e. `size` is an element count, not a byte count).
   - For `EMT_FLAG_NULL_TERMINATED` arguments: bytes up to and including the
     terminating `\0`.
   - For `EMT_FLAG_LENGTH_PREFIXED` arguments: an element count, whose
     byte-width is given by the type's `size` field, followed by the
     concatenation of the streams of that many elements (for static,
     fixed-size elements this is `count × child_size` bytes).

Before the first trace record the program writes the runtime address of the
`MGIC` record (also as an `emt_ptr_t`) so the parser can determine the ASLR
offset.
