use std::io::StdoutLock;
use std::io::Write;
use std::mem::size_of;
use std::ops::DerefMut;
use std::sync::MutexGuard;

pub union Transmute<T: Copy, U: Copy> {
    pub from: T,
    pub to: U,
}

pub const NULL_TERMINATED: usize = 1usize << (usize::BITS - 1);
pub const LENGTH_PREFIXED: usize = 1usize << (usize::BITS - 2);
pub const PY_FORMAT: usize = 0;
pub const NO_FORMAT: usize = 1;
pub const C_STYLE_FORMAT: usize = 2;

/// Trait for emtrace output gadgets.
pub trait Out {
    fn out(&mut self, b: &[u8]);
    /// Begin emitting a trace.
    ///
    /// This function is called by the `emtrace::trace` macro just before starting to emit a trace.
    /// It will provide the address of the format info bytes as `info_addr`,
    /// and the total number of bytes that the trace will contain as `total_size`.
    ///
    /// If the trace contains dynamically sized data, then `total_size` is provided as
    /// the minimal number of bytes that will be sent, bitwise ored with either `emtrace::NULL_TERMINATED` or
    /// `emtrace::LENGTH_PREFIXED`, depending on the kind of dynamic data.
    fn begin(&mut self, info_addr: usize, total_size: usize);
}

impl<T: Out> Out for MutexGuard<'_, T> {
    fn out(&mut self, b: &[u8]) {
        self.deref_mut().out(b)
    }
    fn begin(&mut self, info_addr: usize, total_size: usize) {
        self.deref_mut().begin(info_addr, total_size)
    }
}

impl Out for StdoutLock<'_> {
    fn out(&mut self, b: &[u8]) {
        self.write_all(b).unwrap();
    }
    fn begin(&mut self, _info_addr: usize, _total_size: usize) {}
}

impl Out for Vec<u8> {
    fn out(&mut self, b: &[u8]) {
        self.extend_from_slice(b);
    }
    fn begin(&mut self, _info_addr: usize, total_size: usize) {
        let total_size = total_size & !(NULL_TERMINATED | LENGTH_PREFIXED);
        self.reserve(total_size);
    }
}

pub trait Trace {
    const SIZE: usize;
    const ID: &'static str;
    // fn serialize<Out>(&self, f: Out) where
    //     Out: Fn(&[u8; Self::SIZE]) -> ();
    fn serialize<O: Out>(&self, f: &mut O);
    fn size(&self) -> usize;
}

impl<T> Trace for &T
where
    T: Trace,
    T: ?Sized,
{
    const SIZE: usize = T::SIZE;
    const ID: &'static str = T::ID;
    fn serialize<O: Out>(&self, f: &mut O) {
        (*self).serialize(f)
    }
    fn size(&self) -> usize {
        (*self).size()
    }
}

impl<T: Trace> Trace for Box<T> {
    const SIZE: usize = T::SIZE;
    const ID: &'static str = T::ID;
    fn serialize<O: Out>(&self, f: &mut O) {
        self.as_ref().serialize(f)
    }
    fn size(&self) -> usize {
        self.as_ref().size()
    }
}

impl Trace for str {
    const SIZE: usize = NULL_TERMINATED;
    const ID: &'static str = "string";
    fn serialize<O: Out>(&self, f: &mut O) {
        f.out(self.as_bytes());
    }
    fn size(&self) -> usize {
        self.len()
    }
}

impl Trace for String {
    const SIZE: usize = str::SIZE;
    const ID: &'static str = str::ID;
    fn serialize<O: Out>(&self, f: &mut O) {
        self.as_str().serialize(f)
    }
    fn size(&self) -> usize {
        self.as_str().size()
    }
}

impl<T: Trace> Trace for [T] {
    const SIZE: usize = LENGTH_PREFIXED;
    const ID: &'static str = T::ID;
    fn serialize<O: Out>(&self, f: &mut O) {
        for el in self {
            if T::SIZE & LENGTH_PREFIXED != 0 {
                el.size().serialize(f);
            }
            el.serialize(f);
            if T::SIZE & NULL_TERMINATED != 0 {
                el.size().serialize(f);
            }
        }
    }
    fn size(&self) -> usize {
        let mut data_size = self.iter().map(|t| t.size()).sum();

        if T::SIZE & LENGTH_PREFIXED != 0 {
            data_size += self.len() * usize::SIZE;
        }
        if T::SIZE & NULL_TERMINATED != 0 {
            data_size += self.len() * u8::SIZE;
        }
        data_size
    }
}

impl<T: Trace> Trace for Vec<T> {
    const SIZE: usize = <[T] as Trace>::SIZE;
    const ID: &'static str = <[T] as Trace>::ID;
    fn serialize<O: Out>(&self, f: &mut O) {
        self[..].serialize(f)
    }
    fn size(&self) -> usize {
        self[..].size()
    }
}

impl<T: Trace, const N: usize> Trace for [T; N] {
    const SIZE: usize = {
        let t_size = !(LENGTH_PREFIXED | NULL_TERMINATED) & T::SIZE;
        let total_size = N * t_size;
        let flags = (LENGTH_PREFIXED | NULL_TERMINATED) & T::SIZE;
        total_size & flags
    };
    const ID: &'static str = T::ID;

    fn serialize<O: Out>(&self, f: &mut O) {
        for el in self {
            if T::SIZE & LENGTH_PREFIXED != 0 {
                el.size().serialize(f);
            }
            el.serialize(f);
            if T::SIZE & NULL_TERMINATED != 0 {
                el.size().serialize(f);
            }
        }
    }

    fn size(&self) -> usize {
        let mut data_size = self.iter().map(|el| el.size()).sum();

        if T::SIZE & LENGTH_PREFIXED != 0 {
            data_size += N * usize::SIZE;
        }
        if T::SIZE & NULL_TERMINATED != 0 {
            data_size += N * u8::SIZE;
        }
        data_size
    }
}

macro_rules! impl_trace_for_primitive {
    ($type:ty, $id:expr) => {
        impl Trace for $type {
            const SIZE: usize = std::mem::size_of::<Self>();
            const ID: &'static str = $id;

            fn serialize<O: Out>(&self, f: &mut O) {
                f.out(&self.to_ne_bytes());
            }

            fn size(&self) -> usize {
                std::mem::size_of::<Self>()
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
    const SIZE: usize = std::mem::size_of::<u8>();
    const ID: &'static str = "bool";

    fn serialize<O: Out>(&self, f: &mut O) {
        f.out(&[*self as u8]);
    }

    fn size(&self) -> usize {
        std::mem::size_of::<u8>()
    }
}

#[macro_export]
macro_rules! count {
    () => (0usize);
    ( $x:tt $($xs:tt)* ) => (1usize + $crate::count!($($xs)*));
}

/// Emit a trace.
/// The format string is the concatenation of all string literals before the first comma.
/// After that come the values to be emitted, along with their types, e.g.:
/// ```
/// use emtrace::{trace, Out};
/// trace!("{} + {}" " = {}", i32: 1, i32: 2, i32: 3);
/// // 1 + 2 = 3
/// ```
/// A type can be made traceable by implementing the Trace trait for it.
///
/// The format string along with other information on how many bytes are emitted by this trace
/// will be in the executable's .emtrace section unless
/// specified otherwise via `.section = "<section name>"` after the passed values.
///
/// The trace will be emitted sequentially, and uninterrupted by outputs from other threads on stdout
/// unless otherwise specified via `.out = <expr>` after the passed values.
/// TODO: explain out-channel
/// ```
#[macro_export]
macro_rules! trace {
    ($($fmt:literal)+ $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .consume_out = $consume_out:expr)? $(, .out = $out:expr)? $(, .formatter=$formatter:expr)?)
     => {
        {
            const FMT: &str = std::concat!($($fmt,)+);
            const NUM_PREFIX: usize = 5;
            const INFO_SIZE: usize = {
                let mut size: usize = 0;
                ($($(
                    {
                        const N: usize = <$types as $crate::Trace>::ID.as_bytes().len();
                        size += N + 1 + size_of::<usize>() * 2;
                    }
                ),*)?);
                size += NUM_PREFIX * size_of::<usize>() + FMT.as_bytes().len() + file!().as_bytes().len() + 2;
                size
            };
            #[unsafe(link_section = ".emtrace")]
            $(
                static _DUMMY: i32 = 0;
                #[unsafe(link_section = $section)]
            )?
            #[used]
            static FMT_INFO: [u8; INFO_SIZE] = unsafe {
                let usize_size = size_of::<usize>();
                let num_args: usize = $crate::count!($($($types)*)?);
                let mut offset_idx: usize = 0;
                let mut data_idx: usize = (NUM_PREFIX + num_args * 2) * usize_size;
                let mut info  = [0; INFO_SIZE];

                let num_args_arr = num_args.to_ne_bytes();
                let mut i = 0;
                while i < usize_size {
                    info[offset_idx] = num_args_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                let fmt_offset_arr = data_idx.to_ne_bytes();
                let mut i = 0;
                while i < usize_size {
                    info[offset_idx] = fmt_offset_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                const N: usize = FMT.as_bytes().len();
                let fmt_str_arr = *$crate::Transmute::<*const [u8; N], &[u8; N]> {
                    from: FMT.as_ptr() as *const [u8; N],
                }.to;
                let mut i = 0;
                while i < N {
                    info[data_idx] = fmt_str_arr[i];
                    data_idx += 1;
                    i += 1;
                }
                data_idx += 1;

                ($($(
                    {
                        let id_offset_arr = data_idx.to_ne_bytes();
                        let mut i = 0;
                        while i < usize_size {
                            info[offset_idx] = id_offset_arr[i];
                            offset_idx += 1;
                            i += 1;
                        }

                        let size_arr = <$types as $crate::Trace>::SIZE.to_ne_bytes();
                        let mut i = 0;
                        while i < usize_size {
                            info[offset_idx] = size_arr[i];
                            offset_idx += 1;
                            i += 1;
                        }

                        const N: usize = <$types as $crate::Trace>::ID.as_bytes().len();
                        let id_arr = *$crate::Transmute::<*const [u8; N], &[u8; N]> {
                            from: <$types as $crate::Trace>::ID.as_ptr() as *const [u8; N],
                        }.to;
                        let mut i = 0;
                        while i < N {
                            info[data_idx] = id_arr[i];
                            data_idx += 1;
                            i += 1;
                        }
                        data_idx += 1;
                    }
                ),*)?);

                let formatter = $crate::NO_FORMAT;
                // if there is at least one `,type: arg` pair in the macro arguments,
                // then this will overwrite the default NO_FORMAT formatter to PY_FORMAT
                $($(
                    let _dummy = formatter;
                    let formatter = <$types as $crate::Trace>::SIZE - <$types as $crate::Trace>::SIZE + $crate::PY_FORMAT;
                )*)?
                $(
                    let _dummy = formatter;
                    let formatter = $formatter;
                )?
                let formatter_arr = (formatter as usize).to_ne_bytes();
                let mut i = 0;
                while i < usize_size {
                    info[offset_idx] = formatter_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                let file_offset_arr = data_idx.to_ne_bytes();
                let mut i = 0;
                while i < usize_size {
                    info[offset_idx] = file_offset_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                const M: usize = file!().as_bytes().len();
                let file_arr = *$crate::Transmute::<*const [u8; M], &[u8; M]> {
                    from: file!().as_ptr() as *const [u8; M],
                }.to;
                let mut i = 0;
                while i < M {
                    info[data_idx] = file_arr[i];
                    data_idx += 1;
                    i += 1;
                }

                let line_arr = (line!() as usize).to_ne_bytes();
                let mut i = 0;
                while i < usize_size {
                    info[offset_idx] = line_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                info

            };
            const TOTAL_SIZE: usize = {
                let mut total_size = 0usize;
                $($(
                    let size:usize = <$types as $crate::Trace>::SIZE;
                    let mut mask = !0usize;
                    if (size & $crate::LENGTH_PREFIXED) != 0 {
                        total_size |= $crate::LENGTH_PREFIXED;
                        mask &= !$crate::LENGTH_PREFIXED;
                        total_size += size_of::<usize>();
                    }
                    if (size & $crate::NULL_TERMINATED) != 0 {
                        total_size |= $crate::NULL_TERMINATED;
                        mask &= !$crate::NULL_TERMINATED;
                        total_size += 1;
                    }

                    total_size += mask & size;
                )*)?
                total_size += size_of::<usize>();

                total_size
            };

            #[allow(unused_mut)]
            let mut gadget = std::io::stdout().lock();
            $(
                let _dummy = gadget;
                #[allow(unused_mut)]
                let mut gadget = $consume_out;
            )?
            let gadget_ref = &mut gadget;
            $(
                let _dummy = gadget_ref;
                #[allow(unused_mut)]
                let gadget_ref = &mut $out;
            )?
            let addr = FMT_INFO.as_ptr().addr();
            gadget_ref.begin(addr, TOTAL_SIZE);
            $crate::Trace::serialize(&addr, gadget_ref);
            $($(
                let x:&$types = &$args;
                if (<$types as $crate::Trace>::SIZE & $crate::LENGTH_PREFIXED) != 0 {
                    $crate::Trace::serialize(&$crate::Trace::size(x), gadget_ref);
                }
                $crate::Trace::serialize(x, gadget_ref);
                if (<$types as $crate::Trace>::SIZE & $crate::NULL_TERMINATED) != 0 {
                    let null = 0u8;
                    $crate::Trace::serialize(&null, gadget_ref);
                }
            )*)?
        }
    };
}

#[macro_export]
macro_rules! traceln {
    ($fmt:literal $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .out = $out:expr)?$(, .formatter=$formatter:expr)?)
     => {
        $crate::trace!($fmt "\n" $(, $($types: $args),+)? $(, .section = $section)? $(, .out = $out)? $(, .formatter=$formatter)?)
    };
}

#[macro_export]
macro_rules! expect {
    ($expected:literal, .section=$section:literal) => {
        {
            const LEN: usize = $expected.len();
            #[unsafe(link_section = $section)]
            #[unsafe(no_mangle)]
            static EXPECTED_OUTPUT: [u8; LEN] = *$expected;
            let _ = unsafe { std::ptr::read_volatile(&EXPECTED_OUTPUT) };
        }
    };
    ($expected:literal) => {
        expect!($expected, .section=".emtrace.test.expected")
    };
}

const MAGIC_SIZE: usize = 35 + 3 * size_of::<usize>();
#[unsafe(link_section = ".emtrace")]
#[used]
pub static EMTRACE_MAGIC: [u8; MAGIC_SIZE] = {
    let id = [
        0xd1, 0x97, 0xf5, 0x22, 0xd9, 0x26, 0x9f, 0xd1, 0xad, 0x70, 0x33, 0x92, 0xf6, 0x59, 0xdf,
        0xd0, 0xfb, 0xec, 0xbd, 0x60, 0x97, 0x13, 0x25, 0xe8, 0x92, 0x01, 0xb2, 0x5a, 0x38, 0x5d,
        0x9e, 0xc7,
    ];

    let mut magic = [0; MAGIC_SIZE];
    let mut i = 0;
    while i < 32 {
        magic[i] = id[i];
        i += 1;
    }
    magic[32] = 35;
    magic[33] = size_of::<usize>() as u8;
    magic[34] = size_of::<usize>() as u8;

    let mut idx = 35;

    let byte_order_id_arr = 0x0706050403020100usize.to_ne_bytes();
    let mut i = 0;
    while i < size_of::<usize>() {
        magic[idx] = byte_order_id_arr[i];
        idx += 1;
        i += 1;
    }

    let null_terminated_arr = NULL_TERMINATED.to_ne_bytes();
    let mut i = 0;
    while i < size_of::<usize>() {
        magic[idx] = null_terminated_arr[i];
        idx += 1;
        i += 1;
    }

    let length_prefixed_arr = LENGTH_PREFIXED.to_ne_bytes();
    let mut i = 0;
    while i < size_of::<usize>() {
        magic[idx] = length_prefixed_arr[i];
        idx += 1;
        i += 1;
    }
    magic
};

pub fn magic_address_bytes() -> [u8; size_of::<usize>()] {
    (&EMTRACE_MAGIC as *const [u8; MAGIC_SIZE])
        .addr()
        .to_ne_bytes()
}

#[cfg(test)]
mod tests {
    use super::{Out, Trace, trace};
    use std::str::FromStr;

    #[test]
    fn test() {
        let mut buffer = Vec::<u8>::new();
        trace!("{}", i32: 32, .out=buffer);
        assert_eq!(buffer.len(), usize::SIZE + i32::SIZE);
        let mut arg_bytes: [u8; i32::SIZE] = [0; i32::SIZE];
        arg_bytes.copy_from_slice(&buffer[usize::SIZE..usize::SIZE + i32::SIZE]);
        let arg = i32::from_ne_bytes(arg_bytes);
        assert_eq!(arg, 32);

        // let vv = vec![vec![1, 2, 3], vec![4, 5, 6]];
        // let vvv: Vec<&[i32]> = vec![&v, &vv[0]];
    }

    #[test]
    fn empty_vec() {
        let mut capture = Vec::<u8>::new();
        let v = Vec::<i32>::new();
        trace!("{}", [i32]: v, .out=capture);
        assert_eq!(capture.len(), usize::SIZE * 2);
        trace!("{}", Vec<i32>: v, .out=capture);
        assert_eq!(capture.len(), usize::SIZE * 4);
    }

    #[test]
    fn flat_vec() {
        let mut capture1 = Vec::<u8>::new();
        let v = vec![1, 3, 4, 6];

        let expected_len = 2 * usize::SIZE + v.len() * i32::SIZE;
        trace!("{}", [i32]: v, .out=capture1);
        assert_eq!(capture1.len(), expected_len);
        let mut capture2 = Vec::<u8>::new();
        trace!("{}", Vec<i32>: v, .out=capture2);
        assert_eq!(capture2.len(), expected_len);

        assert_eq!(capture1[usize::SIZE..], capture2[usize::SIZE..]);
        assert_ne!(capture1[..usize::SIZE], capture2[..usize::SIZE]);
    }

    #[test]
    fn nested_vecs() {
        let mut capture1 = Vec::<u8>::new();
        let mut capture2 = Vec::<u8>::new();
        let mut capture3 = Vec::<u8>::new();
        let mut capture4 = Vec::<u8>::new();
        let owning = vec![vec![1, 2, 3, 4], vec![10]];
        let referencing_vec = vec![&owning[0], &owning[1]];
        let referencing_slice = vec![&owning[0][..], &owning[1]];

        let expected_len = 2 * usize::SIZE
            + owning
                .iter()
                .map(|inner| inner.len() * i32::SIZE + usize::SIZE)
                .sum::<usize>();
        trace!("{}", Vec<Vec<i32>>: owning, .out=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", [Vec<i32>]: owning, .out=capture2);
        assert_eq!(capture2.len(), expected_len);
        trace!("{}", [&Vec<i32>]: referencing_vec, .out=capture3);
        assert_eq!(capture3.len(), expected_len);
        trace!("{}", [&[i32]]: referencing_slice, .out=capture4);
        assert_eq!(capture4.len(), expected_len);

        assert_eq!(capture1[usize::SIZE..], capture2[usize::SIZE..]);
        assert_eq!(capture1[usize::SIZE..], capture3[usize::SIZE..]);
        assert_eq!(capture1[usize::SIZE..], capture4[usize::SIZE..]);
    }

    #[test]
    fn string() {
        let mut capture1 = Vec::<u8>::new();
        let mut capture2 = Vec::<u8>::new();
        let mut capture3 = Vec::<u8>::new();
        let literal = "Hello, there!";
        let owning = String::from_str(literal).unwrap();

        let expected_len = usize::SIZE + literal.len() + u8::SIZE;
        trace!("{}", str: literal, .out=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", str: owning, .out=capture2);
        assert_eq!(capture2.len(), expected_len);
        trace!("{}", String: owning, .out=capture3);
        assert_eq!(capture3.len(), expected_len);

        assert_eq!(capture1[usize::SIZE..], capture2[usize::SIZE..]);
        assert_eq!(capture1[usize::SIZE..], capture3[usize::SIZE..]);
    }
}
