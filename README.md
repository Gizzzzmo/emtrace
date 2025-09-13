# Emtrace

Super fast, minimal overhead, tracing / logging library for multiple languages.
All processing of logged information (like formatting) is deferred; the
application only sends out those bytes that are relevant to what you are trying
to log. By default these bytes are written to stdout, but in general the output
channel is completely customizable.

Designed for low-latency, deeply-embedded, as well as bandwidth-constrained
tasks.

## TODO

- [x] add basic documentation
- [ ] add license
- [ ] add usage examples to documentation
- [ ] add basic tests
- [ ] push to github and add CI pipeline (github actions?)
- [ ] add dedicated C++ implementation
- [ ] add CMake profile-switching to justfile
- [ ] add versioning information
- [ ] publish rust crate to crates.io
- [ ] add nix build recipe and publish to nixpkgs
- [ ] make CMake version installable, add conan recipe, and publish to conan
  center
- [ ] add compatibility with other binary formats (mach-o and coff)
