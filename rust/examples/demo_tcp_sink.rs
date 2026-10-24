//! Demonstrates a custom program-lifetime sink backed by a TCP connection:
//!
//! 1. define a local type owning the connection (`TcpSink` wraps the `TcpStream` in
//!    a `Mutex`),
//! 2. implement [`Sink`] for it,
//! 3. store it in a global `OnceLock` after connecting in `main`,
//! 4. hand `magic!` the per-record *access expression*.
//!
//! To keep the example self-contained, the "server" is a local thread in this same
//! process. Run with `cargo run --example demo_tcp_sink`.

use emtrace::{Sink, traceln};
use std::io::Read;
use std::net::{Shutdown, TcpListener, TcpStream};
use std::sync::{Mutex, MutexGuard, OnceLock};

// (1) The long-lived sink: local type, owns the TCP connection established at
// program start. The outer `Mutex` provides the per-record lock span.
#[derive(Debug)]
struct TcpSink(TcpStream);

// The crate's blanket `impl<T: Sink> Sink for MutexGuard<'_, T>` delegates to this
// impl while the connection is locked for the record span.
impl Sink for TcpSink {
    type OutError = std::io::Error;
    fn out(&mut self, b: &[u8]) -> Result<(), Self::OutError> {
        std::io::Write::write_all(&mut self.0, b)
    }
    fn finish(&mut self) -> Result<(), Self::OutError> {
        std::io::Write::flush(&mut self.0)
    }
}

// (3) Global storage for the connection.
static CONNECTION: OnceLock<Mutex<TcpSink>> = OnceLock::new();

// The MGIC record plus the `emt_default_*` symbol implementations: the binary's
// host. The `.sink_guard` expression is evaluated once per record: it locks the
// connection's `Mutex`, and the guard spans the whole record.
emtrace::magic!(
    .sink_guard: MutexGuard<'static, TcpSink> = CONNECTION
        .get()
        .expect("the connection is established in main before any trace")
        .lock()
        .unwrap()
);

fn main() {
    // A tiny in-process "server" so the example needs no external listener.
    let listener = TcpListener::bind("127.0.0.1:0").expect("bind");
    let server_addr = listener.local_addr().expect("local addr");

    // The server thread drains the raw runtime stream that the records produce.
    let server = std::thread::spawn(move || {
        let (mut stream, _) = listener.accept().expect("accept");
        let mut received = Vec::new();
        let _ = stream.read_to_end(&mut received);
        println!(
            "server received {} bytes of runtime trace stream",
            received.len()
        );
    });

    // (4) Connect at program start and hand the connection to the global sink.
    let stream = TcpStream::connect(server_addr).expect("connect");
    CONNECTION
        .set(Mutex::new(TcpSink(stream)))
        .expect("connected at most once");

    traceln!("Hello {}", i32: 1);
    traceln!("Hello over TCP {} {}", &str: "world", i32: 2);

    // Signal end of stream so the server thread's read completes and the example
    // terminates deterministically.
    match CONNECTION
        .get()
        .expect("connected")
        .lock()
        .expect("connection mutex")
        .0
        .shutdown(Shutdown::Write)
    {
        Ok(()) => {}
        Err(e) => eprintln!("shutdown failed: {e}"),
    }
    let _ = server.join();
}
