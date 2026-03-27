use proc_macro::TokenStream;
use proc_macro2::{Span, TokenStream as TokenStream2};
use quote::{format_ident, quote};
use syn::{
    Expr, LitStr, Token, Type,
    parse::discouraged::Speculative,
    parse::{Parse, ParseStream},
    parse_macro_input,
};

// ──────────────────────────────────────────────────────────────────────────────
// Shared type that describes one trace argument
// ──────────────────────────────────────────────────────────────────────────────

/// One argument in a `trace!` invocation.
/// Syntax: `[Type:] expr`
struct TraceArg {
    ty: Option<Type>,
    expr: Expr,
}

impl Parse for TraceArg {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        // Try to parse `Type: expr`.  We need look-ahead because a bare expr
        // can start with many of the same tokens as a type (paths, generics…).
        // Strategy: try `Type :` fork, if that succeeds and the colon is not
        // `::`, treat it as an explicit type annotation.
        let fork = input.fork();
        let ty_result: syn::Result<(Type, Token![:])> = (|| {
            let ty = fork.parse::<Type>()?;
            // make sure it's `:` not `::`
            if fork.peek(Token![:]) && !fork.peek(Token![::]) {
                let colon = fork.parse::<Token![:]>()?;
                Ok((ty, colon))
            } else {
                Err(fork.error("not a type annotation"))
            }
        })();

        match ty_result {
            Ok((ty, _colon)) => {
                input.advance_to(&fork);
                let expr = input.parse::<Expr>()?;
                Ok(TraceArg { ty: Some(ty), expr })
            }
            Err(_) => {
                let expr = input.parse::<Expr>()?;
                Ok(TraceArg { ty: None, expr })
            }
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Parser for the full `trace!` / `traceln!` invocation
// ──────────────────────────────────────────────────────────────────────────────

struct TraceInput {
    fmt_parts: Vec<LitStr>,
    args: Vec<TraceArg>,
    section: Option<LitStr>,
    sink_guard: Option<Expr>,
    sink: Option<Expr>,
    formatter: Option<Expr>,
    error_handler: Option<Expr>,
    no_panic: bool,
}

impl Parse for TraceInput {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        // One or more adjacent string literals form the format string
        let mut fmt_parts = Vec::new();
        fmt_parts.push(input.parse::<LitStr>()?);
        while input.peek(LitStr) {
            fmt_parts.push(input.parse::<LitStr>()?);
        }

        let mut args = Vec::new();
        let mut section = None;
        let mut sink_guard = None;
        let mut sink = None;
        let mut formatter = None;
        let mut error_handler = None;
        let mut no_panic = false;

        // Now parse zero or more `, item` entries
        while input.peek(Token![,]) {
            input.parse::<Token![,]>()?;
            if input.is_empty() {
                break;
            }

            // Options start with `.`
            if input.peek(Token![.]) {
                input.parse::<Token![.]>()?;
                let ident: syn::Ident = input.parse()?;
                match ident.to_string().as_str() {
                    "section" => {
                        input.parse::<Token![=]>()?;
                        section = Some(input.parse::<LitStr>()?);
                    }
                    "sink_guard" => {
                        input.parse::<Token![=]>()?;
                        sink_guard = Some(input.parse::<Expr>()?);
                    }
                    "sink" => {
                        input.parse::<Token![=]>()?;
                        sink = Some(input.parse::<Expr>()?);
                    }
                    "formatter" => {
                        input.parse::<Token![=]>()?;
                        formatter = Some(input.parse::<Expr>()?);
                    }
                    "error_handler" => {
                        input.parse::<Token![=]>()?;
                        error_handler = Some(input.parse::<Expr>()?);
                    }
                    "no_panic" => {
                        no_panic = true;
                    }
                    other => {
                        return Err(syn::Error::new(
                            ident.span(),
                            format!("unknown option `.{other}`"),
                        ));
                    }
                }
            } else {
                args.push(input.parse::<TraceArg>()?);
            }
        }

        Ok(TraceInput {
            fmt_parts,
            args,
            section,
            sink_guard,
            sink,
            formatter,
            error_handler,
            no_panic,
        })
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// `trace!` and `traceln!` implementation
// ──────────────────────────────────────────────────────────────────────────────

fn trace_impl(input: TraceInput, newline: bool) -> TokenStream2 {
    let TraceInput {
        fmt_parts,
        args,
        section,
        sink_guard,
        sink,
        formatter,
        error_handler,
        no_panic,
    } = input;

    // Concatenate format string parts, optionally appending newline
    let fmt_str = {
        let mut s = fmt_parts.iter().map(|l| l.value()).collect::<String>();
        if newline {
            s.push('\n');
        }
        s
    };

    let section_name = section
        .as_ref()
        .map(|l| l.value())
        .unwrap_or_else(|| ".emtrace".to_string());

    // ── determine sink expression ──────────────────────────────────────────
    // We need a mutable reference to a sink.
    //
    // Three cases:
    //  1. `.sink_guard = expr`  →  move expr into local, take &mut
    //  2. `.sink = expr`        →  take &mut of the expr (expr must be a place)
    //  3. default               →  stdout().lock()
    //
    // `.sink` is special: the sink value lives in the caller's scope and we
    // only borrow it, so we must NOT move it.

    let sink_setup: TokenStream2;
    let sink_ref_expr: TokenStream2;

    if let Some(ref sg) = sink_guard {
        // Move the guard into a local binding so it lives for the whole block
        sink_setup = quote! {
            let mut __emtrace_sink_guard = #sg;
        };
        sink_ref_expr = quote! { &mut __emtrace_sink_guard };
    } else if let Some(ref s) = sink {
        // Borrow the caller-owned sink
        sink_setup = quote! {};
        sink_ref_expr = quote! { &mut #s };
    } else {
        // Default: stdout
        sink_setup = quote! {
            let mut __emtrace_stdout = ::std::io::stdout().lock();
        };
        sink_ref_expr = quote! { &mut __emtrace_stdout };
    }

    // ── formatter value ───────────────────────────────────────────────────
    // Default: NO_FORMAT when no args, PY_FORMAT when there are args.
    let formatter_expr: TokenStream2 = if let Some(ref f) = formatter {
        quote! { #f }
    } else if args.is_empty() {
        quote! { ::emtrace::NO_FORMAT }
    } else {
        quote! { ::emtrace::PY_FORMAT }
    };

    // ── per-argument code ─────────────────────────────────────────────────
    // For each arg we need:
    //   1. A `static FMT_INFO_ARG_N` const to hold type info at compile time
    //      (we will inline everything into the single FMT_INFO struct instead)
    //   2. Runtime serialization code

    // Build the per-arg type reference tokens
    let arg_types: Vec<TokenStream2> = args
        .iter()
        .map(|a| {
            if let Some(ref ty) = a.ty {
                quote! { #ty }
            } else {
                // No explicit type; use a helper that forces the type to be inferred
                // from the expression.  We emit `_` and let Rust infer it.
                quote! { _ }
            }
        })
        .collect();

    // ── static format info generation ────────────────────────────────────
    // We generate a unique static holding the C-compatible TRCE record.
    // The static has type `FormatInfo<N>` where N is computed from a
    // `const {}` block calling `trce_record_size`.

    // Generate unique static name to avoid collisions
    let static_ident = format_ident!("__EMTRACE_FMT_INFO_{:x}", {
        use std::collections::hash_map::DefaultHasher;
        use std::hash::{Hash, Hasher};
        let mut h = DefaultHasher::new();
        fmt_str.hash(&mut h);
        section_name.hash(&mut h);
        h.finish()
    });

    // Build per-arg ArgInfo literal tokens for the const block
    let mut arg_info_tokens: Vec<TokenStream2> = Vec::new();
    for ty in arg_types.iter() {
        arg_info_tokens.push(quote! {
            ::emtrace::__private::ArgInfo {
                id: <#ty as ::emtrace::Trace>::ID,
                size: <#ty as ::emtrace::Trace>::SIZE_BYTES,
                flag: <#ty as ::emtrace::Trace>::FLAG,
                num_children: <#ty as ::emtrace::Trace>::NUM_CHILDREN,
                descendants: <#ty as ::emtrace::Trace>::DESCENDANTS,
                num_descendants: <#ty as ::emtrace::Trace>::NUM_DESCENDANTS,
            }
        });
    }

    let section_lit = LitStr::new(&section_name, Span::call_site());

    // ── runtime serialization ─────────────────────────────────────────────
    // Note: these statements run inside the closure passed to `__private::emit`,
    // which returns `Result<(), S::OutError>`.  We use plain `?` (no map_err).
    let serialize_args: Vec<TokenStream2> = args
        .iter()
        .enumerate()
        .map(|(i, arg)| {
            let ty = &arg_types[i];
            let expr = &arg.expr;
            quote! {
                {
                    let __x: &#ty = &(#expr);
                    if matches!(<#ty as ::emtrace::Trace>::FLAG, ::emtrace::FLAG_LENGTH_PREFIXED) {
                        let __len = ::emtrace::Trace::size(__x) as ::emtrace::SizeT;
                        ::emtrace::Trace::serialize(&__len, __sink)?;
                    }
                    ::emtrace::Trace::serialize(__x, __sink)?;
                    if matches!(<#ty as ::emtrace::Trace>::FLAG, ::emtrace::FLAG_NULL_TERMINATED) {
                        ::emtrace::Trace::serialize(&0u8, __sink)?;
                    }
                }
            }
        })
        .collect();

    // ── build the result expression ───────────────────────────────────────
    // The static type uses a const-generic N computed by trce_record_size.
    // `file!()` and `line!()` expand at the call site.
    let inner = quote! {
        {
            use ::core::mem::size_of;

            const __ARGS: &[::emtrace::__private::ArgInfo] = &[#(#arg_info_tokens),*];
            const __FMT: &str = #fmt_str;
            const __FILE: &str = file!();
            const __LINE: usize = line!() as usize;
            const __FORMATTER: usize = #formatter_expr;
            const __N: usize = ::emtrace::__private::trce_record_size(__ARGS, __FMT, __FILE);

            #[unsafe(link_section = #section_lit)]
            #[used]
            static #static_ident: ::emtrace::FormatInfo<__N> = {
                ::emtrace::__private::build_trce_record::<__N>(
                    __ARGS,
                    __FMT,
                    __FORMATTER,
                    __FILE,
                    __LINE,
                )
            };

            #sink_setup

            let __sink = #sink_ref_expr;

            // Compute address of the format info record (shifted by ALIGNMENT_POWER)
            let __addr = (#static_ident.bytes.as_ptr().addr()
                >> ::emtrace::ALIGNMENT_POWER)
                as ::emtrace::PointerT;

            // Total fixed-size bytes: pointer + static sizes of all args
            let __total: ::emtrace::SizeT = (size_of::<::emtrace::PointerT>()
                #(+ <#arg_types as ::emtrace::Trace>::STATIC_SIZE)*
            ) as ::emtrace::SizeT;

            // `emit` anchors type inference on the concrete sink type so
            // Error<BeginError, OutError> is always fully determined.
            ::emtrace::__private::emit(__sink, __addr, __total, |__sink| {
                #(#serialize_args)*
                Ok(())
            })
        }
    };

    // ── wrap in error handling ────────────────────────────────────────────
    if let Some(ref handler) = error_handler {
        quote! {
            {
                let __r = #inner;
                (#handler)(__r)
            }
        }
    } else if no_panic {
        inner
    } else {
        quote! {
            #inner.unwrap()
        }
    }
}

/// Emit a trace with the given format string and arguments.
///
/// # Syntax
///
/// ```text
/// trace!("fmt"  [, [Type:] expr]* [, .option[=value]]*)
/// ```
///
/// Arguments may optionally carry an explicit type annotation:
/// ```rust,ignore
/// trace!("value: {}", i32: my_val);
/// trace!("inferred: {}", my_val);
/// ```
///
/// # Options
///
/// - `.sink = <expr>` — write to this mutable-borrow sink
/// - `.sink_guard = <expr>` — move this sink (e.g. a `MutexGuard`) into the macro
/// - `.section = "<name>"` — custom ELF section for the format record (default `.emtrace`)
/// - `.formatter = <expr>` — `PY_FORMAT` / `C_STYLE_FORMAT` / `NO_FORMAT`
/// - `.error_handler = <closure>` — handle errors instead of panicking
/// - `.no_panic` — return `Result` instead of panicking
#[proc_macro]
pub fn trace(input: TokenStream) -> TokenStream {
    let parsed = parse_macro_input!(input as TraceInput);
    trace_impl(parsed, false).into()
}

/// Like [`trace!`] but appends a newline to the format string.
#[proc_macro]
pub fn traceln(input: TokenStream) -> TokenStream {
    let parsed = parse_macro_input!(input as TraceInput);
    trace_impl(parsed, true).into()
}

// ──────────────────────────────────────────────────────────────────────────────
// `expect!`
// ──────────────────────────────────────────────────────────────────────────────

struct ExpectInput {
    expected: LitStr,
    section: Option<LitStr>,
}

impl Parse for ExpectInput {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let expected = input.parse::<LitStr>()?;
        let mut section = None;
        if input.peek(Token![,]) {
            input.parse::<Token![,]>()?;
            if input.peek(Token![.]) {
                input.parse::<Token![.]>()?;
                let ident: syn::Ident = input.parse()?;
                if ident == "section" {
                    input.parse::<Token![=]>()?;
                    section = Some(input.parse::<LitStr>()?);
                } else {
                    return Err(syn::Error::new(ident.span(), "expected `.section=`"));
                }
            }
        }
        Ok(ExpectInput { expected, section })
    }
}

/// Embed expected trace output in the `.emt_exp` ELF section.
///
/// Used by end-to-end tests to validate trace output.
///
/// ```rust,ignore
/// expect!("expected output\n");
/// expect!("expected output\n", .section=".custom_section");
/// ```
#[proc_macro]
pub fn expect(input: TokenStream) -> TokenStream {
    let ExpectInput { expected, section } = parse_macro_input!(input as ExpectInput);
    let section_name = section
        .map(|l| l.value())
        .unwrap_or_else(|| ".emt_exp".to_string());
    let section_lit = LitStr::new(&section_name, Span::call_site());
    let val = expected.value();
    let bytes: Vec<u8> = val.bytes().collect();
    let len = bytes.len();
    let byte_lits: Vec<_> = bytes
        .iter()
        .map(|b| proc_macro2::Literal::u8_suffixed(*b))
        .collect();

    quote! {
        {
            #[unsafe(link_section = #section_lit)]
            #[unsafe(no_mangle)]
            static EXPECTED_OUTPUT: [u8; #len] = [#(#byte_lits),*];
            let _ = unsafe { ::core::ptr::read_volatile(&EXPECTED_OUTPUT) };
        }
    }
    .into()
}

// ──────────────────────────────────────────────────────────────────────────────
// `emtrace_init!`
// ──────────────────────────────────────────────────────────────────────────────

struct InitInput {
    sink: Option<Expr>,
    sink_guard: Option<Expr>,
    cobs: bool,
}

impl Parse for InitInput {
    fn parse(input: ParseStream) -> syn::Result<Self> {
        let mut sink = None;
        let mut sink_guard = None;
        let mut cobs = false;

        let mut first = true;
        while !input.is_empty() {
            if !first {
                input.parse::<Token![,]>()?;
                if input.is_empty() {
                    break;
                }
            }
            first = false;

            if input.peek(Token![.]) {
                input.parse::<Token![.]>()?;
                let ident: syn::Ident = input.parse()?;
                match ident.to_string().as_str() {
                    "sink" => {
                        input.parse::<Token![=]>()?;
                        sink = Some(input.parse::<Expr>()?);
                    }
                    "sink_guard" => {
                        input.parse::<Token![=]>()?;
                        sink_guard = Some(input.parse::<Expr>()?);
                    }
                    "cobs" => {
                        input.parse::<Token![=]>()?;
                        let lit: syn::LitBool = input.parse()?;
                        cobs = lit.value();
                    }
                    other => {
                        return Err(syn::Error::new(
                            ident.span(),
                            format!("unknown option `.{other}`"),
                        ));
                    }
                }
            } else {
                return Err(input.error("expected an option like `.sink=...`"));
            }
        }

        Ok(InitInput {
            sink,
            sink_guard,
            cobs,
        })
    }
}

/// Initialize the emtrace output by writing the MGIC pointer to the sink.
///
/// Generates a `static MgicInfo<N>` in the `.emtrace` ELF section containing
/// the C-compatible MGIC record, then writes a `PointerT` (the shifted runtime
/// address of that static) to the sink.  The Python parser reads the pointer
/// from the stream and resolves the MGIC record via ASLR offset — exactly the
/// same protocol as the C `EMTRACE_INIT()` macro.
///
/// Must be called once before any `trace!` calls.
///
/// # Syntax
///
/// ```rust,ignore
/// emtrace_init!();                      // write to stdout
/// emtrace_init!(.sink = my_sink);       // write to a mutable-borrow sink
/// emtrace_init!(.sink_guard = guard);   // move a sink (e.g. MutexGuard)
/// emtrace_init!(.cobs = true);          // signal COBS encoding
/// ```
#[proc_macro]
pub fn emtrace_init(input: TokenStream) -> TokenStream {
    let InitInput {
        sink,
        sink_guard,
        cobs,
    } = parse_macro_input!(input as InitInput);

    let encoding_expr: TokenStream2 = if cobs {
        quote! { ::emtrace::COBS_ENCODING as usize }
    } else {
        quote! { ::emtrace::RAW_ENCODING as usize }
    };

    let sink_setup: TokenStream2;
    let sink_ref_expr: TokenStream2;

    if let Some(ref sg) = sink_guard {
        sink_setup = quote! { let mut __emtrace_sink_guard = #sg; };
        sink_ref_expr = quote! { &mut __emtrace_sink_guard };
    } else if let Some(ref s) = sink {
        sink_setup = quote! {};
        sink_ref_expr = quote! { &mut #s };
    } else {
        sink_setup = quote! { let mut __emtrace_stdout = ::std::io::stdout().lock(); };
        sink_ref_expr = quote! { &mut __emtrace_stdout };
    }

    quote! {
        {
            use ::emtrace::Sink as _;

            // Compute the encoding const at compile time
            const __ENCODING: usize = #encoding_expr;
            const __MGIC_N: usize = ::emtrace::__private::mgic_record_size();

            // Static MGIC record placed in the `.emtrace` ELF section.
            // The parser scans this section to locate the MGIC record.
            #[unsafe(link_section = ".emtrace")]
            #[used]
            static __EMTRACE_MGIC: ::emtrace::MgicInfo<__MGIC_N> =
                ::emtrace::__private::build_mgic_record::<__MGIC_N>(__ENCODING);

            #sink_setup
            let __sink = #sink_ref_expr;

            // Write the shifted address of the static as a PointerT to the stream.
            // The parser uses this pointer to locate the MGIC record in the section.
            let __mgic_addr = (__EMTRACE_MGIC.bytes.as_ptr().addr()
                >> ::emtrace::ALIGNMENT_POWER) as ::emtrace::PointerT;
            ::emtrace::write_mgic_ptr(__sink, __mgic_addr)
        }
    }
    .into()
}
