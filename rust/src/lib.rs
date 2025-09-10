mod emtrace {
    pub union Transmute<T: Copy, U: Copy> {
        from: T,
        to: U,
    }

    pub const NULL_TERMINATED: usize = usize::max_value() | (1 as usize);
    pub const LENGTH_PREFIXED: usize = usize::max_value() & !(1 as usize);

    pub trait Trace {
        const SIZE: usize;
        const ID: &'static str;
        // fn serialize<Out>(&self, f: Out) where
        //     Out: Fn(&[u8; Self::SIZE]) -> ();
        fn serialize<Out>(&self, f: &mut Out)
        where
            Out: FnMut(&[u8]) -> ();
        fn size(&self) -> usize;
    }

    impl Trace for &str {
        const SIZE: usize = NULL_TERMINATED;
        const ID: &'static str = "string";
        fn serialize<Out>(&self, f: &mut Out)
        where
            Out: FnMut(&[u8]) -> (),
        {
            f(self.as_bytes());
            f(&[0 as u8]);
        }
        fn size(&self) -> usize {
            self.as_bytes().len()
        }
    }

    impl Trace for i32 {
        const SIZE: usize = size_of::<Self>();
        const ID: &'static str = "signed";
        fn serialize<Out>(&self, f: &mut Out)
        where
            Out: FnMut(&[u8]) -> (),
        {
            unsafe {
                let bytes = Transmute::<&Self, &[u8; size_of::<Self>()]> { from: self }.to;
                f(bytes);
            }
        }
        fn size(&self) -> usize {
            size_of::<Self>()
        }
    }

    impl Trace for usize {
        const SIZE: usize = size_of::<Self>();
        const ID: &'static str = "unsigned";
        fn serialize<Out>(&self, f: &mut Out)
        where
            Out: FnMut(&[u8]) -> (),
        {
            unsafe {
                let bytes = Transmute::<&Self, &[u8; size_of::<Self>()]> { from: self }.to;
                f(bytes);
            }
        }
        fn size(&self) -> usize {
            size_of::<Self>()
        }
    }

    impl Trace for u8 {
        const SIZE: usize = size_of::<u8>();
        const ID: &'static str = "char";
        fn serialize<Out>(&self, f: &mut Out)
        where
            Out: FnMut(&[u8]) -> (),
        {
            f(&[*self]);
        }
        fn size(&self) -> usize {
            size_of::<u8>()
        }
    }

    use std::io::Write;
    pub fn out(b: &[u8]) {
        let mut handle = std::io::stdout().lock();
        handle.write_all(b).unwrap();
    }

    #[macro_export]
    macro_rules! count {
        () => (0usize);
        ( $x:tt $($xs:tt)* ) => (1usize + count!($($xs)*));
    }

    #[macro_export]
    macro_rules! trace {
    ($fmt:literal $(, $($types:ty : $args:expr),+)? $(, .section = $section:literal)? $(, .out = $out:expr)?) => {
        {
            const INFO_SIZE: usize = {
                let mut size: usize = 0;
                ($($(
                    {
                        const N: usize = <$types as Trace>::ID.as_bytes().len();
                        size += N + 1 + size_of::<usize>() * 2;
                    }
                ),*)?);
                size + 4 * size_of::<usize>() + $fmt.as_bytes().len() + file!().as_bytes().len() + 2
            };
            #[unsafe(link_section = ".emtrace")]
            $(
                static _DUMMY: i32 = 0;
                #[unsafe(link_section = $section)]
            )?
            #[used]
            static FMT_INFO: [u8; INFO_SIZE] = unsafe {
                let usize_size = size_of::<usize>();
                let num_args: usize = count!($($($types)*)?);
                let mut offset_idx: usize = 0;
                let mut data_idx: usize = (4 + num_args * 2) * usize_size;
                let mut info: [u8; INFO_SIZE] = [0; INFO_SIZE];

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

                const N: usize = $fmt.as_bytes().len();
                let fmt_str_arr = *Transmute::<*const [u8; N], &[u8; N]> {
                    from: $fmt.as_ptr() as *const [u8; N],
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

                        let size_arr = <$types as Trace>::SIZE.to_ne_bytes();
                        let mut i = 0;
                        while i < usize_size {
                            info[offset_idx] = size_arr[i];
                            offset_idx += 1;
                            i += 1;
                        }

                        const N: usize = <$types as Trace>::ID.as_bytes().len();
                        let id_arr = *Transmute::<*const [u8; N], &[u8; N]> {
                            from: <$types as Trace>::ID.as_ptr() as *const [u8; N],
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
                let file_offset_arr = data_idx.to_ne_bytes();
                let mut i = 0;
                while i < usize_size {
                    info[offset_idx] = file_offset_arr[i];
                    offset_idx += 1;
                    i += 1;
                }

                const M: usize = file!().as_bytes().len();
                let file_arr = *Transmute::<*const [u8; M], &[u8; M]> {
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
            let out = &mut out;
            $(
                let _dummy = out;
                let out = $out;
            )?
            Trace::serialize(&FMT_INFO.as_ptr().addr(), out);
            $($(
                let x: $types = $args;
                if <$types as Trace>::SIZE == LENGTH_PREFIXED {
                    Trace::serialize(&Trace::size(&x), out);
                }
                Trace::serialize(&x, out);
            )*)?
        }
    };
}

    #[unsafe(link_section = ".emtrace")]
    #[used]
    pub static EMTRACE_MAGIC: [u8; 32] = [
        0xd1, 0x97, 0xf5, 0x22, 0xd9, 0x26, 0x9f, 0xd1, 0xad, 0x70, 0x33, 0x92, 0xf6, 0x59, 0xdf,
        0xd0, 0xfb, 0xec, 0xbd, 0x60, 0x97, 0x13, 0x25, 0xe8, 0x92, 0x01, 0xb2, 0x5a, 0x38, 0x5d,
        0x9e, 0xc7,
    ];

    #[cfg(test)]
    mod tests {
        use super::*;

        #[test]
        fn test() {
            let mut buffer = Vec::<u8>::new();
            let mut out = |b: &[u8]| {
                buffer.extend_from_slice(b);
            };
            trace!("{}", i32: 0, .out=&mut out);
            assert_eq!(buffer.len(), size_of::<usize>() + size_of::<i32>());
        }
    }

    // fn blub(b: &[u8]) {
    //     let mut handle = std::io::stdout().lock();
    //     handle.write_all(b).unwrap();
    // }
    //
    // fn main() -> () {
    //     // Trace::serialize(&EMTRACE_MAGIC.as_ptr().addr(), &mut out);
    //     // emtrace!("format string {}\n", usize : 56, .section = "blub", .out = &mut blub);
    //     // emtrace!("Hello, World! {} {}\n", i32 : -10, u8 : b'b');
    //     // emtrace!("--------------------\n\
    //     //       |{:^18}|\n\
    //     //       --------------------\n", &str : "Hello there!");
    //     // emtrace!("format string", i32 56, i32 78, i32 66, i32 43);
    //     // println!("{}", "a".len());
    //     // let mut x = [3;4];
    //     // let y = [1;2];
    //     // x[..2].copy_from_slice(&y);
    //     // println!("{:?}", x);
    //     return;
    // }
}
