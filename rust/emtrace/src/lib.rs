#![doc = include_str!("../../../README.md")]
use core::convert::Infallible;
use core::ops::DerefMut;
use core::result::Result;

#[cfg(feature = "std")]
use std::io::StdoutLock;
#[cfg(feature = "std")]
use std::io::Write;
#[cfg(feature = "std")]
use std::sync::MutexGuard;

cfg_if::cfg_if!(
    if #[cfg(feature="alignment_power_0")] {
        pub const ALIGNMENT_POWER: u8 = 0;
    } else if #[cfg(feature="alignment_power_1")] {
        pub const ALIGNMENT_POWER: u8 = 1;
    } else if #[cfg(feature="alignment_power_2")] {
        pub const ALIGNMENT_POWER: u8 = 2;
    } else if #[cfg(feature="alignment_power_3")] {
        pub const ALIGNMENT_POWER: u8 = 3;
    } else if #[cfg(feature="alignment_power_4")] {
        pub const ALIGNMENT_POWER: u8 = 4;
    } else if #[cfg(feature="alignment_power_5")] {
        pub const ALIGNMENT_POWER: u8 = 5;
    } else if #[cfg(feature="alignment_power_6")] {
        pub const ALIGNMENT_POWER: u8 = 6;
    } else if #[cfg(feature="alignment_power_7")] {
        pub const ALIGNMENT_POWER: u8 = 7;
    } else if #[cfg(feature="alignment_power_8")] {
        pub const ALIGNMENT_POWER: u8 = 8;
    } else if #[cfg(feature="alignment_power_9")] {
        pub const ALIGNMENT_POWER: u8 = 9;
    } else if #[cfg(feature="alignment_power_10")] {
        pub const ALIGNMENT_POWER: u8 = 10;
    } else {
        pub const ALIGNMENT_POWER: u8 = 0;
    }
);

// ── Flag constants (C-compatible small integers) ─────────────────────────────

/// Per-argument flag: associated bytes have a fixed size.
pub const FLAG_STATIC: usize = 0;
/// Per-argument flag: associated bytes are variable-length and null-terminated.
pub const FLAG_NULL_TERMINATED: usize = 1;
/// Per-argument flag: associated bytes are variable-length, prefixed by a
/// `SizeT` element count in the output stream.
pub const FLAG_LENGTH_PREFIXED: usize = 2;

// ── Formatter IDs ────────────────────────────────────────────────────────────

/// Use Python's `str.format()` for formatting.
pub const PY_FORMAT: usize = 0;
/// Do not use any formatter; print the string as-is.
pub const NO_FORMAT: usize = 1;
/// Use Python's C-style formatter (`%`).
pub const C_STYLE_FORMAT: usize = 2;

// ── Encoding IDs ─────────────────────────────────────────────────────────────

/// Raw (no encoding).
pub const RAW_ENCODING: usize = 0;
/// COBS encoding.
pub const COBS_ENCODING: usize = 1;
/// COBS with passthrough.
pub const COBS_PASSTHROUGH_ENCODING: usize = 2;

// ── FormatInfo ────────────────────────────────────────────────────────────────

/// A compile-time byte array holding a C-compatible TRCE record.
/// Stored in the `.emtrace` ELF section.
#[repr(C)]
pub struct TrceRecord<const N: usize> {
    pub bytes: [u8; N],
}

// ── MgicInfo ──────────────────────────────────────────────────────────────────

/// A compile-time byte array holding a C-compatible MGIC record.
/// Stored in the `.emtrace` ELF section via the `emtrace_init!` macro.
#[repr(C)]
pub struct MgicRecord<const N: usize> {
    pub bytes: [u8; N],
}

// ── Sink trait ────────────────────────────────────────────────────────────────

/// Trait for emtrace sinks.
pub trait Sink {
    /// The error that can happen when calling `out` and `finish`.
    /// Set this to `Infallible` if your implementation can't produce an error.
    type OutError;
    /// The error that can happen when calling `begin`.
    /// Set this to `Infallible` if your implementation can't produce an error.
    type BeginError;
    /// Write bytes of an emitted trace.
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError>;
    /// Begin emitting a trace.
    ///
    /// Called just before emitting a trace. Provides `info_addr` (address of
    /// the format info record in the `.emtrace` section) and `total_size`
    /// (fixed-size bytes to be written).
    fn begin(&mut self, info_addr: usize, total_size: usize) -> Result<(), Self::BeginError>;

    fn finish(self) -> Result<(), Self::OutError>
    where
        Self: core::marker::Sized,
    {
        Ok(())
    }
}

// ── COBS encoder ─────────────────────────────────────────────────────────────

pub struct CobsEncoder {
    buffer: [u8; 254],
    pos: u8,
}

impl Default for CobsEncoder {
    fn default() -> Self {
        Self::new()
    }
}

impl CobsEncoder {
    pub fn new() -> Self {
        Self {
            buffer: [0; 254],
            pos: 0,
        }
    }

    pub fn encode<T: Sink>(&mut self, sink: &mut T, b: &[u8]) -> Result<(), T::OutError> {
        for byte in b {
            if *byte == 0 {
                sink.out(core::slice::from_ref(&(self.pos + 1)))?;
                sink.out(&self.buffer[..self.pos as usize])?;
                self.pos = 0;
                continue;
            }
            if self.pos == 254 {
                sink.out(core::slice::from_ref(&255))?;
                sink.out(&self.buffer)?;
                self.pos = 0;
            }
            self.buffer[self.pos as usize] = *byte;
            self.pos += 1;
        }
        Ok(())
    }

    pub fn finalize<T: Sink>(&mut self, sink: &mut T) -> Result<(), T::OutError> {
        if self.pos > 0 {
            sink.out(core::slice::from_ref(&(self.pos + 1)))?;
            sink.out(&self.buffer[..self.pos as usize])?;
        } else {
            sink.out(core::slice::from_ref(&1u8))?;
        }
        sink.out(core::slice::from_ref(&0u8))
    }
}

pub struct CobsConsumer<T: Sink> {
    inner: T,
    encoder: CobsEncoder,
}

impl<T: Sink> CobsConsumer<T> {
    pub fn new(inner: T) -> Self {
        Self {
            inner,
            encoder: CobsEncoder::new(),
        }
    }
}

impl<T: Sink> Sink for CobsConsumer<T> {
    type OutError = T::OutError;
    type BeginError = T::BeginError;

    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.encoder.encode(&mut self.inner, b)
    }

    fn begin(&mut self, info_addr: usize, total_size: usize) -> Result<(), Self::BeginError> {
        self.inner.begin(info_addr, total_size)
    }

    fn finish(mut self) -> Result<(), Self::OutError> {
        self.encoder.finalize(&mut self.inner)?;
        self.inner.finish()
    }
}

pub struct CobsAdapter<'a, T: Sink> {
    inner: &'a mut T,
    encoder: CobsEncoder,
}

impl<'a, T: Sink> CobsAdapter<'a, T> {
    pub fn new(inner: &'a mut T) -> Self {
        Self {
            inner,
            encoder: CobsEncoder::new(),
        }
    }
}

impl<T: Sink> Sink for CobsAdapter<'_, T> {
    type OutError = T::OutError;
    type BeginError = T::BeginError;

    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.encoder.encode(self.inner, b)
    }

    fn begin(&mut self, info_addr: usize, total_size: usize) -> Result<(), Self::BeginError> {
        self.inner.begin(info_addr, total_size)
    }

    fn finish(mut self) -> Result<(), Self::OutError> {
        self.encoder.finalize(self.inner)
    }
}

#[cfg(feature = "std")]
impl<T: Sink> Sink for MutexGuard<'_, T> {
    type OutError = T::OutError;
    type BeginError = T::BeginError;
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.deref_mut().out(b)
    }
    fn begin(&mut self, info_addr: usize, total_size: usize) -> Result<(), Self::BeginError> {
        self.deref_mut().begin(info_addr, total_size)
    }
}

#[cfg(feature = "std")]
impl Sink for StdoutLock<'_> {
    type OutError = std::io::Error;
    type BeginError = Infallible;
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.write_all(b)?;
        Ok(())
    }
    fn begin(&mut self, info_addr: usize, total_size: usize) -> Result<(), Self::BeginError> {
        Ok(())
    }
}

impl Sink for Vec<u8> {
    type OutError = Infallible;
    type BeginError = Infallible;
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.extend_from_slice(b);
        Ok(())
    }
    fn begin(&mut self, info_addr: usize, _total_size: usize) -> Result<(), Self::BeginError> {
        self.out(&info_addr.to_ne_bytes())?;
        Ok(())
    }
}

struct LinkResolveSink;

use std::ffi::c_void;
// extern C function
unsafe extern "C" {
    #[unsafe(no_mangle)]
    fn emt_default_begin(info_addr: *const c_void, total_size: usize, extra_arg: *mut c_void);

    #[unsafe(no_mangle)]
    fn emt_default_out(data: *const c_void, size: usize, extra_arg: *mut c_void);

    #[unsafe(no_mangle)]
    fn emt_default_finish(info_addr: *const c_void, total_size: usize, extra_arg: *mut c_void);
}

impl Sink for LinkResolveSink {
    type OutError = Infallible;
    type BeginError = Infallible;
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        unsafe {
            emt_default_out(b.as_ptr() as *const c_void, b.len(), core::ptr::null_mut());
        };
        Ok(())
    }
    fn begin(&mut self, info_addr: usize, total_size: usize) -> Result<(), Self::BeginError> {
        unsafe {
            emt_default_begin(info_addr as *const c_void, 0, core::ptr::null_mut());
        }
        Ok(())
    }
}

impl Drop for LinkResolveSink {
    fn drop(&mut self) {
        unsafe {
            emt_default_finish(core::ptr::null(), 0, core::ptr::null_mut());
        }
    }
}

// ── Trace trait ───────────────────────────────────────────────────────────────

/// Maximum number of descendants in the flat descendants table.
pub const DESCENDANTS_LIMIT: usize = 0x1000;

/// Describes a child/descendant entry in the type tree.
/// `(&'static str name, &'static str type_id, usize size_bytes, usize flag, usize num_children)`
pub type DescendantEntry = (&'static str, &'static str, usize, usize, usize);

/// Trait for types that can be serialized into an emtrace stream.
pub trait Trace {
    /// Byte size for static types; 0 for dynamic types.
    const SIZE_BYTES: usize;
    /// Flag: `FLAG_STATIC`, `FLAG_NULL_TERMINATED`, or `FLAG_LENGTH_PREFIXED`.
    const FLAG: usize;
    /// Type identifier string (e.g. `"signed"`, `"string"`, `"list"`).
    const ID: &'static str;
    /// Number of direct children (0 for scalars, 1 for lists).
    const NUM_CHILDREN: usize;
    /// Flat table of descendants (children, grandchildren, …).
    /// Each entry: `(child_name, child_id, child_size_bytes, child_flag, child_num_children)`.
    const DESCENDANTS: &'static [DescendantEntry];
    /// Total number of entries in `DESCENDANTS`.
    const NUM_DESCENDANTS: usize;

    /// Static size contribution for the runtime `total_size` hint.
    const STATIC_SIZE: usize = match Self::FLAG {
        FLAG_STATIC => Self::SIZE_BYTES,
        _ => 0,
    };

    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError>;
    fn size(&self) -> usize;
}

impl<T> Trace for &T
where
    T: Trace,
    T: ?Sized,
{
    const SIZE_BYTES: usize = T::SIZE_BYTES;
    const FLAG: usize = T::FLAG;
    const ID: &'static str = T::ID;
    const NUM_CHILDREN: usize = T::NUM_CHILDREN;
    const DESCENDANTS: &'static [DescendantEntry] = T::DESCENDANTS;
    const NUM_DESCENDANTS: usize = T::NUM_DESCENDANTS;
    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        (*self).serialize(f)
    }
    fn size(&self) -> usize {
        (*self).size()
    }
}

impl<T: Trace> Trace for Box<T> {
    const SIZE_BYTES: usize = T::SIZE_BYTES;
    const FLAG: usize = T::FLAG;
    const ID: &'static str = T::ID;
    const NUM_CHILDREN: usize = T::NUM_CHILDREN;
    const DESCENDANTS: &'static [DescendantEntry] = T::DESCENDANTS;
    const NUM_DESCENDANTS: usize = T::NUM_DESCENDANTS;
    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        self.as_ref().serialize(f)
    }
    fn size(&self) -> usize {
        self.as_ref().size()
    }
}

impl Trace for str {
    const SIZE_BYTES: usize = 0;
    const FLAG: usize = FLAG_NULL_TERMINATED;
    const ID: &'static str = "string";
    const NUM_CHILDREN: usize = 0;
    const DESCENDANTS: &'static [DescendantEntry] = &[];
    const NUM_DESCENDANTS: usize = 0;
    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        f.out(self.as_bytes())
    }
    fn size(&self) -> usize {
        self.len()
    }
}

impl Trace for String {
    const SIZE_BYTES: usize = str::SIZE_BYTES;
    const FLAG: usize = str::FLAG;
    const ID: &'static str = str::ID;
    const NUM_CHILDREN: usize = str::NUM_CHILDREN;
    const DESCENDANTS: &'static [DescendantEntry] = str::DESCENDANTS;
    const NUM_DESCENDANTS: usize = str::NUM_DESCENDANTS;
    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        self.as_str().serialize(f)
    }
    fn size(&self) -> usize {
        self.as_str().size()
    }
}

impl<T: Trace> Trace for [T] {
    const SIZE_BYTES: usize = 0;
    const FLAG: usize = FLAG_LENGTH_PREFIXED;
    const ID: &'static str = "list";
    const NUM_CHILDREN: usize = 1;
    const DESCENDANTS: &'static [DescendantEntry] = &{
        let mut children: [DescendantEntry; DESCENDANTS_LIMIT] =
            [("", "", 0, 0, 0); DESCENDANTS_LIMIT];
        // First entry: the direct child
        children[0] = ("", T::ID, T::SIZE_BYTES, T::FLAG, T::NUM_CHILDREN);
        // Copy descendants of T shifted by 1
        let mut remaining = T::NUM_CHILDREN;
        let mut i = 0;
        while remaining > 0 {
            children[i + 1] = T::DESCENDANTS[i];
            remaining += T::DESCENDANTS[i].4;
            remaining -= 1;
            i += 1;
        }
        children
    };
    const NUM_DESCENDANTS: usize = 1 + T::NUM_DESCENDANTS;
    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        for el in self {
            if T::FLAG == FLAG_LENGTH_PREFIXED {
                el.size().serialize(f)?;
            }
            el.serialize(f)?;
            if T::FLAG == FLAG_NULL_TERMINATED {
                0u8.serialize(f)?;
            }
        }
        Ok(())
    }
    fn size(&self) -> usize {
        self.len()
    }
}

impl<T: Trace> Trace for Vec<T> {
    const SIZE_BYTES: usize = <[T] as Trace>::SIZE_BYTES;
    const FLAG: usize = <[T] as Trace>::FLAG;
    const ID: &'static str = <[T] as Trace>::ID;
    const NUM_CHILDREN: usize = <[T] as Trace>::NUM_CHILDREN;
    const DESCENDANTS: &'static [DescendantEntry] = <[T] as Trace>::DESCENDANTS;
    const NUM_DESCENDANTS: usize = <[T] as Trace>::NUM_DESCENDANTS;
    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        self[..].serialize(f)
    }
    fn size(&self) -> usize {
        self[..].size()
    }
}

impl<T: Trace, const N: usize> Trace for [T; N] {
    const SIZE_BYTES: usize = N;
    const FLAG: usize = FLAG_STATIC;
    const ID: &'static str = "list";
    const NUM_CHILDREN: usize = 1;
    const DESCENDANTS: &'static [DescendantEntry] = <[T] as Trace>::DESCENDANTS;
    const NUM_DESCENDANTS: usize = <[T] as Trace>::NUM_DESCENDANTS;

    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        for el in self {
            if T::FLAG == FLAG_LENGTH_PREFIXED {
                el.size().serialize(f)?;
            }
            el.serialize(f)?;
            if T::FLAG == FLAG_NULL_TERMINATED {
                0u8.serialize(f)?;
            }
        }
        Ok(())
    }

    fn size(&self) -> usize {
        N
    }
}

macro_rules! impl_trace_for_primitive {
    ($type:ty, $id:expr) => {
        impl Trace for $type {
            const SIZE_BYTES: usize = core::mem::size_of::<Self>();
            const FLAG: usize = FLAG_STATIC;
            const ID: &'static str = $id;
            const NUM_CHILDREN: usize = 0;
            const DESCENDANTS: &'static [DescendantEntry] = &[];
            const NUM_DESCENDANTS: usize = 0;

            fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
                f.out(&self.to_ne_bytes())
            }

            fn size(&self) -> usize {
                core::mem::size_of::<Self>()
            }
        }
    };
}

impl_trace_for_primitive!(i8, "signed char");
impl_trace_for_primitive!(i16, "signed");
impl_trace_for_primitive!(i32, "signed");
impl_trace_for_primitive!(i64, "signed");
impl_trace_for_primitive!(i128, "signed");
impl_trace_for_primitive!(isize, "signed");

impl_trace_for_primitive!(u8, "char");
impl_trace_for_primitive!(u16, "unsigned");
impl_trace_for_primitive!(u32, "unsigned");
impl_trace_for_primitive!(u64, "unsigned");
impl_trace_for_primitive!(u128, "unsigned");
impl_trace_for_primitive!(usize, "unsigned");

impl_trace_for_primitive!(f32, "float");
impl_trace_for_primitive!(f64, "double");

impl Trace for bool {
    const SIZE_BYTES: usize = core::mem::size_of::<u8>();
    const FLAG: usize = FLAG_STATIC;
    const ID: &'static str = "bool";
    const NUM_CHILDREN: usize = 0;
    const DESCENDANTS: &'static [DescendantEntry] = &[];
    const NUM_DESCENDANTS: usize = 0;

    fn serialize<O: Sink>(&self, f: &mut O) -> Result<(), O::OutError> {
        f.out(&[*self as u8])
    }

    fn size(&self) -> usize {
        core::mem::size_of::<u8>()
    }
}

// ── Error type ────────────────────────────────────────────────────────────────

#[derive(Debug)]
pub enum Error<BeginError, OutError> {
    Begin(BeginError),
    Out(OutError),
}

// ── MGIC record writer ────────────────────────────────────────────────────────

/// Write the pointer to a static MGIC record to `sink`.
///
/// The `emtrace_init!` macro generates a `static MgicInfo<N>` in the `.emtrace`
/// ELF section and calls this function with its address.  The parser reads
/// this pointer from the stream, resolves it against the section, and parses
/// the MGIC record there — exactly the same protocol as the C `EMTRACE_INIT()`.
pub fn write_mgic_ptr<S: Sink>(sink: &mut S, mgic_addr: usize) -> Result<(), S::OutError> {
    sink.out(&mgic_addr.to_ne_bytes())
}

// ── Private helpers for proc macro ───────────────────────────────────────────

/// Public but hidden module used by the `emtrace-macros` proc macro.
#[doc(hidden)]
pub mod __private {
    use super::{ALIGNMENT_POWER, DescendantEntry, Error, MgicRecord, Sink, Trace};
    use core::mem::size_of;

    /// Helper called by the `trace!` / `traceln!` proc macro to emit a trace.
    ///
    /// Having the `&mut S` parameter here anchors Rust's type inference on the
    /// concrete sink type, so the `Error<S::BeginError, S::OutError>` return
    /// type is always fully determined — even when `f` has an empty body.
    #[inline]
    pub fn emit<S, F>(
        sink: &mut S,
        addr: usize,
        total: usize,
        f: F,
    ) -> Result<(), Error<S::BeginError, S::OutError>>
    where
        S: Sink,
        F: FnOnce(&mut S) -> Result<(), S::OutError>,
    {
        sink.begin(addr, total).map_err(Error::Begin)?;
        Trace::serialize(&addr, sink).map_err(Error::Out)?;
        f(sink).map_err(Error::Out)
    }

    macro_rules! write_slice_to_byte_array {
        ($target:ident[$index:ident] = $val:expr) => {{
            let bytes = $val;
            let mut i = 0;
            while i < bytes.len() {
                $target[$index + i] = bytes[i];
                i += 1;
                $index += 1;
            }
        }};
    }

    macro_rules! write_value_to_byte_array {
        ($target:ident[$index:ident] = $val:expr) => {{
            let bytes = $val.to_ne_bytes();
            write_slice_to_byte_array!($target[$index] = bytes);
        }};
    }

    macro_rules! write_str_to_byte_array {
        ($target:ident[$index:ident] = $s:expr) => {{
            let bytes = $s.as_bytes();
            write_slice_to_byte_array!($target[$index] = bytes);
            $target[$index] = 0; // null terminator
            $index += 1;
        }};
    }

    const RECORD_HEADER_SIZE: usize = 10 + size_of::<usize>();

    pub const fn build_record_frame<const N: usize>() -> ([u8; N], usize, usize) {
        let mut record_frame = [0u8; N];
        record_frame[0] = b'E';
        record_frame[1] = b'M';
        record_frame[2] = b'T';
        record_frame[3] = 6;
        record_frame[4] = 10u8; // offset to record_size
        record_frame[5] = RECORD_HEADER_SIZE as u8; // offset to layout

        let mut i = 10;

        write_value_to_byte_array!(record_frame[i] = 0usize); // placeholder for record_size

        assert!(
            i == RECORD_HEADER_SIZE,
            "Header size mismatch in build_record_frame",
        );

        (record_frame, 6, RECORD_HEADER_SIZE)
    }

    // ── MGIC const helpers ────────────────────────────────────────────────────

    // Layout constants for the MGIC record.
    const MGIC_BYTE_STRING: [u8; 32] = [
        0xd1, 0x97, 0xf5, 0x22, 0xd9, 0x26, 0x9f, 0xd1, 0xad, 0x70, 0x33, 0x92, 0xf6, 0x59, 0xdf,
        0xd0, 0xfb, 0xec, 0xbd, 0x60, 0x97, 0x13, 0x25, 0xe8, 0x92, 0x01, 0xb2, 0x5a, 0x38, 0x5d,
        0x9e, 0xc7,
    ];

    const MGIC_RECORD_VERSION: u16 = 0;

    /// Byte size of the MGIC record (constant, independent of encoding).
    pub const fn mgic_record_size() -> usize {
        RECORD_HEADER_SIZE + 38 + 2 * size_of::<usize>()
    }

    /// Build a MGIC record as a byte array.
    ///
    /// `N` must equal `mgic_record_size()`.
    pub const fn build_mgic_record<const N: usize, PointerT>(encoding: usize) -> MgicRecord<N> {
        let (mut mgic_record, record_type_offset, payload_offset) = build_record_frame::<N>();

        mgic_record[record_type_offset] = b'M';
        mgic_record[record_type_offset + 1] = b'G';
        mgic_record[record_type_offset + 2] = b'I';
        mgic_record[record_type_offset + 3] = b'C';

        // meta[39]
        // magic hash (32 bytes)
        let mut i = payload_offset;
        write_slice_to_byte_array!(mgic_record[i] = MGIC_BYTE_STRING);
        assert!(i - payload_offset == 32);
        write_slice_to_byte_array!(mgic_record[i] = MGIC_RECORD_VERSION.to_le_bytes()); // version
        assert!(i - payload_offset == 34);
        const MGIC_RECORD_META_OFFSET: usize = 38;
        mgic_record[i] = MGIC_RECORD_META_OFFSET as u8;
        i += 1;
        mgic_record[i] = size_of::<usize>() as u8;
        i += 1;
        mgic_record[i] = size_of::<PointerT>() as u8;
        i += 1;
        mgic_record[i] = ALIGNMENT_POWER;
        i += 1;
        assert!(i - payload_offset == MGIC_RECORD_META_OFFSET);

        // size_t_meta[0] = byteorder_id
        write_value_to_byte_array!(mgic_record[i] = 0x0706050403020100usize);
        write_value_to_byte_array!(mgic_record[i] = encoding);

        assert!(mgic_record_size() == i);

        MgicRecord { bytes: mgic_record }
    }

    /// Describes one argument's type to the proc macro at compile time.
    pub struct ArgInfo {
        pub id: &'static str,
        pub size: usize,
        pub flag: usize,
        pub num_children: usize,
        pub descendants: &'static [DescendantEntry],
        pub num_descendants: usize,
    }

    /// Compute the byte size of a TRCE record with the given arguments.
    ///
    /// Layout (matches C `struct emt_info_unlikely_to_shadow_t`):
    ///   framing[10]
    ///   record_size (size_t / usize)
    ///   layout[NUM_PREFIX + layout_entries_for_args] * sizeof(size_t / usize)
    ///   fmt_str (+ null)
    ///   per-arg: type_id (+ null)
    ///   per-arg-with-child: child_name (+ null) + child_type_id (+ null) + …
    ///   file_str (+ null)
    ///
    /// NUM_PREFIX = 2 (num_args, fmt_offset)
    /// Per-arg layout entries (each is a native `size_t`):
    ///   leaf (no children):   4 entries  (type_id_offset, size, flag, num_children=0)
    ///   with N children:      4 + 4*N entries
    ///     (above + for each direct child: name_offset, child_size, child_flag, child_type_id_offset)
    ///
    /// Note: only direct children are encoded; grandchildren are not (format supports 1 level).
    /// Note: layout entries use native `usize` (= C `size_t`), NOT `SizeT` (= C `emt_size_t`).
    pub const fn trce_record_size(args: &[ArgInfo], fmt: &str, file: &str) -> usize {
        // Layout array entries use native usize (= C size_t), not SizeT (= C emt_size_t).
        let size_t_size = size_of::<usize>();

        // Count layout usize entries
        let mut layout_entries = 2usize; // num_args + fmt_offset
        let mut i = 0;
        while i < args.len() {
            let arg = &args[i];
            // 4 entries per arg (type_id_offset, size, flag, num_children)
            layout_entries += 4;
            // 4 entries per direct child only (name_offset, child_size, child_flag, child_type_id_offset)
            layout_entries += 4 * arg.num_children;
            i += 1;
        }
        // formatter + file_offset + line
        layout_entries += 3;

        // Framing (10) + record_size (usize) + layout array (all usize)
        let header_size = 10 + size_t_size + layout_entries * size_t_size;

        // Data section: fmt string + null
        let mut data_size = fmt.len() + 1;

        // Per-arg type id strings
        let mut i = 0;
        while i < args.len() {
            let arg = &args[i];
            data_size += arg.id.len() + 1;
            // Direct child name + type id strings (1 level only)
            let mut j = 0;
            while j < arg.num_children {
                data_size += arg.descendants[j].0.len() + 1; // child name
                data_size += arg.descendants[j].1.len() + 1; // child type id
                j += 1;
            }
            i += 1;
        }

        // File string + null
        data_size += file.len() + 1;

        header_size + data_size
    }

    /// Build the full TRCE record as a byte array.
    ///
    /// `N` must equal `trce_record_size(args, fmt, file)`.
    pub const fn build_trce_record<const N: usize>(
        args: &[ArgInfo],
        fmt: &str,
        formatter: usize,
        file: &str,
        line: usize,
    ) -> super::TrceRecord<N> {
        // Layout array entries and record_size field both use native usize (= C size_t).
        let size_t_size = size_of::<usize>();

        let (mut trce_record, record_type_offset, mut offset_idx) = build_record_frame::<N>();

        trce_record[record_type_offset] = b'T';
        trce_record[record_type_offset + 1] = b'R';
        trce_record[record_type_offset + 2] = b'C';
        trce_record[record_type_offset + 3] = b'E';

        // Pre-compute number of layout entries to find where data starts
        // (only direct children, not grandchildren — format supports 1 level)
        let mut layout_entries = 2usize;
        {
            let mut ai = 0;
            while ai < args.len() {
                layout_entries += 4 + 4 * args[ai].num_children;
                ai += 1;
            }
            layout_entries += 3; // formatter, file_offset, line
        }
        let mut data_idx = 10 + size_t_size + layout_entries * size_t_size;

        // num_args
        write_value_to_byte_array!(trce_record[offset_idx] = args.len());

        // fmt_offset (will be written to data at data_idx after layout)
        write_value_to_byte_array!(trce_record[offset_idx] = data_idx);

        // Write fmt string to data
        write_str_to_byte_array!(trce_record[data_idx] = fmt);

        // Per-arg entries
        let mut ai = 0;
        while ai < args.len() {
            let arg = &args[ai];

            // type_id_offset (will be at data_idx)
            write_value_to_byte_array!(trce_record[offset_idx] = data_idx);

            // size
            write_value_to_byte_array!(trce_record[offset_idx] = arg.size);

            // flag
            write_value_to_byte_array!(trce_record[offset_idx] = arg.flag);

            // num_children
            write_value_to_byte_array!(trce_record[offset_idx] = arg.num_children);

            // Write type id string
            write_str_to_byte_array!(trce_record[data_idx] = arg.id);

            // Direct child descriptors (1 level only; grandchildren not written to layout)
            let mut j = 0;
            while j < arg.num_children {
                let d = &arg.descendants[j];

                // child name_offset
                write_value_to_byte_array!(trce_record[offset_idx] = data_idx);

                // child size
                write_value_to_byte_array!(trce_record[offset_idx] = d.2);

                // child flag
                write_value_to_byte_array!(trce_record[offset_idx] = d.3);

                // child type_id_offset: written after name so we know the offset.
                // We already wrote name_offset, size, flag above; offset_idx now
                // points to the child_type_id_offset slot.
                write_str_to_byte_array!(trce_record[data_idx] = d.0); // child name → advances data_idx

                let type_id_bytes = data_idx.to_ne_bytes();
                let mut i = 0;
                while i < size_t_size {
                    trce_record[offset_idx] = type_id_bytes[i];
                    offset_idx += 1;
                    i += 1;
                }

                // Write child type id
                write_str_to_byte_array!(trce_record[offset_idx] = d.1);

                j += 1;
            }

            ai += 1;
        }

        // formatter
        write_value_to_byte_array!(trce_record[offset_idx] = formatter);

        // file_offset
        write_value_to_byte_array!(trce_record[offset_idx] = data_idx);

        // Write file string
        write_str_to_byte_array!(trce_record[data_idx] = file);
        let _unused = data_idx;

        // line
        write_value_to_byte_array!(trce_record[offset_idx] = line);

        super::TrceRecord { bytes: trce_record }
    }
}

// ── Re-exports from proc macro crate ─────────────────────────────────────────

pub use emtrace_macros::{emtrace_init, expect, trace, traceln};
