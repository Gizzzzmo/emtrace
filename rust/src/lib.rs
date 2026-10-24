//! The binary record library for the emtrace crate
#![doc = include_str!("../README.md")]
use core::convert::Infallible;
use core::ffi::c_void;
use core::mem::size_of;
use core::ops::DerefMut;
use core::result::Result;
use core::slice;
use std::io::StdoutLock;
use std::io::Write;
use std::mem::MaybeUninit;
use std::sync::MutexGuard;

// The features allow changing the default alignment of all trace record format information
const DEFAULT_ALIGNMENT_POWER: usize = {
    if cfg!(feature = "default_alignment_1024") {
        10
    } else if cfg!(feature = "default_alignment_512") {
        9
    } else if cfg!(feature = "default_alignment_256") {
        8
    } else if cfg!(feature = "default_alignment_128") {
        7
    } else if cfg!(feature = "default_alignment_64") {
        6
    } else if cfg!(feature = "default_alignment_32") {
        5
    } else if cfg!(feature = "default_alignment_16") {
        4
    } else if cfg!(feature = "default_alignment_8") {
        3
    } else if cfg!(feature = "default_alignment_4") {
        2
    } else if cfg!(feature = "default_alignment_2") {
        1
    } else if cfg!(feature = "default_alignment_1") {
        0
    } else {
        8
    }
};

// Only one default_alignment_x feature may be active.
const DEFAULT_ALIGNMENT_COUNT: usize = {
    let mut count = 0usize;
    if cfg!(feature = "default_alignment_1") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_2") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_4") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_8") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_16") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_32") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_64") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_128") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_256") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_512") {
        count += 1;
    }
    if cfg!(feature = "default_alignment_1024") {
        count += 1;
    }
    count
};

const _: () = assert!(
    DEFAULT_ALIGNMENT_COUNT < 2,
    "enable at most one `default_alignment_*` feature"
);

// When any feature minimum_alignment_x is active, then all trace records that don't explicitly
// overwrite their alignment are aligned to *at least* this many bytes.
// Multiple such feature being active is fine, since higher alignments always satisfy lower ones.
const MIN_ALIGNMENT_POWER: usize = {
    let mut power = 0usize;
    if cfg!(feature = "minimum_alignment_2") {
        power = 1;
    }
    if cfg!(feature = "minimum_alignment_4") {
        power = 2;
    }
    if cfg!(feature = "minimum_alignment_8") {
        power = 3;
    }
    if cfg!(feature = "minimum_alignment_16") {
        power = 4;
    }
    if cfg!(feature = "minimum_alignment_32") {
        power = 5;
    }
    if cfg!(feature = "minimum_alignment_64") {
        power = 6;
    }
    if cfg!(feature = "minimum_alignment_128") {
        power = 7;
    }
    if cfg!(feature = "minimum_alignment_256") {
        power = 8;
    }
    if cfg!(feature = "minimum_alignment_512") {
        power = 9;
    }
    if cfg!(feature = "minimum_alignment_1024") {
        power = 10;
    }
    power
};

/// The alignment power used by all trace records that don't explicitly overwrite their alignment.
pub const ALIGNMENT_POWER: u8 = if DEFAULT_ALIGNMENT_POWER > MIN_ALIGNMENT_POWER {
    DEFAULT_ALIGNMENT_POWER
} else {
    MIN_ALIGNMENT_POWER
} as u8;

/// In the TRCE record layout, per-argument size flag: the argument stream has a fixed
/// byte size (given by the record's `size` field).
pub const EMT_FLAG_STATIC: usize = 0;
/// In the TRCE record layout, per-argument size flag: the argument stream is terminated
/// by the number of consecutive zero bytes given by the record's `size` field.
pub const EMT_FLAG_NULL_TERMINATED: usize = 1;
/// In the TRCE record layout, per-argument size flag: the argument stream is prefixed by
/// an element count whose byte-width is given by the record's `size` field, followed by
/// that many elements.
pub const EMT_FLAG_LENGTH_PREFIXED: usize = 2;

/// In the TRCE record layout, the formatter to use: python string.format
pub const EMT_PY_FORMAT: usize = 0;
/// In the TRCE record layout, the formatter to use: no formatting
pub const EMT_NO_FORMAT: usize = 1;
/// In the TRCE record layout, the formatter to use: C-style printf formatting
pub const EMT_C_STYLE_FORMAT: usize = 2;

/// In the MGIC record, the encoding id: raw binary stream
pub const EMT_ENCODING_NONE: u8 = 0;
/// In the MGIC record, the encoding id: COBS encoded stream
pub const EMT_ENCODING_COBS: u8 = 1;
/// In the MGIC record, the encoding id: COBS-encoded stream with raw passthrough
pub const EMT_ENCODING_COBS_PASSTHROUGH: u8 = 2;

/// The version of the layout of a MGIC record
pub const MAGIC_LAYOUT_VERSION: (u8, u8) = (1, 0);

/// The version of the layout of a TRCE record
pub const TRACE_LAYOUT_VERSION: (u8, u8) = (1, 0);

// Compatibility aliases for the pre-record-format names.

/// In the TRCE record layout, the formatter to use: python string.format
pub const PY_FORMAT: usize = EMT_PY_FORMAT;
/// In the TRCE record layout, the formatter to use: no formatting
pub const NO_FORMAT: usize = EMT_NO_FORMAT;
/// In the TRCE record layout, the formatter to use: C-style printf formatting
pub const C_STYLE_FORMAT: usize = EMT_C_STYLE_FORMAT;

/// In the MGIC record, the encoding id: raw binary stream
pub const RAW_ENCODING: u8 = EMT_ENCODING_NONE;
/// In the MGIC record, the encoding id: COBS encoded stream
pub const COBS_ENCCODING: u8 = EMT_ENCODING_COBS;
/// In the MGIC record, the encoding id: COBS encoded stream with raw passthrough
pub const COBS_PASSTHROUGH_ENCODING: u8 = EMT_ENCODING_COBS_PASSTHROUGH;

/// Describes how an argument's runtime stream is laid out.
#[derive(Copy, Clone, Debug)]
pub enum Size {
    /// The argument stream is always the specified number of bytes.
    Static(usize),
    /// The argument stream is terminated by the specified number of consecutive zero
    /// bytes.
    NullTerminated(u8),
    /// The argument stream is prefixed by an element count stored with the specified
    /// byte width, followed by the elements' streams.
    LengthPrefixed(usize),
}

impl Size {
    /// The value the record stores in its `flag` field for this `Size`.
    #[doc(hidden)]
    pub const fn flag(&self) -> usize {
        match self {
            Self::Static(_) => EMT_FLAG_STATIC,
            Self::NullTerminated(_) => EMT_FLAG_NULL_TERMINATED,
            Self::LengthPrefixed(_) => EMT_FLAG_LENGTH_PREFIXED,
        }
    }

    /// The value the record stores in its `size` field for this `Size`. The field's
    /// meaning depends on the flag value.
    #[doc(hidden)]
    pub const fn info_size(&self) -> usize {
        match self {
            Self::Static(size) => *size,
            Self::NullTerminated(count) => *count as usize,
            Self::LengthPrefixed(width) => *width,
        }
    }
}

/// The limit of descendants a type descriptor can have.
const DESCENDANTS_LIMIT: usize = 0x100;

/// An entry in a type's pre-order flattened descendant list.
///
/// Complex types (lists) provide their own descendants plus the descendants of their
/// single child: the first [`Trace::NUM_CHILDREN`] entries are the direct children (for
/// lists, the element type, named `""`); the remaining entries are the deeper
/// descendants in pre-order.
#[derive(Copy, Clone)]
pub struct Descendant {
    /// the name this descendant carries under its parent (e.g. `""` for list elements)
    pub name: &'static str,
    /// the descendant's type id (e.g. `"signed"`, `"string"`, `"list"`)
    pub id: &'static str,
    /// the descendant's runtime stream layout
    pub size: Size,
    /// how many direct children this descendant has
    pub num_children: usize,
}

/// A zero/padded descendant entry; used as the initial filler for `DESCENDANTS` arrays.
impl Descendant {
    #[doc(hidden)]
    pub const ZERO: Descendant = Descendant {
        name: "",
        id: "",
        size: Size::Static(0),
        num_children: 0,
    };
}

/// A trait implemented by every traceable type.
pub trait Trace {
    /// the runtime stream layout of this type's values (see [`Size`])
    const SIZE: Size;
    /// the type id recorded in the TRCE record (e.g. `"signed"`, `"string"`)
    const ID: &'static str;
    /// how many direct children the type's type-description has
    const NUM_CHILDREN: usize;
    /// pre-order flattened list of this type's descendants (without itself), see
    /// [`Descendant`]
    const DESCENDANTS: &'static [Descendant];

    /// number of TRCE layout entries this type requires as a *top-level* argument:
    /// 4 for itself plus 5 per descendant
    const NUM_LAY: usize = {
        let mut n = 4usize;
        let mut remaining = Self::NUM_CHILDREN;
        let mut i = 0;
        while remaining > 0 {
            let d = Self::DESCENDANTS[i];
            n += 5;
            remaining += d.num_children;
            remaining -= 1;
            i += 1;
        }
        n
    };

    /// number of string bytes this type contributes to the record's string area:
    /// its own id plus every descendant's name and id, all NUL terminated
    const STR_SIZE: usize = {
        let mut len = Self::ID.len() + 1;
        let mut remaining = Self::NUM_CHILDREN;
        let mut i = 0;
        while remaining > 0 {
            let d = Self::DESCENDANTS[i];
            len += d.name.len() + 1;
            len += d.id.len() + 1;
            remaining += d.num_children;
            remaining -= 1;
            i += 1;
        }
        len
    };

    /// number of descendants of this type (excluding itself)
    const NUM_DESC: usize = {
        let mut n = 0usize;
        let mut remaining = Self::NUM_CHILDREN;
        let mut i = 0;
        while remaining > 0 {
            let d = Self::DESCENDANTS[i];
            remaining += d.num_children;
            remaining -= 1;
            i += 1;
            n += 1;
        }
        n
    };

    /// serialize the *content* of `self`'s runtime stream into the sink.
    ///
    /// Length prefixes and terminators are written by [`emit_args`] (and by list
    /// serialization, which wraps each element).
    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError>;
    /// For lists: the element count. For other types: irrelevant.
    fn size(&self) -> usize;
}

/// Write an argument's runtime stream: length prefix (for length-prefixed types), the
/// value's content, and a terminator (for null-terminated types).
pub fn emit_args<T: Trace + ?Sized, S: Sink>(x: &T, f: &mut S) -> Result<(), S::OutError> {
    if let Size::LengthPrefixed(_) = <T as Trace>::SIZE {
        let n = x.size();
        Trace::serialize(&n, f)?;
    }
    Trace::serialize(x, f)?;
    if let Size::NullTerminated(count) = <T as Trace>::SIZE {
        out_terminator(f, count)?;
    }
    Ok(())
}

fn out_terminator<S: Sink>(f: &mut S, count: u8) -> Result<(), S::OutError> {
    let mut i = 0;
    while i < count {
        Trace::serialize(&0u8, f)?;
        i += 1;
    }
    Ok(())
}

impl<T> Trace for &T
where
    T: Trace,
    T: ?Sized,
{
    const SIZE: Size = T::SIZE;
    const ID: &'static str = T::ID;
    const NUM_CHILDREN: usize = T::NUM_CHILDREN;
    const DESCENDANTS: &'static [Descendant] = T::DESCENDANTS;
    #[inline]
    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        (*self).serialize(f)
    }
    #[inline]
    fn size(&self) -> usize {
        (*self).size()
    }
}

impl<T: Trace> Trace for Box<T> {
    const SIZE: Size = T::SIZE;
    const ID: &'static str = T::ID;
    const NUM_CHILDREN: usize = T::NUM_CHILDREN;
    const DESCENDANTS: &'static [Descendant] = T::DESCENDANTS;
    #[inline]
    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        self.as_ref().serialize(f)
    }
    #[inline]
    fn size(&self) -> usize {
        self.as_ref().size()
    }
}

impl<T: Trace, const N: usize> Trace for [T; N] {
    const SIZE: Size = Size::Static(N);
    const ID: &'static str = "list";
    const NUM_CHILDREN: usize = 1;
    const DESCENDANTS: &'static [Descendant] = <[T] as Trace>::DESCENDANTS;

    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        for el in self {
            emit_args(el, f)?;
        }
        Ok(())
    }
    #[inline]
    fn size(&self) -> usize {
        N
    }
}

impl<T: Trace> Trace for [T] {
    const SIZE: Size = Size::LengthPrefixed(size_of::<usize>());
    const ID: &'static str = "list";
    const NUM_CHILDREN: usize = 1;
    // pre-order: first the element type as the single child, then the element type's
    // own descendants. The array is padded with `Descendant::ZERO` (all loops are
    // driven by child counts, so padding entries are never read).
    const DESCENDANTS: &'static [Descendant] = &{
        let mut all = [Descendant::ZERO; DESCENDANTS_LIMIT];
        all[0] = Descendant {
            name: "",
            id: T::ID,
            size: T::SIZE,
            num_children: T::NUM_CHILDREN,
        };
        let mut i = 0;
        while i < T::NUM_DESC {
            all[i + 1] = T::DESCENDANTS[i];
            i += 1;
        }
        all
    };

    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        for el in self {
            emit_args(el, f)?;
        }
        Ok(())
    }
    #[inline]
    fn size(&self) -> usize {
        self.len()
    }
}

impl<T: Trace> Trace for Vec<T> {
    const SIZE: Size = <[T] as Trace>::SIZE;
    const ID: &'static str = <[T] as Trace>::ID;
    const NUM_CHILDREN: usize = <[T] as Trace>::NUM_CHILDREN;
    const DESCENDANTS: &'static [Descendant] = <[T] as Trace>::DESCENDANTS;

    #[inline]
    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        self[..].serialize(f)
    }
    #[inline]
    fn size(&self) -> usize {
        self[..].size()
    }
}

impl Trace for str {
    const SIZE: Size = Size::NullTerminated(1);
    const ID: &'static str = "string";
    const NUM_CHILDREN: usize = 0;
    const DESCENDANTS: &'static [Descendant] = &[];

    #[inline]
    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        f.out(self.as_bytes())
    }
    #[inline]
    fn size(&self) -> usize {
        self.len()
    }
}

impl Trace for String {
    const SIZE: Size = str::SIZE;
    const ID: &'static str = str::ID;
    const NUM_CHILDREN: usize = str::NUM_CHILDREN;
    const DESCENDANTS: &'static [Descendant] = str::DESCENDANTS;

    #[inline]
    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        self.as_str().serialize(f)
    }
    #[inline]
    fn size(&self) -> usize {
        self.as_str().size()
    }
}

macro_rules! impl_trace_for_primitive {
    ($type:ty, $id:expr) => {
        impl Trace for $type {
            const SIZE: Size = Size::Static(size_of::<Self>());
            const ID: &'static str = $id;
            const NUM_CHILDREN: usize = 0;
            const DESCENDANTS: &'static [Descendant] = &[];

            #[inline]
            fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
                f.out(&self.to_ne_bytes())
            }
            #[inline]
            fn size(&self) -> usize {
                size_of::<Self>()
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
    const SIZE: Size = Size::Static(size_of::<u8>());
    const ID: &'static str = "bool";
    const NUM_CHILDREN: usize = 0;
    const DESCENDANTS: &'static [Descendant] = &[];

    fn serialize<S: Sink>(&self, f: &mut S) -> Result<(), S::OutError> {
        f.out(&[*self as u8])
    }
    fn size(&self) -> usize {
        size_of::<u8>()
    }
}

/// A byte sink: decides where the trace bytes go and how the full stream is encoded.
pub trait Sink {
    /// The error type of this sink's operations.
    type OutError;
    /// The MGIC encoding id (see `TRACE_FORMAT.md`) that describes the byte stream this
    /// sink produces when used as the sink wrapped by the [`magic!`](crate::magic)
    /// macro. Defaults to `EMT_ENCODING_NONE` (raw stream); COBS sinks override this.
    const ENCODING_ID: u8 = EMT_ENCODING_NONE;
    /// Called before a trace is emitted.
    ///
    /// `record` points at the tracing record (the start of its framing bytes) and
    /// `total_size` is a lower-bound estimate of the number of bytes that this trace's
    /// runtime stream will emit.
    fn begin(&mut self, _record: *const u8, _total_size: usize) -> Result<(), Self::OutError> {
        Ok(())
    }
    /// Write raw bytes to the output.
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError>;
    /// Called after the last byte of a trace has been written.
    fn finish(&mut self) -> Result<(), Self::OutError> {
        Ok(())
    }
}

impl Sink for Vec<u8> {
    type OutError = Infallible;
    fn begin(&mut self, _record: *const u8, total_size: usize) -> Result<(), Self::OutError> {
        self.reserve(total_size);
        Ok(())
    }
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.extend_from_slice(b);
        Ok(())
    }
}

impl Sink for StdoutLock<'_> {
    type OutError = std::io::Error;
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.write_all(b)?;
        Ok(())
    }
}

/// The default sink of `trace!` and `traceln!`: it routes the trace's runtime stream
/// through the four `emt_default_*` C-ABI symbols, resolved at *link time* (see
/// [`magic!`](crate::magic) and `TRACE_FORMAT.md`).
///
/// These symbols are defined by the host of the final binary - the `magic!` macro
/// defines them in Rust, and the C `EMTRACE_MAGIC`-style macros in C. A binary that
/// uses `trace!`'s default sink must therefore define them (via `magic!`), or link an
/// artifact that does, or linking fails with the customary undefined-symbol errors -
/// exactly like in the C implementation.
pub struct DefaultSink {
    record: *const u8,
    total_size: usize,
}

impl DefaultSink {
    /// Constructs the default sink.
    ///
    /// Every `Sink` method of this type routes through the binary's host functions and
    /// frames the record stream: the constructed instance is only safe to drive from
    /// the [`trace`] macro itself, which runs `begin`, then any number of `out`s, then
    /// `finish` exactly once per record. Outside that protocol - calling `out` before
    /// `begin`, re-entering a record, or dropping the sink between the calls - the
    /// wrapper breaks the record framing contract of the linked host, so constructing
    /// it is `unsafe` and the trait macros are the only sanctioned construction site.
    ///
    /// # Safety
    /// The returned sink's trait methods must only be called as part of the trace
    /// macro's generated record flow (`begin` once, then `out`s, then `finish`).
    #[must_use]
    pub unsafe fn new() -> Self {
        Self {
            record: core::ptr::null(),
            total_size: 0,
        }
    }
}

impl Sink for DefaultSink {
    type OutError = Infallible;
    // The record pointer is only forwarded across the ABI, never dereferenced here.
    #[allow(clippy::not_unsafe_ptr_arg_deref)]
    fn begin(&mut self, record: *const u8, total_size: usize) -> Result<(), Self::OutError> {
        self.record = record;
        self.total_size = total_size;
        unsafe { emt_default_begin(record, total_size, core::ptr::null_mut()) };
        Ok(())
    }
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        unsafe { emt_default_out(b.as_ptr(), b.len(), core::ptr::null_mut()) };
        Ok(())
    }
    fn finish(&mut self) -> Result<(), Self::OutError> {
        unsafe { emt_default_finish(self.record, self.total_size, core::ptr::null_mut()) };
        Ok(())
    }
}

impl<T: Sink> Sink for MutexGuard<'_, T> {
    type OutError = T::OutError;
    fn begin(&mut self, record: *const u8, total_size: usize) -> Result<(), Self::OutError> {
        self.deref_mut().begin(record, total_size)
    }
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.deref_mut().out(b)
    }
    fn finish(&mut self) -> Result<(), Self::OutError> {
        self.deref_mut().finish()
    }
}

/// Shims a sink as the `extra_arg` of `emt_default_serialize_pointer`'s `out`
/// callback.
///
/// Sink errors cannot cross the C ABI boundary and are swallowed here, exactly like
/// in the C implementation (whose callbacks are `void`).
unsafe extern "C" fn sink_out_shim<S: Sink>(data: *const u8, size: usize, extra: *mut c_void) {
    let sink: *mut S = extra.cast();
    unsafe {
        let bytes = core::slice::from_raw_parts(data, size);
        let _ = (*sink).out(bytes);
    }
}

/// A sink that forwards every `out` call to a C `out` callback - the stateful,
/// struct-shaped equivalent of [`sink_out_shim`].
///
/// The host's generated `emt_default_serialize_pointer` uses it to run a
/// [`PtrSerializer`] written against the `out_ptr`/`&mut S` interface at the C ABI
/// boundary: the callback pair is bridged as a [`Sink`], forwards bytes verbatim to
/// the caller (Rust guests via the shim, C guests via their own out-chains), and
/// `begin`/`finish` are no-ops - the serializer interface never uses them.
#[doc(hidden)]
pub struct OutFnSink {
    out: EmtOutFn,
    extra: *mut c_void,
}

impl OutFnSink {
    /// # Safety
    /// Much like the [`DefaultSink`] the real thing that is unsafe here is the `out` function that
    /// is part of the Sink implementation, since it goes through the C ABI function [`emt_default_out`]
    #[must_use]
    pub const unsafe fn new(out: EmtOutFn, extra: *mut c_void) -> Self {
        Self { out, extra }
    }
}

impl Sink for OutFnSink {
    type OutError = Infallible;
    fn begin(&mut self, _record: *const u8, _total_size: usize) -> Result<(), Self::OutError> {
        Ok(())
    }
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        unsafe { (self.out)(b.as_ptr(), b.len(), self.extra) };
        Ok(())
    }
    fn finish(&mut self) -> Result<(), Self::OutError> {
        Ok(())
    }
}

/// Determines how to serialize the address of a record into a byte stream.
///
/// In principle the `out_ptr` function could serialize the `record` pointer it receives in any way
/// it wants as long as from the serialization the actual record it pointed to can be uniquely determined.
///
/// Realistically this means it will shift the address to the right by some number of bits, and then
/// output some number of the least significant bytes through the provided `sink`.
/// A generic implementation for exactly this behavior is available in the form of
/// [`ShiftPtrSerializer`].
pub trait PtrSerializer {
    /// The alignment power (of two) this serializer assumes every serialized record
    /// address to have.
    const ALIGNMENT_POWER: u8;
    /// The pointer payload width (in bytes) this serializer writes. Backing the host
    /// ([`magic!`](crate::magic)), this value is recorded as the MGIC record's pointer
    /// width.
    const PTR_WIDTH: usize;

    /// Serialize the record's address into the sink.
    fn out_ptr<S: Sink>(&mut self, record: *const u8, sink: &mut S) -> Result<(), S::OutError>;
}

/// The default pointer serializer of every trace pipeline: uses the global pointer serializer
/// created by the [`magic!`] macro by delegating completely to
/// the link-time `emt_default_serialize_pointer` symbol - exactly like the C
/// implementation's gates.
pub struct DefaultPtrSerializer;

// The record pointer is only forwarded across the ABI, never dereferenced here.
#[allow(clippy::not_unsafe_ptr_arg_deref)]
impl PtrSerializer for DefaultPtrSerializer {
    /// The host's built-in serializer (see [`magic!`](crate::magic)) shifts by this
    /// crate's alignment power and emits a `usize`-width payload.
    const ALIGNMENT_POWER: u8 = ALIGNMENT_POWER;
    const PTR_WIDTH: usize = size_of::<usize>();

    fn out_ptr<S: Sink>(&mut self, record: *const u8, sink: &mut S) -> Result<(), S::OutError> {
        unsafe {
            emt_default_serialize_pointer(
                record,
                sink_out_shim::<S>,
                core::ptr::from_mut(sink).cast::<c_void>(),
            );
        }
        Ok(())
    }
}

/// Simple pointer serializer that shifts the pointer by `SHIFT` bits to the right,
/// and then emits the `N_BYTES` least significant bytes of what's left through the provided sink
/// in its `out_ptr` implementation.
pub struct ShiftPtrSerializer<
    const N_BYTES: usize = { size_of::<usize>() },
    const SHIFT: u8 = ALIGNMENT_POWER,
>;

impl ShiftPtrSerializer {
    #[must_use]
    pub const fn new() -> Self {
        Self
    }
}

impl Default for ShiftPtrSerializer {
    fn default() -> Self {
        Self::new()
    }
}

impl<const N_BYTES: usize, const SHIFT: u8> PtrSerializer for ShiftPtrSerializer<N_BYTES, SHIFT> {
    /// The shifted address is assumed to be reconstructable from `N_BYTES` bytes,
    /// given record addresses aligned to `2^SHIFT`.
    const ALIGNMENT_POWER: u8 = SHIFT;
    const PTR_WIDTH: usize = N_BYTES;

    fn out_ptr<S: Sink>(&mut self, record: *const u8, sink: &mut S) -> Result<(), S::OutError> {
        let ptr = (record as usize) >> (SHIFT as usize);
        let bytes = if cfg!(target_endian = "big") {
            &ptr.to_ne_bytes()[size_of::<usize>() - N_BYTES..]
        } else {
            &ptr.to_ne_bytes()[..N_BYTES]
        };
        sink.out(bytes)
    }
}

/// Use to encode messages
pub struct CobsEncoder {
    buffer: [MaybeUninit<u8>; 254],
    pos: u8,
}

impl Default for CobsEncoder {
    fn default() -> Self {
        Self::new()
    }
}

impl CobsEncoder {
    #[must_use]
    pub fn new() -> Self {
        Self {
            buffer: [MaybeUninit::uninit(); 254],
            pos: 0,
        }
    }

    pub fn encode<S: Sink>(&mut self, sink: &mut S, b: &[u8]) -> Result<(), S::OutError> {
        let ptr: *mut u8 = self.buffer.as_mut_ptr().cast();
        // Safety: This gives us a view into `self.buffer` through which we can assume that every read
        // accesses initialized memory. We must ensure that this *is* in fact the case when
        // accessing it.
        //
        // It is ensured because there are only two positions where buffer contents are read in the code
        // below. And in both cases a slice of [..self.pos] is accessed, so as long as all memory
        // accessed is under the index self.pos this is safe. This invariant is guaranteed, because
        // self.pos is only every incremented in one place at the end of this function, where
        // simultaneously the byte at the previous pos is written to / initialized.
        let buffer: &mut [u8] = unsafe { slice::from_raw_parts_mut(ptr, 254) };

        for byte in b {
            if *byte == 0 {
                let len = self.pos + 1;
                sink.out(core::slice::from_ref(&len))?;
                sink.out(&buffer[..self.pos as usize])?;
                self.pos = 0;
                continue;
            }
            if self.pos == 254 {
                sink.out(&[255u8][..])?;
                sink.out(buffer)?;
                self.pos = 0;
            }
            self.buffer[self.pos as usize].write(*byte);
            self.pos += 1;
        }
        Ok(())
    }

    pub fn finalize<S: Sink>(&mut self, sink: &mut S) -> Result<(), S::OutError> {
        let ptr: *mut u8 = self.buffer.as_mut_ptr().cast();
        // Safety: This gives us a view into `self.buffer` through which we can assume that every read
        // accesses initialized memory. We must ensure that this *is* in fact the case when
        // accessing it.
        //
        // Only a slice of [..self.pos] of the slice is ever accessed below, and the only place
        // `self.pos` is incremented is in the function `encode`, where simultaneously the byte at
        // the previous pos is written to / initialized
        let buffer: &mut [u8] = unsafe { slice::from_raw_parts_mut(ptr, 254) };

        if self.pos > 0 {
            let len = self.pos + 1;
            sink.out(core::slice::from_ref(&len))?;
            sink.out(&buffer[..self.pos as usize])?;
        } else {
            sink.out(&[1u8][..])?;
        }
        let res = sink.out(&[0u8][..]);
        self.pos = 0;
        res
    }
}

/// A sink that encodes every [`trace!`] macro's output as a single COBS frame before forwarding the
/// resulting bytes to the `inner` [`Sink`]
pub struct CobsAdapter<'a, T: Sink> {
    inner: &'a mut T,
    encoder: CobsEncoder,
}

impl<'a, T: Sink> CobsAdapter<'a, T> {
    #[must_use]
    pub fn new(inner: &'a mut T) -> Self {
        Self {
            inner,
            encoder: CobsEncoder::new(),
        }
    }
}

impl<T: Sink> Sink for CobsAdapter<'_, T> {
    type OutError = T::OutError;
    const ENCODING_ID: u8 = EMT_ENCODING_COBS;
    fn begin(&mut self, record: *const u8, total_size: usize) -> Result<(), Self::OutError> {
        self.inner.begin(record, total_size)
    }
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.encoder.encode(self.inner, b)
    }
    fn finish(&mut self) -> Result<(), Self::OutError> {
        self.encoder.finalize(self.inner)?;
        self.inner.finish()
    }
}

/// A sink that does the same thing as [`CobsAdapter`], but owns the inner sink instead of holding
/// only a reference to it.
pub struct CobsConsumer<T: Sink> {
    inner: T,
    encoder: CobsEncoder,
}

impl<T: Sink> CobsConsumer<T> {
    #[must_use]
    pub fn new(inner: T) -> Self {
        Self {
            inner,
            encoder: CobsEncoder::new(),
        }
    }

    /// Consumes the consumer, returning the wrapped sink (e.g. to flush or close it
    /// at thread end).
    pub fn into_inner(self) -> T {
        self.inner
    }
}

impl<T: Sink> Sink for CobsConsumer<T> {
    type OutError = T::OutError;
    const ENCODING_ID: u8 = EMT_ENCODING_COBS;
    fn begin(&mut self, record: *const u8, total_size: usize) -> Result<(), Self::OutError> {
        self.inner.begin(record, total_size)
    }
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        self.encoder.encode(&mut self.inner, b)
    }
    fn finish(&mut self) -> Result<(), Self::OutError> {
        self.encoder.finalize(&mut self.inner)?;
        self.inner.finish()
    }
}

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------

/// Constants and const helpers to build records (see tests<trace_format.md>).
#[doc(hidden)]
pub mod record {
    use super::Trace;
    use core::mem::size_of;

    /// framing: 'EMT' + 7 metadata bytes + 'type tag'
    pub const FRAMING_SIZE: usize = 14;
    /// the byte-width of `size_t` in records
    pub const SZ: usize = size_of::<usize>();
    /// record-relative offset of the `record_size` field
    pub const RECORD_SIZE_OFF: usize = FRAMING_SIZE;
    /// record-relative offset of the endianness probe
    pub const PROBE_OFF: usize = RECORD_SIZE_OFF + SZ;
    /// record-relative offset of the payload
    pub const PAYLOAD_OFF: usize = PROBE_OFF + SZ;

    /// The endianness probe value with the record's `size_t` width.
    pub const fn probe() -> usize {
        let mut v: usize = 0;
        let mut i = 0;
        while i < SZ {
            v |= i << (8 * i);
            i += 1;
        }
        v
    }

    /// The size of a complete MGIC record (framing + record size + probe + payload).
    pub const MAGIC_PAYLOAD_SIZE: usize = 35;
    pub const MAGIC_SIZE: usize = PAYLOAD_OFF + MAGIC_PAYLOAD_SIZE;

    pub const fn write_sz(buf: &mut [u8], pos: &mut usize, v: usize) {
        let bytes = v.to_ne_bytes();
        let mut i = 0;
        while i < SZ {
            buf[*pos + i] = bytes[i];
            i += 1;
        }
        *pos += SZ;
    }

    /// Write a NUL terminated string; returns the offset it was written at.
    pub const fn write_str(buf: &mut [u8], pos: &mut usize, s: &str) -> usize {
        let off = *pos;
        let mut i = 0;
        while i < s.len() {
            buf[off + i] = s.as_bytes()[i];
            i += 1;
        }
        buf[off + s.len()] = 0;
        *pos += s.len() + 1;
        off
    }

    /// Write the 14-byte record framing plus the record's size and endianness probe.
    pub const fn write_frame(
        buf: &mut [u8],
        tag: &[u8; 4],
        major: u8,
        minor: u8,
        record_size: usize,
    ) {
        buf[0] = b'E';
        buf[1] = b'M';
        buf[2] = b'T';
        buf[3] = 10;
        buf[4] = RECORD_SIZE_OFF as u8;
        buf[5] = PAYLOAD_OFF as u8;
        buf[6] = PROBE_OFF as u8;
        buf[7] = SZ as u8;
        buf[8] = major;
        buf[9] = minor;
        buf[10] = tag[0];
        buf[11] = tag[1];
        buf[12] = tag[2];
        buf[13] = tag[3];
        let mut pos = RECORD_SIZE_OFF;
        write_sz(buf, &mut pos, record_size);
        write_sz(buf, &mut pos, probe());
    }

    /// Write a top-level type description and its descendants into the record.
    pub const fn write_top_type<T: Trace + ?Sized>(buf: &mut [u8], lp: &mut usize, dp: &mut usize) {
        let id_off = write_str(buf, dp, T::ID);
        write_sz(buf, lp, id_off);
        write_sz(buf, lp, T::SIZE.info_size());
        write_sz(buf, lp, T::SIZE.flag());
        write_sz(buf, lp, T::NUM_CHILDREN);
        let mut remaining = T::NUM_CHILDREN;
        let mut i = 0;
        while remaining > 0 {
            let d = T::DESCENDANTS[i];
            let name_off = write_str(buf, dp, d.name);
            let id_off = write_str(buf, dp, d.id);
            write_sz(buf, lp, name_off);
            write_sz(buf, lp, id_off);
            write_sz(buf, lp, d.size.info_size());
            write_sz(buf, lp, d.size.flag());
            write_sz(buf, lp, d.num_children);
            remaining += d.num_children;
            remaining -= 1;
            i += 1;
        }
    }

    /// Write the MGIC record's payload: 32 bytes of magic plus metadata byte fields.
    pub const fn write_magic(buf: &mut [u8], encoding: u8, ptr_width: usize, alignment_power: u8) {
        let mut pos = PAYLOAD_OFF;
        let magic = [
            0xd1, 0x97, 0xf5, 0x22, 0xd9, 0x26, 0x9f, 0xd1, 0xad, 0x70, 0x33, 0x92, 0xf6, 0x59,
            0xdf, 0xd0, 0xfb, 0xec, 0xbd, 0x60, 0x97, 0x13, 0x25, 0xe8, 0x92, 0x01, 0xb2, 0x5a,
            0x38, 0x5d, 0x9e, 0xc7,
        ];
        let mut i = 0;
        while i < 32 {
            buf[pos + i] = magic[i];
            i += 1;
        }
        pos += 32;
        buf[pos] = ptr_width as u8;
        pos += 1;
        buf[pos] = alignment_power;
        pos += 1;
        buf[pos] = encoding;
    }
}

/// Wrapper type for trace records stored in the `.emtrace` section.
///
/// All records are `2^ALIGNMENT_POWER`-byte aligned, so that record pointers can be
/// losslessly shifted at runtime (see TRACE_FORMAT.md). The alignment cannot live in
/// a single definition: `repr(align)` requires a literal, so the active definition is
/// selected behind the descending-priority `cfg_select!` ladder below. The
/// `align_of` assertion below verifies the ladder against [`ALIGNMENT_POWER`].
macro_rules! define_format_info {
    ($align:literal) => {
        #[repr(C, align($align))]
        pub struct FormatInfo<const LEN: usize> {
            pub bytes: [u8; LEN],
        }
    };
}

// The arms are ordered by descending alignment power and the first true arm wins:
// everything above it was already excluded by the arm order, so each arm only needs
// to test "is any feature of this power enabled" - no equivalence formulas are
// needed. The one compound arm is power 8: the 256-byte *fallback* also applies when
// no `default_alignment_*` feature is selected at all (in which case even small
// minimums cannot lower the effective alignment), and it is the arm that catches that
// configuration before the sub-256 arms could claim it. The `_` arm is provably
// unreachable - any configuration selects some arm - and makes any future drift of
// the feature list a loud compile error instead of a silently wrong alignment.
cfg_select! {
    any(feature = "minimum_alignment_1024", feature = "default_alignment_1024") => {
        define_format_info!(1024);
    }
    any(feature = "minimum_alignment_512", feature = "default_alignment_512") => {
        define_format_info!(512);
    }
    any(
        feature = "minimum_alignment_256",
        feature = "default_alignment_256",
        not(any(
            feature = "default_alignment_1",
            feature = "default_alignment_2",
            feature = "default_alignment_4",
            feature = "default_alignment_8",
            feature = "default_alignment_16",
            feature = "default_alignment_32",
            feature = "default_alignment_64",
            feature = "default_alignment_128",
            feature = "default_alignment_256",
            feature = "default_alignment_512",
            feature = "default_alignment_1024"
        ))
    ) => {
        define_format_info!(256);
    }
    any(feature = "minimum_alignment_128", feature = "default_alignment_128") => {
        define_format_info!(128);
    }
    any(feature = "minimum_alignment_64", feature = "default_alignment_64") => {
        define_format_info!(64);
    }
    any(feature = "minimum_alignment_32", feature = "default_alignment_32") => {
        define_format_info!(32);
    }
    any(feature = "minimum_alignment_16", feature = "default_alignment_16") => {
        define_format_info!(16);
    }
    any(feature = "minimum_alignment_8", feature = "default_alignment_8") => {
        define_format_info!(8);
    }
    any(feature = "minimum_alignment_4", feature = "default_alignment_4") => {
        define_format_info!(4);
    }
    any(feature = "minimum_alignment_2", feature = "default_alignment_2") => {
        define_format_info!(2);
    }
    feature = "default_alignment_1" => {
        define_format_info!(1);
    }
    _ => {
        compile_error!("internal error: alignment ladder fell through");
    }
}

const _: () = assert!(
    core::mem::align_of::<FormatInfo<{ record::MAGIC_SIZE }>>() == 1usize << ALIGNMENT_POWER
);

// The MGIC record and the runtime symbols, resolved at *link* time: the `magic!`
// macro defines them in Rust hosts, and the C `EMTRACE_MAGIC`-style macros in C hosts
// ("only the host artifact defines these", see TRACE_FORMAT.md). The default Rust
// sink ([`DefaultSink`]) routes through the functions, so host and guest artifacts
// - in either language - interoperate freely.
pub type EmtOutFn = unsafe extern "C" fn(*const u8, usize, *mut c_void);

unsafe extern "C" {
    #[doc(hidden)]
    pub static g_emt_default_magic: FormatInfo<{ record::MAGIC_SIZE }>;
    #[doc(hidden)]
    pub fn emt_default_begin(info: *const u8, total_size: usize, extra_arg: *mut c_void);
    #[doc(hidden)]
    pub fn emt_default_out(data: *const u8, size: usize, extra_arg: *mut c_void);
    #[doc(hidden)]
    pub fn emt_default_serialize_pointer(ptr: *const u8, out: EmtOutFn, extra_arg: *mut c_void);
    #[doc(hidden)]
    pub fn emt_default_finish(info: *const u8, total_size: usize, extra_arg: *mut c_void);
}

/// Consumes the `magic!` option list one token (or option) at a time. Each option is
/// stored in a bracketed state slot keyed on a leading marker literal - `[STDOUT]` /
/// `[COBS]` / `[GUARD <ty> <val>]` / `[NOSINK]`, `[]` / `[EX <expr>]` for scalar
/// states. A state slot is non-empty once its option has been seen, so duplicate
/// options, unknown options and syntax garbage all fall through to the final
/// `compile_error!` arm instead of silently overwriting a slot.
#[doc(hidden)]
#[macro_export]
macro_rules! __magic_munch {
    // All options consumed: dispatch to validation and emission.
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_emit!([$($s)*] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [$($sec)*]);
    };

    // ---- sink configuration (mutually exclusive) ----
    ([STDOUT] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*] .cobs $($rest:tt)*) => {
        $crate::__magic_munch!([COBS] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [$($sec)*] $($rest)*);
    };
    ([STDOUT] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]
     .sink_guard: $t:ty = $v:expr, $($rest:tt)*) => {
        $crate::__magic_munch!([GUARD $t $v] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [$($sec)*] $($rest)*);
    };
    ([STDOUT] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*] .sink_guard: $t:ty = $v:expr) => {
        $crate::__magic_munch!([GUARD $t $v] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [$($sec)*]);
    };
    ([STDOUT] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]
     .no_global_sink $($rest:tt)*) => {
        $crate::__magic_munch!([NOSINK] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [$($sec)*] $($rest)*);
    };

    // Option separator: consumed between options, so the individual option arms can
    // match without touching their separators (expr-valued options must still require
    // the comma explicitly, per the fragment-follower rules).
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*] , $($rest:tt)*) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [$($sec)*] $($rest)*);
    };

    // ---- encoding override (custom or record-only offerings) ----
    // Note: options ending in `$v:expr` need two forms each, since Rust's macro
    // fragment rules only allow `=>`, `,` or `;` to follow an `expr` fragment -
    // a comma for the mid-list form, and an exact-match form for the trailing option.
    ([$($s:tt)*] [] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*] .encoding = $v:expr, $($rest:tt)*) => {
        $crate::__magic_munch!([$($s)*] [EX $v] [$($p)*] [$($w)*] [$($a)*] [$($sec)*] $($rest)*);
    };
    ([$($s:tt)*] [] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*] .encoding = $v:expr) => {
        $crate::__magic_munch!([$($s)*] [EX $v] [$($p)*] [$($w)*] [$($a)*] [$($sec)*]);
    };

    // ---- pointer serializer configuration (mutually exclusive) ----
    ([$($s:tt)*] [$($e:tt)*] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]
     .ptr_serializer: $t:ty = $v:expr, $($rest:tt)*) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [CUSTOM $t $v] [$($w)*] [$($a)*] [$($sec)*] $($rest)*);
    };
    ([$($s:tt)*] [$($e:tt)*] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*] .ptr_serializer: $t:ty = $v:expr) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [CUSTOM $t $v] [$($w)*] [$($a)*] [$($sec)*]);
    };
    ([$($s:tt)*] [$($e:tt)*] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]
     .no_global_ptr_serializer $($rest:tt)*) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [NONE] [$($w)*] [$($a)*] [$($sec)*] $($rest)*);
    };

    // ---- record-only pointer dimensions (`.no_global_ptr_serializer` only) ----
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [] [$($a:tt)*] [$($sec:tt)*]
     .ptr_width = $v:expr, $($rest:tt)*) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [$($p)*] [EX $v] [$($a)*] [$($sec)*] $($rest)*);
    };
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [] [$($a:tt)*] [$($sec:tt)*] .ptr_width = $v:expr) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [$($p)*] [EX $v] [$($a)*] [$($sec)*]);
    };
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [] [$($sec:tt)*]
     .alignment_power = $v:expr, $($rest:tt)*) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [$($p)*] [$($w)*] [EX $v] $($rest)*);
    };
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [] .alignment_power = $v:expr) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [$($p)*] [$($w)*] [EX $v]);
    };

    // ---- section override (MGIC + record placement) ----
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] []
     .section = $sec:literal, $($rest:tt)*) => {
        $crate::__magic_munch!(
            [$($s)*] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [EX $sec] $($rest)*
        );
    };
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] []
     .section = $sec:literal) => {
        $crate::__magic_munch!([$($s)*] [$($e)*] [$($p)*] [$($w)*] [$($a)*] [EX $sec]);
    };

    // Unrecognized or duplicated option (any state slot is non-empty on a repeat), or
    // syntactically broken input; reports the full accepted surface.
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*] $($rest:tt)+) => {
        ::core::compile_error!(
            concat!(
                "invalid `magic!` configuration. Valid options: `.cobs`, ",
                "`.sink_guard: <type> = <expr>`, `.no_global_sink`, `.encoding = <u8>` ",
                "(only for a custom sink or with `.no_global_sink`), ",
                "`.ptr_serializer: <type> = <expr>`, `.no_global_ptr_serializer`, ",
                "`.ptr_width = <usize>`, `.alignment_power = <u8>` ",
                "(both only with `.no_global_ptr_serializer`), ",
                "`.section = <&str literal>` (must match the `.section` of the `trace!` ",
                "calls whose records should live in the same section); each at most once"
            )
        );
    };
}

/// Validates the consumed [`magic!`](crate::magic) states and emits the MCIG record,
/// the optional sink host, and the optional pointer-serializer host.
#[doc(hidden)]
#[macro_export]
macro_rules! __magic_emit {
    // ----- invalid combinations -----
    // `.encoding` overrides a fixed advertisement of the built-in sink arms (they
    // cannot be made to honor a different encoding).
    ([STDOUT] [EX $($e:tt)+] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        ::core::compile_error!(
            "`.encoding` cannot be set for the default `magic!` stdout sink"
        );
    };
    ([COBS] [EX $($e:tt)+] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        ::core::compile_error!(
            "`.encoding` cannot be set for the `magic!` `.cobs` sink"
        );
    };
    // `.ptr_width`/`.alignment_power` describe a stream without a host serializer;
    // with a linked one, the linked serializer owns both values (the MGIC record must
    // not contradict it).
    ([$($s:tt)*] [$($e:tt)*] [] [EX $($w:tt)+] [$($a:tt)*] [$($sec:tt)*]) => {
        ::core::compile_error!(
            "`.ptr_width` can only be used with `.no_global_ptr_serializer`"
        );
    };
    ([$($s:tt)*] [$($e:tt)*] [] [$($w:tt)*] [EX $($a:tt)+] [$($sec:tt)*]) => {
        ::core::compile_error!(
            "`.alignment_power` can only be used with `.no_global_ptr_serializer`"
        );
    };
    ([$($s:tt)*] [$($e:tt)*] [CUSTOM $($p:tt)+] [EX $($w:tt)+] [$($a:tt)*] [$($sec:tt)*]) => {
        ::core::compile_error!(
            "`<ty>::PTR_WIDTH` of a `.ptr_serializer` cannot be overridden with `.ptr_width`"
        );
    };
    ([$($s:tt)*] [$($e:tt)*] [CUSTOM $($p:tt)+] [$($w:tt)*] [EX $($a:tt)+] [$($sec:tt)*]) => {
        ::core::compile_error!(
            "`<ty>::ALIGNMENT_POWER` of a `.ptr_serializer` cannot be overridden with \
             `.alignment_power`"
        );
    };

    // ----- stdout sink (the default) -----
    ([STDOUT] [$($e:tt)*] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $crate::EMT_ENCODING_NONE,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::PTR_WIDTH,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [::std::option::Option<::std::io::StdoutLock<'static>>],
            [::std::io::stdout().lock()]
        );
        $crate::__magic_ser_fn!();
    };
    ([STDOUT] [$($e:tt)*] [CUSTOM $t:tt $v:tt] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $crate::EMT_ENCODING_NONE,
            <$t as $crate::PtrSerializer>::PTR_WIDTH,
            <$t as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [::std::option::Option<::std::io::StdoutLock<'static>>],
            [::std::io::stdout().lock()]
        );
        $crate::__magic_ser_fn!($t, $v);
    };
    ([STDOUT] [$($e:tt)*] [NONE] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record_noser!([$($sec)*], $crate::EMT_ENCODING_NONE, [$($w)*], [$($a)*]);
        $crate::__magic_host!(
            [::std::option::Option<::std::io::StdoutLock<'static>>],
            [::std::io::stdout().lock()]
        );
    };

    // ----- COBS-encoded stdout sink -----
    ([COBS] [$($e:tt)*] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $crate::EMT_ENCODING_COBS,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::PTR_WIDTH,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [
                ::std::option::Option<
                    $crate::CobsConsumer<::std::io::StdoutLock<'static>>
                >
            ],
            [$crate::CobsConsumer::new(::std::io::stdout().lock())]
        );
        $crate::__magic_ser_fn!();
    };
    ([COBS] [$($e:tt)*] [CUSTOM $t:tt $v:tt] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $crate::EMT_ENCODING_COBS,
            <$t as $crate::PtrSerializer>::PTR_WIDTH,
            <$t as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [
                ::std::option::Option<
                    $crate::CobsConsumer<::std::io::StdoutLock<'static>>
                >
            ],
            [$crate::CobsConsumer::new(::std::io::stdout().lock())]
        );
        $crate::__magic_ser_fn!($t, $v);
    };
    ([COBS] [$($e:tt)*] [NONE] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record_noser!([$($sec)*], $crate::EMT_ENCODING_COBS, [$($w)*], [$($a)*]);
        $crate::__magic_host!(
            [
                ::std::option::Option<
                    $crate::CobsConsumer<::std::io::StdoutLock<'static>>
                >
            ],
            [$crate::CobsConsumer::new(::std::io::stdout().lock())]
        );
    };

    // ----- custom sink (thread-local slot), encoding from the sink trait -----
    ([GUARD $gt:tt $gv:tt] [] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            <$gt as $crate::Sink>::ENCODING_ID,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::PTR_WIDTH,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [::std::option::Option<$gt>],
            [$gv]
        );
        $crate::__magic_ser_fn!();
    };
    ([GUARD $gt:tt $gv:tt] [] [CUSTOM $t:tt $v:tt] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            <$gt as $crate::Sink>::ENCODING_ID,
            <$t as $crate::PtrSerializer>::PTR_WIDTH,
            <$t as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [::std::option::Option<$gt>],
            [$gv]
        );
        $crate::__magic_ser_fn!($t, $v);
    };
    ([GUARD $gt:tt $gv:tt] [] [NONE] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record_noser!([$($sec)*],
            <$gt as $crate::Sink>::ENCODING_ID, [$($w)*], [$($a)*]
        );
        $crate::__magic_host!(
            [::std::option::Option<$gt>],
            [$gv]
        );
    };

    // ----- custom sink, explicit `.encoding` override -----
    ([GUARD $gt:tt $gv:tt] [EX $ge:tt] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $ge,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::PTR_WIDTH,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [::std::option::Option<$gt>],
            [$gv]
        );
        $crate::__magic_ser_fn!();
    };
    ([GUARD $gt:tt $gv:tt] [EX $ge:tt] [CUSTOM $t:tt $v:tt] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $ge,
            <$t as $crate::PtrSerializer>::PTR_WIDTH,
            <$t as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_host!(
            [::std::option::Option<$gt>],
            [$gv]
        );
        $crate::__magic_ser_fn!($t, $v);
    };
    ([GUARD $gt:tt $gv:tt] [EX $ge:tt] [NONE] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record_noser!([$($sec)*], $ge, [$($w)*], [$($a)*]);
        $crate::__magic_host!(
            [::std::option::Option<$gt>],
            [$gv]
        );
    };

    // ----- no global sink (record-only configurations) -----
    ([NOSINK] [] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $crate::EMT_ENCODING_NONE,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::PTR_WIDTH,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_ser_fn!();
    };
    ([NOSINK] [] [CUSTOM $t:tt $v:tt] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $crate::EMT_ENCODING_NONE,
            <$t as $crate::PtrSerializer>::PTR_WIDTH,
            <$t as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_ser_fn!($t, $v);
    };
    ([NOSINK] [] [NONE] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record_noser!([$($sec)*], $crate::EMT_ENCODING_NONE, [$($w)*], [$($a)*]);
    };
    ([NOSINK] [EX $ge:tt] [] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $ge,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::PTR_WIDTH,
            <$crate::DefaultPtrSerializer as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_ser_fn!();
    };
    ([NOSINK] [EX $ge:tt] [CUSTOM $t:tt $v:tt] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record!([$($sec)*],
            $ge,
            <$t as $crate::PtrSerializer>::PTR_WIDTH,
            <$t as $crate::PtrSerializer>::ALIGNMENT_POWER
        );
        $crate::__magic_ser_fn!($t, $v);
    };
    ([NOSINK] [EX $ge:tt] [NONE] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        $crate::__magic_record_noser!([$($sec)*], $ge, [$($w)*], [$($a)*]);
    };

    // Provably unreachable: `_` keeps future state drift a loud compile error.
    ([$($s:tt)*] [$($e:tt)*] [$($p:tt)*] [$($w:tt)*] [$($a:tt)*] [$($sec:tt)*]) => {
        ::core::compile_error!("internal error: `magic!` state machine fell through");
    };
}

/// The MGIC record dispatcher of [`magic!`](crate::magic): resolves the `.section`
/// state to the literal for the record's `link_section` attribute. Attributes accept
/// only literals, so the default (`.emtrace`) must be filled in *here* rather than at
/// the attribute site.
#[doc(hidden)]
#[macro_export]
macro_rules! __magic_record {
    ([], $enc:expr, $ptr_width:expr, $alignment_power:expr) => {
        $crate::__magic_record_sec!(".emtrace", $enc, $ptr_width, $alignment_power);
    };
    ([EX $sec:tt], $enc:expr, $ptr_width:expr, $alignment_power:expr) => {
        $crate::__magic_record_sec!($sec, $enc, $ptr_width, $alignment_power);
    };
}

/// The MGIC record emitter behind [`__magic_record!`](crate::__magic_record) - C parity
/// with `EMT_MAGIC`: the record encodes the pipeline's encoding id, pointer payload
/// width and alignment power, so that a parser can decode every other record in the
/// binary, and lands in the section named by `.section` (default `.emtrace`).
///
/// `ptr_width` and `alignment_power` are recorded verbatim: a custom host serializer
/// (`.ptr_serializer`) declares both via its [`PtrSerializer`] impl; record-only hosts
/// (`.no_global_ptr_serializer`) accept them as explicit options and otherwise get the
/// crate's built-in dimensions.
#[doc(hidden)]
#[macro_export]
macro_rules! __magic_record_sec {
    ($sec:literal, $enc:expr, $ptr_width:expr, $alignment_power:expr) => {
        #[used]
        #[unsafe(no_mangle)]
        #[unsafe(link_section = $sec)]
        pub static g_emt_default_magic: $crate::FormatInfo<{ $crate::record::MAGIC_SIZE }> = {
            let mut bytes = [0u8; $crate::record::MAGIC_SIZE];
            $crate::record::write_frame(
                &mut bytes,
                b"MGIC",
                $crate::MAGIC_LAYOUT_VERSION.0,
                $crate::MAGIC_LAYOUT_VERSION.1,
                $crate::record::MAGIC_SIZE as usize,
            );
            $crate::record::write_magic(&mut bytes, $enc as u8, $ptr_width, $alignment_power);
            $crate::FormatInfo { bytes }
        };
    };
}

/// The MGIC record for record-only hosts (`.no_global_ptr_serializer`): resolves the
/// pointer-width/alignment-power options, defaulting either to the built-in
/// dimensions (`usize` payload, the crate's alignment power) when left unset. The
/// `.section` state passes through unchanged.
#[doc(hidden)]
#[macro_export]
macro_rules! __magic_record_noser {
    ([$($sec:tt)*], $enc:expr, [], []) => {
        $crate::__magic_record!([$($sec)*],
            $enc,
            ::core::mem::size_of::<usize>(),
            $crate::ALIGNMENT_POWER
        );
    };
    ([$($sec:tt)*], $enc:expr, [EX $w:tt], []) => {
        $crate::__magic_record!([$($sec)*], $enc, $w, $crate::ALIGNMENT_POWER);
    };
    ([$($sec:tt)*], $enc:expr, [], [EX $a:tt]) => {
        $crate::__magic_record!([$($sec)*], $enc, ::core::mem::size_of::<usize>(), $a);
    };
    ([$($sec:tt)*], $enc:expr, [EX $w:tt], [EX $a:tt]) => {
        $crate::__magic_record!([$($sec)*], $enc, $w, $a);
    };
}

/// The pointer-serializer host of [`magic!`](crate::magic): defines the link-time
/// `emt_default_serialize_pointer` symbol that the default Rust path (via
/// [`DefaultPtrSerializer`]) and all C guests resolve.
///
/// The built-in form (C parity with `EMT_FILE`) shifts the pointer by this crate's
/// alignment power and emits it through the *caller's* own out callback. The
/// `.ptr_serializer` form instead constructs a fresh serializer per call and runs it
/// against an [`OutFnSink`] bridge: the bridge forwards every byte to the same caller
/// callback, so C guests keep control of their own output chain, and a serializer's
/// per-call state lives exactly as long as one serialization.
#[doc(hidden)]
#[macro_export]
macro_rules! __magic_ser_fn {
    () => {
        #[unsafe(no_mangle)]
        unsafe extern "C" fn emt_default_serialize_pointer(
            ptr: *const u8,
            out: $crate::EmtOutFn,
            extra_arg: *mut ::core::ffi::c_void,
        ) {
            unsafe {
                // Shift the pointer into an `emt_ptr_t`, then hand the bytes to the
                // *passed* output function. `ptr` derefability is an ABI assumption,
                // like the user's `extra_arg` for the `out` callback.
                let serialized_ptr = (ptr as usize) >> ($crate::ALIGNMENT_POWER as usize);
                out(
                    ::core::ptr::addr_of!(serialized_ptr).cast(),
                    ::core::mem::size_of::<usize>(),
                    extra_arg,
                );
            }
        }
    };
    ($t:tt, $v:tt) => {
        #[unsafe(no_mangle)]
        unsafe extern "C" fn emt_default_serialize_pointer(
            ptr: *const u8,
            out: $crate::EmtOutFn,
            extra_arg: *mut ::core::ffi::c_void,
        ) {
            let mut serializer: $t = $v;
            let mut sink_bridge = $crate::OutFnSink::new(out, extra_arg);
            // The bridge forwards to the caller's own out-chain; errors cannot
            // cross the C ABI boundary and are swallowed here, like in C.
            let _ = $crate::PtrSerializer::out_ptr(&mut serializer, ptr, &mut sink_bridge);
        }
    };
}

/// The sink host of [`magic!`](crate::magic) (C parity with `EMT_FILE`): declares the
/// thread-local per-record sink slot and defines the three record-flow functions
/// `emt_default_begin`/`emt_default_out`/`emt_default_finish`.
///
/// `slot_ty` is the full slot state type (including its `Option<>` wrapper) and
/// `guard` the per-record guard expression installed in `emt_default_begin`.
#[doc(hidden)]
#[macro_export]
macro_rules! __magic_host {
    // Slot ty and guard are bracket-wrapped token groups (they can be multi-token);
    // re-spliced verbatim into the generated pipeline below.
    ([$($slot_ty:tt)*], [$($guard:tt)*]) => {
        // Mirrors the C implementation's `static per-record` state, but as
        // *thread-local* storage: records on different threads never touch one
        // another's slot state, so the slot needs no synchronization of its own. Each
        // record installs a freshly evaluated guard, which is dropped in
        // `emt_default_finish`; the wrapped sink's own lock (the stdout lock for the
        // default arms, a `MutexGuard` for `Mutex`-backed sinks) therefore still spans
        // `emt_default_begin` through `emt_default_finish`, mirroring the C
        // implementation's whole-record `flockfile`/`funlockfile` span.
        ::std::thread_local! {
            static __EMT_SINK_SLOT: ::core::cell::RefCell<$($slot_ty)*> =
                ::core::cell::RefCell::new(::core::option::Option::None);
        }

        #[unsafe(no_mangle)]
        unsafe extern "C" fn emt_default_begin(
            info: *const u8,
            total_size: usize,
            _extra_arg: *mut ::core::ffi::c_void,
        ) {
            __EMT_SINK_SLOT.with(|slot| {
                // The guard expression may itself trace (evaluating it before the slot
                // borrow keeps such inner traces from hitting the slot's re-entrancy
                // panic); the debug assert below then catches records that *overlap*
                // this one, i.e. second record starting while this one is in flight.
                let fresh = $($guard)*;
                let mut slot = slot.borrow_mut();
                ::core::debug_assert!(
                    slot.is_none(),
                    "emtrace: nested or concurrent trace record in flight"
                );
                *slot = Some(fresh);
                if let Some(sink) = slot.as_mut() {
                    let _ = $crate::Sink::begin(sink, info, total_size);
                }
            });
        }

        #[unsafe(no_mangle)]
        unsafe extern "C" fn emt_default_out(
            data: *const u8,
            size: usize,
            _extra_arg: *mut ::core::ffi::c_void,
        ) {
            // `data`..`data + size` pointing at valid bytes for the call is the C ABI
            // contract between this host and any emit site (Rust or C).
            let bytes = unsafe { ::core::slice::from_raw_parts(data, size) };
            __EMT_SINK_SLOT.with(|slot| {
                if let Some(sink) = slot.borrow_mut().as_mut() {
                    let _ = $crate::Sink::out(sink, bytes);
                }
            });
        }

        #[unsafe(no_mangle)]
        unsafe extern "C" fn emt_default_finish(
            _info: *const u8,
            _total_size: usize,
            _extra_arg: *mut ::core::ffi::c_void,
        ) {
            __EMT_SINK_SLOT.with(|slot| {
                let sink = slot.borrow_mut().take();
                if let Some(mut sink) = sink {
                    let _ = $crate::Sink::finish(&mut sink);
                } // ... and drop the sink
            });
        }
    };
}

/// Create the MGIC record that the emtrace parser requires to interpret the bytestream
/// generated by the `trace!` macros correctly. By default, and mirroring the C
/// `EMTRACE_MAGIC` macros, it also defines the global sink and pointer serializer
/// behind the four `emt_default_*` link-time symbols: default `trace!` calls and C
/// guests resolve them at *link* time.
///
/// Per linked binary there should only be *a single* `magic!` invocation,
/// so you will probably want to place this next to your main function / your program's
/// entry point.
///
/// `trace!` calls can bypass the global sink and pointer serializer: per `trace!` call,
/// `.sink = ...` or `.sink_guard = ...` replaces the sink, and `.ptr_serializer = ...`
/// replaces the pointer serializer. With `.no_global_ptr_serializer` (or `.no_global_sink`),
/// every default-sink [`trace!`] call becomes a *link* error - just like the C `EMT_MAGIC`
/// macros, which only define the record.
///
/// Supported configuration (options in any order; unknown options and conflicting
/// combinations are `compile_error!`s):
///
/// **Sink** (mutually exclusive, default: stdout):
///
/// - none (the default): wraps `std::io::stdout()`. Magic record advertises
///   `EMT_ENCODING_NONE`.
/// - **`.sink_guard: <type> = <expr>`**: wraps a sink of *this* type instead of
///   stdout. `<type>` must be spelled out because the per-record slot is a static
///   whose type cannot be inferred from the expression. The magic record's encoding
///   id is taken from [`Sink::ENCODING_ID`](Sink) of the named type.
/// - **`.no_global_sink`**: emits the MGIC record (C parity with `EMT_MAGIC` *without*
///   the serializer host) and no sink's host functions: the `.emt_default_begin/out/finish`
///   symbols are not defined by this invocation, so every `trace!` call must supply its
///   own `.sink`/`.sink_guard` (or the definitions come from another artifact, as in C).
///   `.encoding = <u8>` may be used to advertise the actual stream encoding (defaults
///   to `EMT_ENCODING_NONE`).
/// - **`.cobs`**: wraps a [`CobsConsumer`] around whatever other sink is used (if any)
///   and sets the encoding id in the MGIC record to `EMT_ENCODING_COBS` so
///   the parser knows how to decode the bytestream that is output through the sink.
///   Call [`init`] with a `CobsAdapter` as well, so that the initialization frame is
///   COBS-encoded like the runtime stream.
/// - **`.encoding = <expr>`** (with `.sink_guard` or `.no_global_sink`): explicitly overwrite
///   the encoding id.
///
/// **Pointer serializer** (mutually exclusive, default: built-in shift serializer):
///
/// - **`.ptr_serializer: <type> = <expr>`**: backs the link-time
///   `emt_default_serialize_pointer` with this serializer: the MGIC record takes its
///   pointer width ([`PtrSerializer::PTR_WIDTH`]) and alignment power
///   ([`PtrSerializer::ALIGNMENT_POWER`]) from `<type>`, and a fresh serializer value
///   (constructed from `<expr>` per call) serializes the pointer through the *calling*
///   pipeline's out chain. Since C guests drive this symbol from their own pipelines,
///   their output stays byte-identical to the built-in behavior. Note that
///   `.ptr_serializer = DefaultPtrSerializer` would delegate the symbol recursively -
///   use any other serializer.
/// - **`.no_global_ptr_serializer`**: leaves `emt_default_serialize_pointer` undefined
///   and the MGIC encoding descriptive: *every* `trace!` call must supply
///   `.ptr_serializer = ...` (or the symbol comes from another artifact).
/// - **`.ptr_width = <usize>`** / **`.alignment_power = <u8>`** (only with
///   `.no_global_ptr_serializer`): independently set the record's advertised pointer
///   width (default: `size_of::<usize>()`) and alignment power (default: this crate's
///   `ALIGNMENT_POWER`). The record's *physical* `.emtrace` alignment is unaffected -
///   records are aligned by this crate's feature configuration, which must therefore
///   be at least as strong as the advertised power.
///
/// **Section** (default: `.emtrace`):
///
/// - **`.section = "name"`**: places the MGIC record in this section instead of
///   `.emtrace` (C parity with overriding `EMT_DEFAULT_SEC_ATTR`). Trace calls whose
///   records should be decodable must pass the *same* `.section = "name"` (see
///   [`trace!`]): the parser resolves record pointers as offsets within one section,
///   so a record static in a different section than the MGIC record breaks decoding.
///   `emtrace --section-name` selects which section to read; when the named section
///   is absent, the parser falls back to scanning the whole file for the MGIC record.
#[macro_export]
macro_rules! magic {
    ($($opts:tt)*) => {
        $crate::__magic_munch!([STDOUT] [] [] [] [] [] $($opts)*);
    };
}

/// Initialize the trace stream: emit the address of the magic record so that a parser
/// can determine the ASLR offset. Must be called before the first trace.
pub fn init<T: Sink>(sink: &mut T) -> Result<(), T::OutError> {
    let magic = core::ptr::addr_of!(g_emt_default_magic) as *const u8;
    sink.begin(magic, record::MAGIC_SIZE)?;
    let mut ptr = DefaultPtrSerializer;
    ptr.out_ptr(magic, sink)?;
    sink.finish()
}

/// The pointer serializer selected for a trace pipeline: the `.ptr_serializer` option
/// if given, the [`DefaultPtrSerializer`] otherwise.
#[doc(hidden)]
#[macro_export]
macro_rules! __ptr_serializer {
    () => {
        $crate::DefaultPtrSerializer
    };
    (.ptr_serializer = $serializer:expr) => {
        $serializer
    };
}

#[macro_export]
macro_rules! sink {
    () => {
        // The record protocol (begin -> out -> finish per record) is driven by the
        // trace macro; the explicit unsafe block marks DefaultSink's construction as
        // the one protocol-bound step - see `DefaultSink::new`.
        unsafe { $crate::DefaultSink::new() }
    };
    (.sink_guard = $sink_guard:expr,) => {
        $sink_guard
    };
    (.sink_guard = $sink_guard:expr, .sink = $sink:expr,) => {{
        const _: () = assert!(
            false,
            "Only one of '.sink', and '.sink_guard' may be specified"
        );
        ()
    }};
    (.sink = $sink:expr,) => {
        ()
    };
}

/// The core macro for emitting a trace.
///
/// `trace_impl!` is the main macro that `trace!` and `traceln!` are built on.
///
/// The format string is the concatenation of all string literals before the first comma.
/// After that come the values to be emitted, along with their types, e.g.:
///
/// ```rust
/// # use emtrace::{magic, trace_impl, Sink};
/// # fn main() {
/// magic!();
/// let mut buf = Vec::new();
/// trace_impl!("{} + {}" " = {}", i32: 1, i32: 2, i32: 3, .sink = buf);
/// # }
/// ```
///
/// A type can be made traceable by implementing the `Trace` trait for it.
///
/// ### Customization
///
/// `trace_impl!` supports several customization options, passed after the trace arguments:
///
/// - **`.sink = <variable>`**: Specifies a sink to write the trace output to. The macro will
///   mutably borrow the value for the duration of the call. It must implement the
///   [`Sink`] trait. For example one could capture trace output in a buffer like so:
///   ```rust
///   # use emtrace::{magic, trace_impl, Sink};
///   # fn main() {
///   magic!();
///   let mut buf = Vec::new();
///   trace_impl!("Hello", .sink = buf);
///   // `buf` is still owned and contains the trace output
///   assert!(!buf.is_empty());
///   # }
///   ```
///
/// - **`.sink_guard = <expr>`**: Specifies a sink that will be consumed by the macro. Like with
///   `.sink=` the expression has to  evaluate to a value that implements [`Sink`].
///   This value is moved into the macro and dropped at the end of the trace_impl invocation.
///   This is ideal for sinks that manage a resource, like a `MutexGuard`,
///   as it ensures the resource is released correctly.
///   ```rust
///   # use emtrace::{magic, trace_impl, Sink};
///   # use std::sync::Mutex;
///   # fn main() {
///   magic!();
///   let sink = Mutex::new(Vec::new());
///   trace_impl!("Hello", .sink_guard = sink.lock().unwrap());
///   // The mutex is unlocked here as the guard has been dropped.
///   # }
///   ```
///   By default, if neither `.sink` nor `.sink_guard` is provided, the trace output is performed
///   through the sink globally registered by the [`magic!`] macro
///
/// - **`.section = "<section_name>"`**: Specifies the ELF section where the format string and
///   metadata are stored. Defaults to `.emtrace`.
///
/// - **`.ptr_serializer = <expr>`**: Replaces the pipeline's pointer serializer.
///   This must be an object of a type implementing the [`PtrSerializer`] trait. The first thing
///   output through the sink by every trace is a pointer that allows the parser to associate the
///   trace with the correct format string and type information embedded in the compiled executable.
///
/// - **`.formatter = <expr>`**: Specifies how the format string should be interpreted by the consumer
///   of the trace data. It can be one of `EMT_PY_FORMAT`, `EMT_C_STYLE_FORMAT`, or `EMT_NO_FORMAT`.
///   By default it is `EMT_PY_FORMAT` when at least one value is provided after the format string,
///   and `EMT_NO_FORMAT` otherwise.
///
/// - **`.error_handler = <closure>`**: Provides a closure to handle potential errors during tracing.
///   The closure receives a `Result<(), Sink::OutError>`. If not provided, the `Result`
///   is returned from the macro expansion.
///   ```rust
///   # use emtrace::{magic, trace_impl, Sink};
///   # fn main() {
///   magic!();
///   trace_impl!("Hello", .sink_guard = std::io::stdout().lock(), .error_handler = |res: Result<(), std::io::Error>| {
///       if res.is_err() {
///           // Handle error
///       }
///   });
///   # }
///   ```
///
/// The macro returns the output of the provided error handler.
#[macro_export]
macro_rules! trace_impl {
    ($($fmt:literal)+ $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .sink_guard = $sink_guard:expr)? $(, .sink = $sink:expr)? $(, .ptr_serializer = $ptr_serializer:expr)? $(, .error_handler = $error_handler:expr)? $(, .formatter=$formatter:expr)?)
     =>
    {
        {
            let result = 'trace: {
                const FMT: &str = core::concat!($($fmt,)+);
                const NUM_ARGS: usize = {
                    let mut n = 0usize;
                    $(
                        $(
                            n += {
                                let _ = core::marker::PhantomData::<$types>;
                                1usize
                            };
                        )+
                    )?
                    n
                };
                const HAS_ARGS: bool = NUM_ARGS > 0;
                const OFFSET_TABLE_SZ: usize = {
                    let mut n = 2usize; // num_args + fmt offset
                    $(
                        $(
                            n += <$types as $crate::Trace>::NUM_LAY;
                        )+
                    )?
                    n += 3usize; // formatter + file + line
                    n
                };
                const STRING_SIZE: usize = {
                    let mut size = FMT.len() + 1 + file!().len() + 1;
                    $(
                        $(
                            size += <$types as $crate::Trace>::STR_SIZE;
                        )+
                    )?
                    size
                };
                const INFO_SIZE: usize =
                    $crate::record::PAYLOAD_OFF + OFFSET_TABLE_SZ * $crate::record::SZ + STRING_SIZE;
                const TOTAL_SIZE: usize = {
                    let mut total: usize = 0;
                    $(
                        $(
                            total += match <$types as $crate::Trace>::SIZE {
                                $crate::Size::Static(bytes) => bytes,
                                $crate::Size::LengthPrefixed(width) => width,
                                $crate::Size::NullTerminated(_) => 0,
                            };
                        )+
                    )?
                    total as usize
                };

                #[unsafe(link_section = ".emtrace")]
                $(
                    static _DUMMY: i32 = 0;
                    #[unsafe(link_section = $section)]
                )?
                    #[used]
                    static FMT_INFO: $crate::FormatInfo<INFO_SIZE> = {
                        let mut bytes = [0u8; INFO_SIZE];
                        $crate::record::write_frame(
                            &mut bytes,
                            b"TRCE",
                            $crate::TRACE_LAYOUT_VERSION.0,
                            $crate::TRACE_LAYOUT_VERSION.1,
                            INFO_SIZE as usize,
                        );
                        let mut lp = $crate::record::PAYLOAD_OFF;
                        let mut dp = lp + OFFSET_TABLE_SZ * $crate::record::SZ;
                        let fmt_pos = $crate::record::write_str(&mut bytes, &mut dp, FMT);
                        $crate::record::write_sz(&mut bytes, &mut lp, NUM_ARGS as usize);
                        $crate::record::write_sz(&mut bytes, &mut lp, fmt_pos as usize);
                        $(
                            $(
                                $crate::record::write_top_type::<$types>(&mut bytes, &mut lp, &mut dp);
                            )+
                        )?
                        let file_pos = $crate::record::write_str(&mut bytes, &mut dp, file!());
                        let formatter: usize =
                            if HAS_ARGS { $crate::EMT_PY_FORMAT } else { $crate::EMT_NO_FORMAT };
                        $(
                            let formatter = $formatter;
                        )?
                        $crate::record::write_sz(&mut bytes, &mut lp, formatter);
                        $crate::record::write_sz(&mut bytes, &mut lp, file_pos as usize);
                        $crate::record::write_sz(&mut bytes, &mut lp, line!() as usize);
                        $crate::FormatInfo { bytes }
                    };

                use $crate::Sink;
                #[allow(unused_mut)]
                let mut sink = $crate::sink!(
                    $(.sink_guard=$sink_guard,)? $(.sink=$sink,)?
                );
                let sink_ref = &mut sink;
                $(
                    let _dummy = sink_ref;
                    #[allow(unused_mut)]
                    let sink_ref = &mut $sink;
                )?
                let ptr_ser = &mut $crate::__ptr_serializer!($( .ptr_serializer = $ptr_serializer )?);
                let record_ptr = core::ptr::addr_of!(FMT_INFO) as *const u8;
                let result = sink_ref.begin(record_ptr, TOTAL_SIZE);
                if let Err(err) = result {
                    break 'trace Err(err);
                }
                let result = $crate::PtrSerializer::out_ptr(ptr_ser, record_ptr, sink_ref);
                if let Err(err) = result {
                    break 'trace Err(err);
                }
                $(
                    $(
                        let x: &$types = &$args;
                        let result = $crate::emit_args(x, sink_ref);
                        if let Err(err) = result {
                            break 'trace Err(err);
                        }
                    )+
                )?
                let result = sink_ref.finish();
                if let Err(err) = result {
                    break 'trace Err(err);
                }
                Ok(())
            };
            $(
                let result = $error_handler(result);
            )?
            result
        }
    }
}

/// Emit a trace.
///
/// The first argument is a format string, the rest are pairs of the form `type:expression`.
/// A trace is emitted - by default through the host pipeline (see [`DefaultSink`]) - recording
/// this trace point's `TRCE` record address, followed by the values of the other parameters.
///
/// ```rust
/// # use emtrace::{magic, trace};
/// # fn main() {
/// magic!();
/// trace!("Hello, {}", str: "world");
/// # }
/// ```
///
/// ### Error Handling
///
/// By default, this macro will panic if an error occurs during tracing. To handle
/// errors manually, use the `.no_panic` option. This will cause the macro to return a
/// `Result` instead of panicking, which you can then handle as needed.
///
/// For other customization options, see [`trace_impl!`].
///
/// [`trace_impl!`]: macro.trace_impl.html
#[macro_export]
macro_rules! trace{
    ($fmt:literal $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .sink_guard = $sink_guard:expr)? $(, .sink= $sink:expr)? $(, .ptr_serializer = $ptr_serializer:expr)? $(, .formatter=$formatter:expr)?)
     => {
        $crate::trace_impl!($fmt $(, $($types: $args),+)? $(, .section = $section)? $(, .sink_guard = $sink_guard)? $(, .sink= $sink)? $(, .ptr_serializer = $ptr_serializer)?, .error_handler=core::result::Result::unwrap $(, .formatter=$formatter)?)
    };
    ($fmt:literal $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .sink_guard = $sink_guard:expr)? $(, .sink= $sink:expr)? $(, .ptr_serializer = $ptr_serializer:expr)?, .no_panic $(, .formatter=$formatter:expr)?)
     => {
        $crate::trace_impl!($fmt $(, $($types: $args),+)? $(, .section = $section)? $(, .sink_guard = $sink_guard)? $(, .sink= $sink)? $(, .ptr_serializer = $ptr_serializer)? $(, .formatter=$formatter)?)
    };
}

/// Emit a trace with a trailing new line in the format string.
///
/// This macro is identical to [`trace!`], except it automatically appends a newline
/// character (`\n`) to the format string.
///
/// ```rust
/// # use emtrace::{magic, traceln};
/// # fn main() {
/// magic!();
/// traceln!("This will be followed by a newline.");
/// # }
/// ```
///
/// Like `trace!`, this macro panics on error by default; use `.no_panic` to receive a
/// `Result` instead. See [`trace!`] for the other customization options.
#[macro_export]
macro_rules! traceln {
    ($fmt:literal $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .sink_guard = $sink_guard:expr)? $(, .sink= $sink:expr)? $(, .ptr_serializer = $ptr_serializer:expr)? $(, .formatter=$formatter:expr)?)
     => {
        $crate::trace_impl!($fmt "\n" $(, $($types: $args),+)? $(, .section = $section)? $(, .sink_guard = $sink_guard)? $(, .sink= $sink)? $(, .ptr_serializer = $ptr_serializer)?, .error_handler=core::result::Result::unwrap $(, .formatter=$formatter)?)
    };
    ($fmt:literal $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .sink_guard = $sink_guard:expr)? $(, .sink= $sink:expr)? $(, .ptr_serializer = $ptr_serializer:expr)?, .no_panic $(, .formatter=$formatter:expr)?)
     => {
        $crate::trace_impl!($fmt "\n" $(, $($types: $args),+)? $(, .section = $section)? $(, .sink_guard = $sink_guard)? $(, .sink= $sink)? $(, .ptr_serializer = $ptr_serializer)? $(, .formatter=$formatter)?)
    };
}

#[macro_export]
macro_rules! expect {
    ($expected:literal, .section=$section:literal) => {
        {
            const LEN: usize = $expected.len();
            #[unsafe(link_section = $section)]
            #[unsafe(no_mangle)]
            static EXPECTED_OUTPUT: [u8; LEN] = {
                let mut bytes: [u8; LEN] = [0; LEN];

                let mut i = 0usize;
                while i < LEN {
                    bytes[i] = $expected.as_bytes()[i];
                    i += 1;
                }

                bytes
            };
            let _ = unsafe { std::ptr::read_volatile(&EXPECTED_OUTPUT) };
        }
    };
    ($expected:literal) => {
        expect!($expected, .section=".emt_exp")
    };
}

#[cfg(test)]
mod tests {
    use super::CobsEncoder;
    use std::sync::{LazyLock, Mutex};

    #[test]
    fn test() {
        let mut capture = Vec::new();
        trace!("{}", i32: 32, .sink=capture);
        assert_eq!(capture.len(), size_of::<usize>() + size_of::<i32>());
        let mut arg_bytes: [u8; size_of::<i32>()] = [0; size_of::<i32>()];
        arg_bytes
            .copy_from_slice(&capture[size_of::<usize>()..size_of::<usize>() + size_of::<i32>()]);
        let arg = i32::from_ne_bytes(arg_bytes);
        assert_eq!(arg, 32);
    }

    #[test]
    fn empty_vec() {
        let mut capture = Vec::new();
        let v = Vec::<i32>::new();
        trace!("{}", [i32]: v, .sink=capture);
        assert_eq!(capture.len(), size_of::<usize>() * 2);
        trace!("{}", Vec<i32>: v, .sink=capture);
        assert_eq!(capture.len(), size_of::<usize>() * 4);
    }

    #[test]
    fn flat_vec() {
        let mut capture1 = Vec::new();
        let mut capture2 = Vec::new();
        let v = vec![1, 3, 4, 6];

        let expected_len = 2 * size_of::<usize>() + v.len() * size_of::<i32>();
        trace!("{}", [i32]: v, .sink=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", Vec<i32>: v, .sink=capture2);
        assert_eq!(capture2.len(), expected_len);

        assert_eq!(
            capture1[size_of::<usize>()..],
            capture2[size_of::<usize>()..]
        );
        assert_ne!(
            capture1[..size_of::<usize>()],
            capture2[..size_of::<usize>()]
        );
    }

    #[test]
    fn nested_vecs() {
        let mut capture1 = Vec::new();
        let mut capture2 = Vec::new();
        let mut capture3 = Vec::new();
        let mut capture4 = Vec::new();
        let owning = vec![vec![1, 2, 3, 4], vec![10]];
        let referencing_vec = vec![&owning[0], &owning[1]];
        let referencing_slice = vec![&owning[0][..], &owning[1]];

        let expected_len = 2 * size_of::<usize>()
            + owning
                .iter()
                .map(|inner| inner.len() * size_of::<i32>() + size_of::<usize>())
                .sum::<usize>();
        trace!("{}", Vec<Vec<i32>>: owning, .sink=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", [Vec<i32>]: owning, .sink=capture2);
        assert_eq!(capture2.len(), expected_len);
        trace!("{}", [&Vec<i32>]: referencing_vec, .sink=capture3);
        assert_eq!(capture3.len(), expected_len);
        trace!("{}", [&[i32]]: referencing_slice, .sink=capture4);
        assert_eq!(capture4.len(), expected_len);

        assert_eq!(
            capture1[size_of::<usize>()..],
            capture2[size_of::<usize>()..]
        );
        assert_eq!(
            capture1[size_of::<usize>()..],
            capture3[size_of::<usize>()..]
        );
        assert_eq!(
            capture1[size_of::<usize>()..],
            capture4[size_of::<usize>()..]
        );
    }

    #[test]
    fn string() {
        use std::str::FromStr;

        let mut capture1 = Vec::new();
        let mut capture2 = Vec::new();
        let mut capture3 = Vec::new();
        let literal = "Hello, there!";
        let owning = String::from_str(literal).unwrap();

        let expected_len = size_of::<usize>() + literal.len() + size_of::<u8>();
        trace!("{}", str: literal, .sink=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", str: owning, .sink=capture2);
        assert_eq!(capture2.len(), expected_len);
        trace!("{}", String: owning, .sink=capture3);
        assert_eq!(capture3.len(), expected_len);

        assert_eq!(
            capture1[size_of::<usize>()..],
            capture2[size_of::<usize>()..]
        );
        assert_eq!(
            capture1[size_of::<usize>()..],
            capture3[size_of::<usize>()..]
        );
    }

    #[test]
    fn vec_of_string() {
        use std::str::FromStr;

        let mut capture1 = Vec::new();
        let mut capture2 = Vec::new();
        let slices = vec!["A", "vector", "of", "strings"];
        let owning: Vec<String> = slices
            .iter()
            .map(|s| String::from_str(s).unwrap())
            .collect();

        let expected_len = 2 * size_of::<usize>() // for pointer-, and length-prefix
            + slices
                .iter()
                .map(|inner| inner.len() + size_of::<u8>()) // for content and null-terminator
                .sum::<usize>();
        trace!("{}", Vec<&str>: slices, .sink=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", Vec<String>: owning, .sink=capture2);
        assert_eq!(capture2.len(), expected_len);

        assert_eq!(
            capture1[size_of::<usize>()..],
            capture2[size_of::<usize>()..]
        );
    }

    #[test]
    fn cobs_encode_short() {
        let mut capture = Vec::new();
        let data: Vec<u8> = vec![1, 2, 3, 0, 4, 5, 0, 0, 6];
        let mut cobs = CobsEncoder::new();
        cobs.encode(&mut capture, &data).unwrap();
        cobs.finalize(&mut capture).unwrap();

        assert_eq!(capture, vec![4, 1, 2, 3, 3, 4, 5, 1, 2, 6, 0]);
    }

    #[test]
    fn cobs_encode_long() {
        let mut capture = Vec::new();
        let data: Vec<u8> = vec![1; 256];

        let mut cobs = CobsEncoder::new();
        cobs.encode(&mut capture, &data).unwrap();
        cobs.finalize(&mut capture).unwrap();

        println!("{:x?}", capture);
        assert_eq!(capture.len(), 259);
        assert_eq!(capture[0], 0xFF);
        assert_eq!(capture[1..255], vec![1; 254][..]);
        assert_eq!(capture[255..], vec![3, 1, 1, 0][..]);
    }

    #[test]
    fn cobs_encode_254() {
        let mut capture = Vec::new();
        let data: Vec<u8> = vec![1; 254];

        let mut cobs = CobsEncoder::new();
        cobs.encode(&mut capture, &data).unwrap();
        cobs.finalize(&mut capture).unwrap();

        println!("{:x?}", capture);
        assert_eq!(capture.len(), 256);
        assert_eq!(capture[0], 0xFF);
        assert_eq!(capture[1..255], vec![1; 254]);
        assert_eq!(capture[255], 0);
    }

    #[test]
    fn cobs_encode_empty() {
        let mut capture = Vec::new();
        let data: Vec<u8> = vec![];
        let mut cobs = CobsEncoder::new();
        cobs.encode(&mut capture, &data).unwrap();
        cobs.finalize(&mut capture).unwrap();

        println!("{:x?}", capture);
        assert_eq!(capture, vec![1, 0]);
    }

    #[test]
    fn cobs_encode_only_zeros() {
        let mut capture = Vec::new();
        let data: Vec<u8> = vec![0; 4];
        let mut cobs = CobsEncoder::new();
        cobs.encode(&mut capture, &data).unwrap();
        cobs.finalize(&mut capture).unwrap();

        println!("{:x?}", capture);
        assert_eq!(capture, vec![1, 1, 1, 1, 1, 0]);
    }

    #[test]
    fn cobs_encoder_rearms_after_finalize() {
        // A hoisted (long-lived) encoder frames one packet per record: `finalize`
        // must reset the buffer position, or the second packet would carry the first
        // record's state.
        let mut capture = Vec::new();
        let mut cobs = CobsEncoder::new();
        cobs.encode(&mut capture, b"ab").unwrap();
        cobs.finalize(&mut capture).unwrap();
        cobs.encode(&mut capture, b"cd").unwrap();
        cobs.finalize(&mut capture).unwrap();

        println!("{:x?}", capture);
        assert_eq!(capture, vec![3, b'a', b'b', 0, 3, b'c', b'd', 0]);
    }

    #[test]
    fn shift_ptr_serializer() {
        use super::ShiftPtrSerializer;

        let mut capture = Vec::new();
        trace!("{}", i32: 32, .sink=capture, .ptr_serializer = ShiftPtrSerializer::new());
        // shifted record pointer + the i32 arg: same shape as the default pipeline.
        assert_eq!(capture.len(), size_of::<usize>() + size_of::<i32>());
        let ptr = usize::from_ne_bytes(capture[..size_of::<usize>()].try_into().unwrap());
        assert_ne!(ptr, 0);
    }

    #[test]
    fn shift_ptr_serializer_truncated() {
        use super::ShiftPtrSerializer;

        let mut capture = Vec::new();
        // explicit instantiation with a 2-byte pointer: little-endian keeps the low
        // bytes, big-endian the high bytes of the shifted address.
        trace!(
            "{}",
            i32: 32,
            .sink=capture,
            .ptr_serializer = ShiftPtrSerializer::<2, 8>
        );
        assert_eq!(capture.len(), size_of::<u16>() + size_of::<i32>());
        let arg = i32::from_ne_bytes(
            capture[size_of::<u16>()..size_of::<u16>() + size_of::<i32>()]
                .try_into()
                .unwrap(),
        );
        assert_eq!(arg, 32);
    }

    #[test]
    fn custom_ptr_serializer() {
        // A serializer emitting only the low 2 bytes of the shifted pointer.
        struct TwoBytePtrSerializer;

        impl super::PtrSerializer for TwoBytePtrSerializer {
            /// 2-byte pointer payload emitting, shifted by the crate alignment.
            const ALIGNMENT_POWER: u8 = super::ALIGNMENT_POWER;
            const PTR_WIDTH: usize = size_of::<u16>();

            fn out_ptr<S: super::Sink>(
                &mut self,
                record: *const u8,
                sink: &mut S,
            ) -> Result<(), S::OutError> {
                let ptr = ((record as usize) >> (super::ALIGNMENT_POWER as usize)) as u16;
                sink.out(&ptr.to_ne_bytes())
            }
        }

        let mut capture = Vec::new();
        trace!("{}", i32: 32, .sink=capture, .ptr_serializer = TwoBytePtrSerializer);
        assert_eq!(capture.len(), size_of::<u16>() + size_of::<i32>());
        let arg = i32::from_ne_bytes(
            capture[size_of::<u16>()..size_of::<u16>() + size_of::<i32>()]
                .try_into()
                .unwrap(),
        );
        assert_eq!(arg, 32);
    }

    #[test]
    fn ptr_serializer_consts_match_semantics() {
        assert_eq!(
            <super::DefaultPtrSerializer as super::PtrSerializer>::PTR_WIDTH,
            size_of::<usize>()
        );
        assert_eq!(
            <super::DefaultPtrSerializer as super::PtrSerializer>::ALIGNMENT_POWER,
            super::ALIGNMENT_POWER
        );
        // `ShiftPtrSerializer::<N_BYTES, SHIFT>` *is* its own advertised dimensions.
        assert_eq!(
            <super::ShiftPtrSerializer as super::PtrSerializer>::PTR_WIDTH,
            size_of::<usize>()
        );
        assert_eq!(
            <super::ShiftPtrSerializer as super::PtrSerializer>::ALIGNMENT_POWER,
            super::ALIGNMENT_POWER
        );
    }

    #[test]
    fn out_fn_sink_forwards_to_callback() {
        // A C-shaped out callback collecting bytes; the bridge sink forwards every
        // `out` call verbatim and never uses `begin`/`finish`.
        unsafe extern "C" fn collect(data: *const u8, size: usize, extra: *mut core::ffi::c_void) {
            let sink: *mut Vec<u8> = extra.cast();
            unsafe {
                (*sink).extend_from_slice(core::slice::from_raw_parts(data, size));
            }
        }

        let mut buffer: Vec<u8> = Vec::new();
        let mut bridge = unsafe {
            super::OutFnSink::new(
                collect,
                core::ptr::from_mut(&mut buffer).cast::<core::ffi::c_void>(),
            )
        };
        super::Sink::out(&mut bridge, b"hel").unwrap();
        super::Sink::out(&mut bridge, b"lo").unwrap();
        assert_eq!(buffer, b"hello");
    }

    #[test]
    fn write_magic_records_pointer_dimensions() {
        use super::record;

        let mut bytes = [0u8; record::MAGIC_SIZE];
        record::write_magic(&mut bytes, super::EMT_ENCODING_NONE, 2, 5);
        assert_eq!(bytes[record::PAYLOAD_OFF + 32], 2);
        assert_eq!(bytes[record::PAYLOAD_OFF + 33], 5);
        assert_eq!(bytes[record::PAYLOAD_OFF + 34], super::EMT_ENCODING_NONE);
        // the default path is unchanged
        let mut default_bytes = [0u8; record::MAGIC_SIZE];
        record::write_magic(
            &mut default_bytes,
            super::EMT_ENCODING_NONE,
            size_of::<usize>(),
            super::ALIGNMENT_POWER,
        );
        assert_eq!(
            default_bytes[record::PAYLOAD_OFF + 32],
            size_of::<usize>() as u8
        );
        assert_eq!(
            default_bytes[record::PAYLOAD_OFF + 33],
            super::ALIGNMENT_POWER
        );
    }

    #[test]
    fn ptr_serializer_is_borrowed_not_consumed() {
        let _guard = TEST_GUARD.lock().unwrap();
        let mut serializer = super::DefaultPtrSerializer;
        let start = CAPTURED.lock().unwrap().len();
        // Both calls borrow the same binding - no move-out error, and both records
        // go through the global (host) serializer.
        trace!("a {}", u8: 1, .ptr_serializer = serializer);
        trace!("c {}", u8: 2, .ptr_serializer = serializer);
        let captured_len = CAPTURED.lock().unwrap().len();
        assert!(captured_len > start);
    }

    #[test]
    fn single_byte_default_alignment() {
        use super::ALIGNMENT_POWER;

        assert_eq!(ALIGNMENT_POWER, 8);
    }

    #[cfg(feature = "minimum_alignment_512")]
    #[test]
    fn minimum_alignment_512_raised() {
        use super::ALIGNMENT_POWER;

        assert!(ALIGNMENT_POWER >= 9);
    }

    #[cfg(feature = "default_alignment_512")]
    #[test]
    fn default_alignment_512_selected() {
        use super::ALIGNMENT_POWER;

        assert!(ALIGNMENT_POWER >= 9);
    }

    // Global capture for the default sink pipeline: the `magic!` symbols wrap *this*
    // sink, so every default-sink trace in this test binary lands here. The wrapped
    // sink expression is evaluated per record inside the generated
    // `emt_default_begin`, so - like in C - it can only reach global state, not
    // locals. The encoding id is derived from the sink type's `Sink::ENCODING_ID`
    // (the trait default, `EMT_ENCODING_NONE`, here).
    static CAPTURED: LazyLock<Mutex<Vec<u8>>> = LazyLock::new(|| Mutex::new(Vec::new()));

    // The harness runs tests concurrently, and this shared stream is mutated by every
    // default-pipeline test, so such tests take this guard around their windows.
    static TEST_GUARD: LazyLock<Mutex<()>> = LazyLock::new(|| Mutex::new(()));

    // The MGIC record and the `emt_default_*` symbol implementations of this test
    // binary: exactly one host, exactly like a real binary's host. `.section` also
    // rides along here (combined with `.sink_guard`), placing the MGIC record in
    // `.emtsec` - the end-to-end counterpart (parser `--section-name` roundtrip)
    // lives in tests/test_emtrace.py::test_custom_section.
    crate::magic!(
        .sink_guard: std::sync::MutexGuard<'static, Vec<u8>> = CAPTURED.lock().unwrap(),
        .section = ".emtsec"
    );

    #[test]
    fn default_sink_extern_pipeline() {
        let _guard = TEST_GUARD.lock().unwrap();
        // Other default-pipeline tests may already have appended records, so assert
        // on this test's own window of the shared stream.
        let start = CAPTURED.lock().unwrap().len();

        trace!("{} + {}", i32: 1, i32: 2);

        // Runtime stream through symbols + wrapped sink:
        // pointer (shifted record address, `usize` bytes) + the two `i32` args.
        let buf = CAPTURED.lock().unwrap().clone();
        let window = &buf[start..];
        assert!(window.len() >= size_of::<usize>());
        let ptr_part = &window[..size_of::<usize>()];
        let ptr = usize::from_ne_bytes(ptr_part.try_into().unwrap());
        assert_ne!(ptr, 0);
        let args = &window[size_of::<usize>()..];
        let want = [1i32.to_ne_bytes(), 2i32.to_ne_bytes()].concat();
        assert_eq!(args, want.as_slice());
    }

    #[test]
    fn multithreaded_default_pipeline() {
        let _guard = TEST_GUARD.lock().unwrap();
        let start = CAPTURED.lock().unwrap().len();

        // Traces from multiple threads simultaneously: whole-record serialization -
        // the magic!-wrapped `MutexGuard` spans `emt_default_begin` through
        // `emt_default_finish`, like the C implementation's whole-record
        // `flockfile`/`funlockfile` span - must prevent the records from
        // interleaving.
        const THREADS: usize = 4;
        const RECORDS: usize = 10;
        let handles: Vec<_> = (0..THREADS)
            .map(|thread| {
                std::thread::spawn(move || {
                    for i in 0..RECORDS {
                        trace!("{}", i32: (thread * RECORDS + i) as i32);
                    }
                })
            })
            .collect();
        for handle in handles {
            handle.join().unwrap();
        }

        // Every record must have arrived intact: independent record pointers, every
        // argument exactly once.
        const RECORD_LEN: usize = size_of::<usize>() + size_of::<i32>();
        let buf = CAPTURED.lock().unwrap();
        let window = &buf[start..];
        assert_eq!(window.len(), THREADS * RECORDS * RECORD_LEN);
        let mut args = Vec::new();
        for record in window.chunks(RECORD_LEN) {
            let ptr = usize::from_ne_bytes(record[..size_of::<usize>()].try_into().unwrap());
            assert_ne!(ptr, 0);
            args.push(i32::from_ne_bytes(
                record[size_of::<usize>()..].try_into().unwrap(),
            ));
        }
        args.sort_unstable();
        let expected: Vec<i32> = (0..(THREADS * RECORDS) as i32).collect();
        assert_eq!(args, expected);
    }
}
