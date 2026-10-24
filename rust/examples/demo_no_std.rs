#![no_std]
use core::convert::Infallible;
use emtrace::{Sink, magic, trace};

// Record-only host: a long-lived global sink isn't available in `no_std` (the thread-
// local slot is std-only), so `magic!` runs in its pure `EMT_MAGIC` configuration:
// only the MGIC record is emitted, with no `emt_default_*` host functions at all.
// Because no global pointer serializer exists, `.ptr_width`/`.alignment_power` could
// independently set the record's advertised pointer dimensions; the defaults
// (`size_of::<usize>()`, this crate's `ALIGNMENT_POWER`) match the
// `ShiftPtrSerializer::new()` used below, so they are left out here.
//
// With no global sink, every `trace!` must supply its own `.sink`/`.sink_guard` - a
// default-sink trace would fail to link (`emt_default_begin` & co. undefined), exactly
// like a C `EMT_MAGIC`-only binary whose records are driven through user-provided
// output code.
magic!(.no_global_sink, .no_global_ptr_serializer);

const CAPACITY: usize = 1024;
struct StaticVec {
    buffer: [u8; CAPACITY],
    size: usize,
}

impl StaticVec {
    pub fn new() -> StaticVec {
        StaticVec {
            buffer: [0; CAPACITY],
            size: 0,
        }
    }
}

impl Sink for StaticVec {
    type OutError = Infallible;

    fn out(&mut self, b: &[u8]) -> core::result::Result<(), Self::OutError> {
        self.buffer[self.size..self.size + b.len()].copy_from_slice(b);
        self.size += b.len();
        Ok(())
    }
}

/// This demo tests that the crate works in a [no_std] environment
/// The default sink used by the `trace!` macro requires std, to print to stdout.
/// We instead provide as an output sink a static vector with limited capacity, and
/// the local-shift pointer serializer, since - with `.no_global_ptr_serializer` -
/// no host-defined `emt_default_serialize_pointer` exists to delegate to.
///
/// Both option values are *borrowed* per call, so a single `StaticVec`/serializer can
/// serve every `trace!` statement; no per-call construction is needed.
fn main() {
    let mut vec = StaticVec::new();
    let mut serializer = emtrace::ShiftPtrSerializer::new();
    trace!("Blub", .sink = vec, .ptr_serializer = serializer);
    trace!("Blub", .sink_guard = vec, .ptr_serializer = serializer);
}
