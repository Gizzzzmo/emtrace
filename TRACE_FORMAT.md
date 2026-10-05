# Trace Format

This document described the format of the binary data in `.emtrace` sections of
executables.

The trace format consists of multiple records. Each record starts with the
ascii-encoded string `EMT`, followed by seven single-byte numbers. The first
four of these are offset pointers relative to the start of the record:

- The first byte points to the record type, a four-byte string.
- The second byte points to an `size_t` value indicating the total size of the
  record (starting from `EMT`) in bytes.
- The third byte points to the payload data of the record.
- The fourth byte points to an `size_t` value used to determine the endianness
  of the record (see below).

The remaining three single-byte numbers describe the record itself:

- The byte-width of `size_t` within this record. All `size_t` values belonging
  to this record — including the total size and the endianness probe — are
  stored with this width.
- The major version of the record type's layout.
- The minor version of the record type's layout.

The first fourteen bytes (`EMT`, the seven numbers, and the four-byte type
string) are called the record *framing*. These pointed-to pieces of data *must
not overlap* with either each other, or with the framing.

**Example (where size_t is four-byte little-endian)**

```txt
             byte-width of size_t and version numbers           endianness probe
                                       vv vvvv                     vvvv vvvv
Byte position (hex):  0    2    4    6    8    a    c    e    10   12   14   16   18   1a   1c   1e
hexadecimal:          454d 540a 0e16 1204 0100 424c 5542 1f00 0000 0001 0203 4d79 5061 796c 6f61 64
ASCII (where appl.):   E M  T                   B L  U B                      M y  P a  y l  o a  d
                             ^^ ^^^^ ^^                  ^^^^ ^^^^         
   pointers to type, size, payload, and endianness      total size of the record, 0x1f bytes
                                                            
```

This record has type `BLUB`, and contains the ASCII-encoded payload `MyPayload`.
Note that `BLUB` isn't (currently) a valid record type. The point is that this
outer framing is generic, and can therefore be extended with new record types in
the future.

There may be arbitrary internal padding bytes between pointed-to data *inside* a
record that don't affect the record-type or -payload, and the order in which
record-type, -size, -payload, and the endianness probe may be chosen
arbitrarily.

*Between* two records any bytes may appear, except the `EMT` sequence which
always signifies the beginning of a new record.

## Record Metadata

Every record is fully self-describing: a parser can locate, measure, and decode
any record without consulting any other record. The following metadata is
carried by *every* record in its framing.

### Endianness Probe

Each record contains an endianness probe: a `size_t` value, stored with the
record's `size_t` width, whose bytes are — from least to most significant —
`0x00`, `0x01`, `0x02`, and so on. For example, with a `size_t` width of four
the probe value is `0x03020100`. When stored in little-endian byte order the
probe bytes appear in increasing order (`00 01 02 03`); when stored in
big-endian byte order they appear in decreasing order (`03 02 01 00`). A parser
can therefore determine the endianness of all `size_t` values in a record from
the probe alone.

### Record Type Versions

Every record carries the major and minor version of its record type's layout.
Record types are versioned independently: the `MGIC` record layout has its own
version, the `TRCE` record layout has its own version, and future record types
carry their own versions.

- A change of the major version signals an incompatible change of the record's
  layout. Parsers must not attempt to decode records whose major version they do
  not know.
- A change of the minor version signals a backwards-compatible change, e.g. the
  addition of new values or optional fields. Parsers that encounter a minor
  version higher than the one they know may attempt to decode the record with
  the rules of the known minor version.

Currently both existing record types are at version 1.0.

## Record Types

Currently there exist two record types, but the uniform `EMT`-prefix scheme
described above allows for future extensions in a way where older parsers can
skip over unknown record types.

### Magic Record `MGIC`

The magic record contains the global metadata about the trace data that cannot
be derived from the individual records. In a normal file format this would be
specified to be at the start of the file. But since the only way to force most
compilers to do this is with a custom linker script, this record may appear
*anywhere*, but must contain a long (32 byte) *magic* byte-string that is
unlikely to appear anywhere else.

In other words, to parse trace data, first search for these magic bytes, then
parse the record it is contained in, and then use the parsed metadata to parse
the runtime stream.

There must be *exactly one* `MGIC` record in a section. Only the final linked
artifact (executable or shared object) contains a `MGIC` record. Intermediate
artifacts — object files and static archives — contain only self-describing
records, so that records from artifacts built with different emtrace versions
can be merged by the linker and still be decoded individually.

In hexadecimal the magic string is:

`d197f522 d9269fd1 ad703392 f659dfd0 fbecbd60 971325e8 9201b25a 385d9ec7`

It must lie at the beginning of the magic record's payload.

The metadata is structured as follows: Immediately after the magic string there
are three more single-byte integers:

- The first describes how many bytes make up an `emt_ptr_t`.
- The second contains the `EMT_ALIGNMENT_POWER`. All output `emt_ptr_t` will
  need to be multiplied by `2^EMT_ALIGNMENT_POWER` to get the actual pointer
  value.
- The third is an encoding id, currently one of:
  - `0`: trace stream consists of raw binary data.
  - `1`: trace stream consists of COBS-packets.
  - `2`: trace stream consists of COBS-packets interleaved with raw binary
    *passthrough* data.

### Alignment

`EMT_ALIGNMENT_POWER` is a global property of the section: every record must
start at an address that is a multiple of `2^EMT_ALIGNMENT_POWER` — both
relative to the start of the section, and in the loaded binary's virtual address
space. This guarantees that shifting runtime addresses right by the alignment
power, as described below, is lossless.

Since records are self-describing but the alignment power is only declared in
the `MGIC` record, inspection tools verify consistency:

- For a linked binary: the section's alignment (`sh_addralign`) must be at least
  `2^EMT_ALIGNMENT_POWER`, and every record must start at a
  `2^EMT_ALIGNMENT_POWER`-aligned offset within the section.
- For a static archive: the *highest possible alignment power* is the largest
  power of two that every record of the archive is guaranteed to be aligned to
  in any linking. The linker aligns each input section to its own
  `sh_addralign`, while the offsets of records within a member's section are
  fixed, so for each member it is the smaller of the member's section alignment
  (`sh_addralign`) and the common alignment of all record starts within that
  member's section. The overall value is the minimum of this quantity across all
  members that contain an `.emtrace` section. Standard linkers cannot realign
  individual records (records are opaque bytes inside one section), so this
  bound is tight.

If a static archive is built with a different `EMT_ALIGNMENT_POWER` than the
binary it is linked into, the archive's records may not be sufficiently aligned
in the final binary. The inspection tools detect this mismatch.

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

All `size_t` values in a `TRCE` record are stored with the record's `size_t`
width and endianness, as described above.

#### Runtime stream format

At runtime, each time a trace point fires it writes:

1. An `emt_ptr_t` containing the address of the corresponding `TRCE` record,
   right-shifted by `EMT_ALIGNMENT_POWER` bits. The parser recovers the section
   offset by computing `address × 2^EMT_ALIGNMENT_POWER + aslr_offset`, where
   `aslr_offset` is the difference between the section offset of the `MGIC`
   record and the runtime address of `MGIC` reported in the initialization step.
   Pointers in the stream are stored with the `emt_ptr_t` width and endianness
   of the `MGIC` record. For this to be lossless, every record's runtime address
   must be smaller than `2^(8·sizeof(emt_ptr_t) + EMT_ALIGNMENT_POWER)`; records
   at or above this bound lose bits when shifted and can no longer be
   distinguished from other records.
2. For each argument, the raw bytes of the argument value:
   - For `EMT_FLAG_STATIC` scalar arguments: exactly `size` bytes.
   - For `EMT_FLAG_STATIC` list arguments: the concatenation of the streams of
     `size` elements, where each element is encoded as described by the type's
     child (i.e. `size` is an element count, not a byte count).
   - For `EMT_FLAG_NULL_TERMINATED` arguments: bytes up to and including the
     terminating `\0`.
   - For `EMT_FLAG_LENGTH_PREFIXED` arguments: an element count, whose
     byte-width is given by the type's `size` field, followed by the
     concatenation of the streams of that many elements (for static, fixed-size
     elements this is `count × child_size` bytes).

Argument values in the stream are stored with the endianness of the `TRCE`
record they belong to.

Before the first trace record the program writes the runtime address of the
`MGIC` record (also as an `emt_ptr_t`) so the parser can determine the ASLR
offset.
