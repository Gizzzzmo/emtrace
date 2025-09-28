#![no_std]
use core::convert::Infallible;
use emtrace::{Sink, trace};
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
    type BeginError = Infallible;
    fn out(&mut self, b: &[u8]) -> core::result::Result<(), Self::OutError> {
        self.buffer[self.size..self.size + b.len()].copy_from_slice(b);
        self.size += b.len();

        Ok(())
    }
    fn begin(
        &mut self,
        _info_addr: emtrace::PointerT,
        _total_size: emtrace::SizeT,
    ) -> core::result::Result<(), Self::BeginError> {
        Ok(())
    }
}

/// This demo tests that the crate works in a [no_std] environment
/// The default sink used by the `trace!` macro requires std, to print to stdout.
/// We instead provide as an output sink a static vector with limited capacity.
fn main() {
    let mut vec = StaticVec::new();
    trace!("Blub", .sink=vec);
    trace!("Blub", .sink_guard=vec);
}
