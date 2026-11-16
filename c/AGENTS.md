# Emtrace for C / C++

Currently there is only a single implementation in C, that works in C++. I plan
on adding a dedicated C++ implementation later.

The entire implementation consists of a single
[header](./include/c/include/emtrace/emtrace.h), large parts of which (several
recursive `__VAR_ARGS__` macros) are auto-generated using a
[python script](./header-generator/build_macro.py). The non-auto-generated parts
are [here](./header-generator/emtrace_template.h).

Useful commands:

```bash
just genh    # regenerate the header file from the template and macro generator
just checkh  # check that the header file is up to date
```

## C++ implementation

The dedicated C++ header
([emtrace.hpp](./include/cxx/include/emtrace/emtrace.hpp)) is generated from
[emtrace_header.hpp](./header-generator/cxx/emtrace_header.hpp) the same way. It
is only built with `EMTRACE_ENABLE_CXX=ON`. Tests for it come in two kinds:

- Unit tests in [test/cxx](./test/cxx) (the `cxx_tests` target, run by
  `just ctest --preset <preset>`). They check the exact bytes written to a sink,
  so they don't need the parser.
- End-to-end tests in [examples/cpp](./examples/cpp) (`test_*.cpp`), which
  contain their expected output in a `.emt_exp` section, and are run through the
  parser by `just eetest`. Each is built twice, once plain and once as a
  `_cobs_cpp` variant, with the exception of tests that configure their own sink
  (`test_custom_sink`, `test_global_sink*`, `test_section`). New ones have to be
  added to `examples/CMakeLists.txt` and `tests/test_emtrace.py`.
