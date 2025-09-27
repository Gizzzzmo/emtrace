use std::io::StdoutLock;
use std::io::Write;
use std::mem::size_of;
use std::ops::DerefMut;
use std::sync::MutexGuard;

cfg_if::cfg_if!(
    if #[cfg(feature="ptr_u128")] {
        pub type PointerT = u128;
    } else if #[cfg(feature="ptr_u64")] {
        pub type PointerT = u64;
    } else if #[cfg(feature="ptr_u32")] {
        pub type PointerT = u32;
    } else if #[cfg(feature="ptr_u16")] {
        pub type PointerT = u16;
    } else if #[cfg(feature="ptr_u8")] {
        pub type PointerT = u8;
    } else {
        pub type PointerT = u128;
    }
);

cfg_if::cfg_if!(
    if #[cfg(feature="size_u128")] {
        pub type SizeT = u128;
    } else if #[cfg(feature="size_u64")] {
        pub type SizeT = u64;
    } else if #[cfg(feature="size_u32")] {
        pub type SizeT = u32;
    } else if #[cfg(feature="size_u16")] {
        pub type SizeT = u16;
    } else if #[cfg(feature="size_u8")] {
        pub type SizeT = u8;
    } else {
        pub type SizeT = u128;
    }
);

const MAGIC_SIZE: usize = 36 + 3 * size_of::<SizeT>();

cfg_if::cfg_if!(
    if #[cfg(feature="alignment_power_0")] {
        pub const ALIGNMENT_POWER: u8 = 0;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(1), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_1")] {
        pub const ALIGNMENT_POWER: u8 = 1;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(2), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_2")] {
        pub const ALIGNMENT_POWER: u8 = 2;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(4), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_3")] {
        pub const ALIGNMENT_POWER: u8 = 3;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(8), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_4")] {
        pub const ALIGNMENT_POWER: u8 = 4;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(16), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_5")] {
        pub const ALIGNMENT_POWER: u8 = 5;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(32), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_6")] {
        pub const ALIGNMENT_POWER: u8 = 6;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(64), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_7")] {
        pub const ALIGNMENT_POWER: u8 = 7;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(128), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_8")] {
        pub const ALIGNMENT_POWER: u8 = 8;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(256), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_9")] {
        pub const ALIGNMENT_POWER: u8 = 9;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(512), C)]
                $struct
            }
        }
    } else if #[cfg(feature="alignment_power_10")] {
        pub const ALIGNMENT_POWER: u8 = 10;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(1024), C)]
                $struct
            }
        }
    } else {
        pub const ALIGNMENT_POWER: u8 = 6;
        macro_rules! with_alignment {
            ($struct:item) => {
                #[repr(align(64), C)]
                $struct
            }
        }
    }
);
with_alignment! {
    pub struct Magic {
        pub bytes: [u8; MAGIC_SIZE],
    }
}
with_alignment! {
    pub struct FormatInfo<const LEN: usize> {
        pub bytes: [u8; LEN],
    }
}

#[derive(Copy, Clone)]
pub enum Size {
    Static(usize),
    NullTerminated,
    LengthPrefixed,
}

impl Size {
    pub const fn bytes(&self) -> [u8; size_of::<SizeT>()] {
        match self {
            Self::NullTerminated => NULL_TERMINATED.to_ne_bytes(),
            Self::LengthPrefixed => LENGTH_PREFIXED.to_ne_bytes(),
            Self::Static(size) => (*size as SizeT).to_ne_bytes(),
        }
    }
}

pub const NULL_TERMINATED: SizeT = 1 << (SizeT::BITS - 1);
pub const LENGTH_PREFIXED: SizeT = 1 << (SizeT::BITS - 2);
pub const PY_FORMAT: SizeT = 0;
pub const NO_FORMAT: SizeT = 1;
pub const C_STYLE_FORMAT: SizeT = 2;

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
    fn begin(&mut self, info_addr: PointerT, total_size: SizeT);
}

pub fn init<T: Out>(out: &mut T) {
    magic_address_bytes().serialize(out);
}

impl<T: Out> Out for MutexGuard<'_, T> {
    fn out(&mut self, b: &[u8]) {
        self.deref_mut().out(b)
    }
    fn begin(&mut self, info_addr: PointerT, total_size: SizeT) {
        self.deref_mut().begin(info_addr, total_size)
    }
}

impl Out for StdoutLock<'_> {
    fn out(&mut self, b: &[u8]) {
        self.write_all(b).unwrap();
    }
    fn begin(&mut self, _info_addr: PointerT, _total_size: SizeT) {}
}

impl Out for Vec<u8> {
    fn out(&mut self, b: &[u8]) {
        self.extend_from_slice(b);
    }
    fn begin(&mut self, _info_addr: PointerT, total_size: SizeT) {
        let total_size = total_size & !(NULL_TERMINATED | LENGTH_PREFIXED);
        self.reserve(total_size as usize);
    }
}

pub const DESCENDANTS_LIMIT: usize = 0x1000;

pub trait Trace {
    const SIZE: Size;
    const ID: &'static str;
    const NUM_CHILDREN: usize;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)];
    /// How many SizeT fields the type requires in the offset table
    const NUM_OFFSET_TABLE_ENTRIES: usize = {
        let mut n = 0;
        // to store own size, and number of children
        n += 2;
        // to store offset to own id
        n += 1;

        let mut i = 0;
        let mut remaining = Self::NUM_CHILDREN;

        // iterate over all descendants
        while remaining > 0 {
            let descendant = Self::DESCENDANTS[i];

            // to store descendant's size and number of children
            n += 2;
            // to store offsets to descendant's name, and id
            n += 2;

            remaining += descendant.3;
            remaining -= 1;
            i += 1;
        }

        n
    };

    /// how many bytes this type needs to describe its type to the parser
    const INFO_SIZE: usize = {
        let mut size = 0;
        // to store own id as null-terminated string
        size += Self::ID.len() + size_of::<u8>();

        let mut i = 0;
        let mut remaining = Self::NUM_CHILDREN;

        // iterate over all descendants
        while remaining > 0 {
            let descendant = Self::DESCENDANTS[i];

            // to store descendant's name as null-terminated string
            size += descendant.0.len() + size_of::<u8>();
            // to store descendant's id as null-terminated string
            size += descendant.1.len() + size_of::<u8>();

            remaining += descendant.3;
            remaining -= 1;
            i += 1;
        }

        size
    };
    fn serialize<O: Out>(&self, f: &mut O);
    fn size(&self) -> usize;
}

impl<T> Trace for &T
where
    T: Trace,
    T: ?Sized,
{
    const SIZE: Size = T::SIZE;
    const ID: &'static str = T::ID;
    const NUM_CHILDREN: usize = T::NUM_CHILDREN;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] = T::DESCENDANTS;
    fn serialize<O: Out>(&self, f: &mut O) {
        (*self).serialize(f)
    }
    fn size(&self) -> usize {
        (*self).size()
    }
}

impl<T: Trace> Trace for Box<T> {
    const SIZE: Size = T::SIZE;
    const ID: &'static str = T::ID;
    const NUM_CHILDREN: usize = T::NUM_CHILDREN;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] = T::DESCENDANTS;
    fn serialize<O: Out>(&self, f: &mut O) {
        self.as_ref().serialize(f)
    }
    fn size(&self) -> usize {
        self.as_ref().size()
    }
}

impl Trace for str {
    const SIZE: Size = Size::NullTerminated;
    const ID: &'static str = "string";
    const NUM_CHILDREN: usize = 0;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] = &[];
    fn serialize<O: Out>(&self, f: &mut O) {
        f.out(self.as_bytes());
    }
    fn size(&self) -> usize {
        self.len()
    }
}

impl Trace for String {
    const SIZE: Size = str::SIZE;
    const ID: &'static str = str::ID;
    const NUM_CHILDREN: usize = str::NUM_CHILDREN;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] = str::DESCENDANTS;
    fn serialize<O: Out>(&self, f: &mut O) {
        self.as_str().serialize(f)
    }
    fn size(&self) -> usize {
        self.as_str().size()
    }
}

impl<T: Trace> Trace for [T] {
    const SIZE: Size = Size::LengthPrefixed;
    const ID: &'static str = "list";
    const NUM_CHILDREN: usize = 1;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] = &{
        let mut children = [("", T::ID, T::SIZE, T::NUM_CHILDREN); DESCENDANTS_LIMIT];

        let mut remaining = T::NUM_CHILDREN;
        let mut i = 0;
        while remaining > 0 {
            children[i + Self::NUM_CHILDREN] = T::DESCENDANTS[remaining];
            remaining += T::DESCENDANTS[remaining].3;
            remaining -= 1;
            i += 1;
        }

        children
    };
    fn serialize<O: Out>(&self, f: &mut O) {
        for el in self {
            if matches!(T::SIZE, Size::LengthPrefixed) {
                (el.size() as SizeT).serialize(f);
            }
            el.serialize(f);
            if matches!(T::SIZE, Size::NullTerminated) {
                0u8.serialize(f);
            }
        }
    }
    fn size(&self) -> usize {
        self.len()
    }
}

impl<T: Trace> Trace for Vec<T> {
    const SIZE: Size = <[T] as Trace>::SIZE;
    const ID: &'static str = <[T] as Trace>::ID;
    const NUM_CHILDREN: usize = <[T] as Trace>::NUM_CHILDREN;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] =
        <[T] as Trace>::DESCENDANTS;
    fn serialize<O: Out>(&self, f: &mut O) {
        self[..].serialize(f)
    }
    fn size(&self) -> usize {
        self[..].size()
    }
}

impl<T: Trace, const N: usize> Trace for [T; N] {
    const SIZE: Size = Size::Static(N);
    const ID: &'static str = "list";
    const NUM_CHILDREN: usize = 1;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] =
        <[T] as Trace>::DESCENDANTS;

    fn serialize<O: Out>(&self, f: &mut O) {
        for el in self {
            if matches!(T::SIZE, Size::LengthPrefixed) {
                (el.size() as SizeT).serialize(f);
            }
            el.serialize(f);
            if matches!(T::SIZE, Size::NullTerminated) {
                0u8.serialize(f);
            }
        }
    }

    fn size(&self) -> usize {
        N
    }
}

macro_rules! impl_trace_for_primitive {
    ($type:ty, $id:expr) => {
        impl Trace for $type {
            const SIZE: Size = Size::Static(std::mem::size_of::<Self>());
            const ID: &'static str = $id;
            const NUM_CHILDREN: usize = 0;
            const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] = &[];

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
    const SIZE: Size = Size::Static(std::mem::size_of::<u8>());
    const ID: &'static str = "bool";
    const NUM_CHILDREN: usize = 0;
    const DESCENDANTS: &'static [(&'static str, &'static str, Size, usize)] = &[];

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
            const NUM_PREFIX: usize = {
                let mut size = 5;
                $($(
                    size += <$types as $crate::Trace>::NUM_OFFSET_TABLE_ENTRIES;
                )*)?
                size
            };
            const INFO_SIZE: usize = {
                let mut size: usize = 0;
                $($(
                        size += <$types as $crate::Trace>::INFO_SIZE;
                )*)?
                size += NUM_PREFIX * size_of::<$crate::SizeT>() + FMT.as_bytes().len() + file!().as_bytes().len() + 2;
                size
            };
            #[unsafe(link_section = ".emtrace")]
            $(
                static _DUMMY: i32 = 0;
                #[unsafe(link_section = $section)]
            )?
            #[used]
            static FMT_INFO: $crate::FormatInfo<INFO_SIZE> = {
                let size_t_size = size_of::<$crate::SizeT>();
                let num_args = ($crate::count!($($($types)*)?)) as $crate::SizeT;
                let mut offset_idx: usize = 0;
                let mut data_idx = (NUM_PREFIX * size_t_size) as $crate::SizeT;
                let mut info  = [0; INFO_SIZE];

                let num_args_arr = num_args.to_ne_bytes();
                let mut i = 0;
                while i < size_t_size {
                    info[offset_idx] = num_args_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                let fmt_offset_arr = data_idx.to_ne_bytes();
                let mut i = 0;
                while i < size_t_size {
                    info[offset_idx] = fmt_offset_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                let fmt_str_arr = FMT.as_bytes();
                let mut i = 0;
                while i < FMT.len() {
                    info[data_idx as usize] = fmt_str_arr[i];
                    data_idx += 1;
                    i += 1;
                }
                data_idx += 1;

                ($($(
                    {
                        let id_offset_arr = data_idx.to_ne_bytes();
                        let mut i = 0;
                        while i < size_t_size {
                            info[offset_idx] = id_offset_arr[i];
                            offset_idx += 1;
                            i += 1;
                        }

                        let size_arr = <$types as $crate::Trace>::SIZE.bytes();
                        let mut i = 0;
                        while i < size_t_size {
                            info[offset_idx] = size_arr[i];
                            offset_idx += 1;
                            i += 1;
                        }

                        let num_children_arr = (<$types as $crate::Trace>::NUM_CHILDREN as $crate::SizeT).to_ne_bytes();
                        let mut i = 0;
                        while i < size_t_size {
                            info[offset_idx] = num_children_arr[i];
                            offset_idx += 1;
                            i += 1;
                        }

                        let id_arr = <$types as $crate::Trace>::ID.as_bytes();
                        let mut i = 0;
                        while i < <$types as $crate::Trace>::ID.len() {
                            info[data_idx as usize] = id_arr[i];
                            data_idx += 1;
                            i += 1;
                        }
                        data_idx += 1;

                        let mut j = 0;
                        let mut remaining = <$types as $crate::Trace>::NUM_CHILDREN;
                        while remaining > 0 {
                            let descendant = <$types as $crate::Trace>::DESCENDANTS[j];

                            let name_offset_arr = data_idx.to_ne_bytes();
                            let mut i = 0;
                            while i < size_t_size {
                                info[offset_idx] = name_offset_arr[i];
                                offset_idx += 1;
                                i += 1;
                            }

                            let size_arr = descendant.2.bytes();
                            let mut i = 0;
                            while i < size_t_size {
                                info[offset_idx] = size_arr[i];
                                offset_idx += 1;
                                i += 1;
                            }

                            let num_children_arr = (descendant.3 as $crate::SizeT).to_ne_bytes();
                            let mut i = 0;
                            while i < size_t_size {
                                info[offset_idx] = num_children_arr[i];
                                offset_idx += 1;
                                i += 1;
                            }

                            let n = descendant.0.len();
                            let name_arr = descendant.0.as_bytes();
                            let mut i = 0;
                            while i < n {
                                info[data_idx as usize] = name_arr[i];
                                data_idx += 1;
                                i += 1;
                            }
                            data_idx += 1;

                            let id_offset_arr = data_idx.to_ne_bytes();
                            let mut i = 0;
                            while i < size_t_size {
                                info[offset_idx] = id_offset_arr[i];
                                offset_idx += 1;
                                i += 1;
                            }

                            let n = descendant.1.len();
                            let id_arr = descendant.1.as_bytes();
                            let mut i = 0;
                            while i < n {
                                info[data_idx as usize] = id_arr[i];
                                data_idx += 1;
                                i += 1;
                            }
                            data_idx += 1;

                            remaining += descendant.3;
                            remaining -= 1;
                            j += 1;
                        }

                    }
                ),*)?);

                let formatter = $crate::NO_FORMAT;
                // if there is at least one `,type: arg` pair in the macro arguments,
                // then this will overwrite the default NO_FORMAT formatter to PY_FORMAT
                $($(
                    let _dummy = formatter;
                    let _dumme = <$types as $crate::Trace>::SIZE;
                    let formatter =  $crate::PY_FORMAT;
                )*)?
                $(
                    let _dummy = formatter;
                    let formatter = $formatter;
                )?
                let formatter_arr = (formatter as $crate::SizeT).to_ne_bytes();
                let mut i = 0;
                while i < size_t_size {
                    info[offset_idx] = formatter_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                let file_offset_arr = data_idx.to_ne_bytes();
                let mut i = 0;
                while i < size_t_size {
                    info[offset_idx] = file_offset_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                let file_arr = file!().as_bytes();
                let mut i = 0;
                while i < file!().len() {
                    info[data_idx as usize] = file_arr[i];
                    data_idx += 1;
                    i += 1;
                }

                let line_arr = (line!() as $crate::SizeT).to_ne_bytes();
                let mut i = 0;
                while i < size_t_size {
                    info[offset_idx] = line_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                $crate::FormatInfo{
                    bytes: info
                }

            };
            const TOTAL_SIZE: usize = {
                let mut total_size = 0usize;
                $($(
                    total_size += match <$types as $crate::Trace>::SIZE {
                        $crate::Size::NullTerminated |$crate::Size::LengthPrefixed => 0,
                        $crate::Size::Static(size) => size,

                    };
                )*)?
                total_size += size_of::<$crate::SizeT>();

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
            let addr = (FMT_INFO.bytes.as_ptr().addr() >> $crate::ALIGNMENT_POWER) as $crate::PointerT;
            gadget_ref.begin(addr, TOTAL_SIZE.try_into().unwrap());
            $crate::Trace::serialize(&addr, gadget_ref);
            $($(
                let x:&$types = &$args;
                if matches!(<$types as $crate::Trace>::SIZE, $crate::Size::LengthPrefixed) {
                    $crate::Trace::serialize(&($crate::Trace::size(x) as $crate::SizeT), gadget_ref);
                }
                $crate::Trace::serialize(x, gadget_ref);
                if matches!(<$types as $crate::Trace>::SIZE, $crate::Size::NullTerminated) {
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
        expect!($expected, .section=".emtrace.test.expected")
    };
}

#[unsafe(link_section = ".emtrace")]
#[used]
pub static EMTRACE_MAGIC: Magic = {
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
    magic[32] = 36;
    magic[33] = size_of::<SizeT>() as u8;
    magic[34] = size_of::<PointerT>() as u8;
    magic[35] = ALIGNMENT_POWER;

    let mut idx = 36;

    #[allow(overflowing_literals)]
    let byte_order_id_arr = (0x0f0e0d0c0b0a09080706050403020100 as SizeT).to_ne_bytes();
    let mut i = 0;
    while i < size_of::<SizeT>() {
        magic[idx] = byte_order_id_arr[i];
        idx += 1;
        i += 1;
    }

    let null_terminated_arr = NULL_TERMINATED.to_ne_bytes();
    let mut i = 0;
    while i < size_of::<SizeT>() {
        magic[idx] = null_terminated_arr[i];
        idx += 1;
        i += 1;
    }

    let length_prefixed_arr = LENGTH_PREFIXED.to_ne_bytes();
    let mut i = 0;
    while i < size_of::<SizeT>() {
        magic[idx] = length_prefixed_arr[i];
        idx += 1;
        i += 1;
    }
    Magic { bytes: magic }
};

pub fn magic_address_bytes() -> [u8; size_of::<PointerT>()] {
    let raw_address = (&EMTRACE_MAGIC as *const Magic).addr();
    ((raw_address >> ALIGNMENT_POWER) as PointerT).to_ne_bytes()
}

#[cfg(test)]
mod tests {
    use super::{Out, SizeT, trace};
    use std::str::FromStr;

    #[test]
    fn test() {
        let mut capture = Vec::new();
        trace!("{}", i32: 32, .out=capture);
        assert_eq!(capture.len(), size_of::<SizeT>() + size_of::<i32>());
        let mut arg_bytes: [u8; size_of::<i32>()] = [0; size_of::<i32>()];
        arg_bytes
            .copy_from_slice(&capture[size_of::<SizeT>()..size_of::<SizeT>() + size_of::<i32>()]);
        let arg = i32::from_ne_bytes(arg_bytes);
        assert_eq!(arg, 32);
    }

    #[test]
    fn empty_vec() {
        let mut capture = Vec::new();
        let v = Vec::<i32>::new();
        trace!("{}", [i32]: v, .out=capture);
        assert_eq!(capture.len(), size_of::<SizeT>() * 2);
        trace!("{}", Vec<i32>: v, .out=capture);
        assert_eq!(capture.len(), size_of::<SizeT>() * 4);
    }

    #[test]
    fn flat_vec() {
        let mut capture1 = Vec::new();
        let mut capture2 = Vec::new();
        let v = vec![1, 3, 4, 6];

        let expected_len = 2 * size_of::<SizeT>() + v.len() * size_of::<i32>();
        trace!("{}", [i32]: v, .out=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", Vec<i32>: v, .out=capture2);
        assert_eq!(capture2.len(), expected_len);

        assert_eq!(
            capture1[size_of::<SizeT>()..],
            capture2[size_of::<SizeT>()..]
        );
        assert_ne!(
            capture1[..size_of::<SizeT>()],
            capture2[..size_of::<SizeT>()]
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

        let expected_len = 2 * size_of::<SizeT>()
            + owning
                .iter()
                .map(|inner| inner.len() * size_of::<i32>() + size_of::<SizeT>())
                .sum::<usize>();
        trace!("{}", Vec<Vec<i32>>: owning, .out=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", [Vec<i32>]: owning, .out=capture2);
        assert_eq!(capture2.len(), expected_len);
        trace!("{}", [&Vec<i32>]: referencing_vec, .out=capture3);
        assert_eq!(capture3.len(), expected_len);
        trace!("{}", [&[i32]]: referencing_slice, .out=capture4);
        assert_eq!(capture4.len(), expected_len);

        assert_eq!(
            capture1[size_of::<SizeT>()..],
            capture2[size_of::<SizeT>()..]
        );
        assert_eq!(
            capture1[size_of::<SizeT>()..],
            capture3[size_of::<SizeT>()..]
        );
        assert_eq!(
            capture1[size_of::<SizeT>()..],
            capture4[size_of::<SizeT>()..]
        );
    }

    #[test]
    fn string() {
        let mut capture1 = Vec::new();
        let mut capture2 = Vec::new();
        let mut capture3 = Vec::new();
        let literal = "Hello, there!";
        let owning = String::from_str(literal).unwrap();

        let expected_len = size_of::<SizeT>() + literal.len() + size_of::<u8>();
        trace!("{}", str: literal, .out=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", str: owning, .out=capture2);
        assert_eq!(capture2.len(), expected_len);
        trace!("{}", String: owning, .out=capture3);
        assert_eq!(capture3.len(), expected_len);

        assert_eq!(
            capture1[size_of::<SizeT>()..],
            capture2[size_of::<SizeT>()..]
        );
        assert_eq!(
            capture1[size_of::<SizeT>()..],
            capture3[size_of::<SizeT>()..]
        );
    }

    #[test]
    fn vec_of_string() {
        let mut capture1 = Vec::new();
        let mut capture2 = Vec::new();
        let slices = vec!["A", "vector", "of", "strings"];
        let owning = slices
            .iter()
            .map(|s| String::from_str(s).unwrap())
            .collect();

        let expected_len = 2 * size_of::<SizeT>()// for pointer, and length-prefix
            + slices
                .iter()
                .map(|inner| inner.len() + size_of::<u8>()) // for content and null-terminator
                .sum::<usize>();
        trace!("{}", Vec<&str>: slices, .out=capture1);
        assert_eq!(capture1.len(), expected_len);
        trace!("{}", Vec<String>: owning, .out=capture2);
        assert_eq!(capture2.len(), expected_len);

        assert_eq!(
            capture1[size_of::<SizeT>()..],
            capture2[size_of::<SizeT>()..]
        );
    }
}
