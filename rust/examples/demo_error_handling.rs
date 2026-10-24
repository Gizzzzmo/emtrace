use emtrace::{Sink, trace};

const CAPACITY: usize = 32;
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
    pub fn len(&self) -> usize {
        self.size
    }
}

#[derive(Debug)]
struct OverflowError;

impl Sink for StaticVec {
    type OutError = OverflowError;
    fn out(&mut self, b: &[u8]) -> core::result::Result<(), Self::OutError> {
        if b.len() > CAPACITY - self.size {
            return Err(OverflowError {});
        }
        self.buffer[self.size..self.size + b.len()].copy_from_slice(b);
        self.size += b.len();

        Ok(())
    }
    fn begin(
        &mut self,
        _info_addr: *const u8,
        total_size: usize,
    ) -> core::result::Result<(), Self::OutError> {
        if total_size > CAPACITY - self.size {
            return Err(OverflowError {});
        }
        Ok(())
    }
}

fn show_result(result: core::result::Result<(), OverflowError>) {
    match result {
        // This branch is matched if an error occurred while tracing.
        Err(OverflowError) => {
            println!("An overflow error occurred while tracing.");
        }
        // This branch is matched if everything went well
        Ok(()) => {
            println!("Success!");
        }
    };
}

fn main() {
    // this static vector only has space for 32 bytes
    let mut vec = StaticVec::new();

    // Specifying `.no_panic` means the macro invocation returns a
    // `Result<(), Sink::OutError>`
    let result = trace!(
        "Just a static string. This call produces a single pointer, which will fit into our empty static vector.",
        .sink=vec,
        .ptr_serializer = emtrace::ShiftPtrSerializer::new(),
        .no_panic
    );
    show_result(result);
    println!("Vector size: {}", vec.len());

    let result = trace!(
        "Lots of data, where we can tell that it won't fit before starting to serialize: {}.",
        [u8; CAPACITY]: [0; CAPACITY],
        .sink=vec,
        .ptr_serializer = emtrace::ShiftPtrSerializer::new(),
        .no_panic
    );
    show_result(result);
    println!("Vector size: {}", vec.len());

    let result = trace!(
        "{}",
        str: "A dynamic string that is too long to be stored in our static vector.",
        .sink=vec,
        .ptr_serializer = emtrace::ShiftPtrSerializer::new(),
        .no_panic
    );
    show_result(result);
    println!("Vector size: {}", vec.len());

    // The vector is full now, so this will still fail, and panic because we didn't specify
    // `.no_panic`
    trace!("Panic!", .sink=vec, .ptr_serializer = emtrace::ShiftPtrSerializer::new());
}
