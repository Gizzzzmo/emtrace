use emtrace::{Error, Sink, trace};

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
    type BeginError = OverflowError;
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
        _info_addr: emtrace::PointerT,
        total_size: emtrace::SizeT,
    ) -> core::result::Result<(), Self::BeginError> {
        if total_size as usize > CAPACITY - self.size {
            return Err(OverflowError {});
        }
        Ok(())
    }
}

fn show_result(result: core::result::Result<(), Error<OverflowError, OverflowError>>) {
    match result {
        // This branch is matched if the call to `begin` succeeded, but a later call to `out` failed.
        Err(Error::Out(OverflowError)) => {
            println!("An overflow error occurred during serialization.");
        }
        // This branch is matched if the call to `begin` failed, no to calls to `out` were performed.
        Err(Error::Begin(OverflowError)) => {
            println!("An overflow error occurred while calling `begin`.");
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

    // Specifying `.handle_errors` means the macro invocation returns a
    // `Result<(), Result<Sink::OutError, Sink::BeginError>>`
    let result = trace!(
        "Just a static string. This call produces a single pointer, which will fit into our empty static vector.",
        .sink=vec,
        .handle_errors
    );
    show_result(result);
    println!("Vector size: {}", vec.len());

    let result = trace!(
        "Lots of data, where we can tell that it won't fit before starting to serialize: {}.",
        [u8; CAPACITY]: [0; CAPACITY],
        .sink=vec,
        .handle_errors
    );
    show_result(result);
    println!("Vector size: {}", vec.len());

    let result = trace!(
        "{}",
        str: "A dynamic string that is too long to be stored in our static vector.",
        .sink=vec,
        .handle_errors
    );
    show_result(result);
    println!("Vector size: {}", vec.len());

    // The vector is full now, so this will still fail, and panic because we didn't specify `.handle_errors`
    trace!("Panic!", .sink=vec);
}
