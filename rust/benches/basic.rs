use criterion::{Criterion, criterion_group, criterion_main};
use emtrace::trace;

pub fn benchmark(c: &mut Criterion) {
    c.bench_function("trace", |b| {
        let mut v = Vec::new();
        b.iter(|| {
            trace!("{}", .sink = v, .ptr_serializer = emtrace::ShiftPtrSerializer::new());
        })
    });
}

criterion_group!(benches, benchmark);
criterion_main!(benches);
