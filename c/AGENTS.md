# Emtrace for C / C++

Currently there is only a single implementation in C, that works in C++. I plan on adding a
dedicated C++ implementation later.

The entire implementation consists of a single [header](./include/c/include/emtrace/emtrace.h),
large parts of which (several recursive `__VAR_ARGS__` macros) are auto-generated using a
[python script](./header-generator/build_macro.py). The non-auto-generated parts are
[here](./header-generator/emtrace_template.h).

Useful commands:

```bash
just genh    # regenerate the header file from the template and macro generator
just checkh  # check that the header file is up to date
```
