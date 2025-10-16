use emtrace::{Sink, init, traceln};
use std::io::Write;
use std::thread;

struct FileSink {
    pub file: std::io::BufWriter<std::fs::File>,
}

impl FileSink {
    fn new(file: std::fs::File) -> Self {
        Self {
            file: std::io::BufWriter::new(file),
        }
    }
}

impl Sink for FileSink {
    type OutError = std::io::Error;
    type BeginError = std::convert::Infallible;
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        use std::io::Write;
        self.file.write_all(b)
    }

    fn begin(
        &mut self,
        _info_addr: emtrace::PointerT,
        _total_size: emtrace::SizeT,
    ) -> Result<(), Self::BeginError> {
        Ok(())
    }
}

fn main() {
    let file = std::fs::File::create("trace.log").unwrap();
    let mut sink = FileSink::new(file);

    init(&mut sink).unwrap();

    let mut handles = vec![];

    for i in 0u8..8 {
        let i = i.clone();
        let handle = thread::spawn(move || {
            let now = std::time::Instant::now();
            let filename = format!("thread_{}.log", i);
            let file = std::fs::File::create(filename).unwrap();
            let mut sink = FileSink::new(file);

            for j in 0..128 {
                traceln!("t = {}: Thread {:d} start. {}", f32: now.elapsed().as_millis() as f32 / 1000.0,  u8: i, i32: j, .sink=sink);
                thread::sleep(std::time::Duration::from_millis(10));
            }
            sink.file.flush().unwrap();
        });

        handles.push(handle);
    }

    for handle in handles {
        handle.join().unwrap();
    }
}
