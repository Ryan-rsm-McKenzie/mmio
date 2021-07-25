# `mmio`
[![Main CI](https://img.shields.io/github/workflow/status/Ryan-rsm-McKenzie/mmio/Main%20CI?logo=github&style=flat)](https://github.com/Ryan-rsm-McKenzie/mmio/actions/workflows/main_ci.yml)
[![Codecov](https://img.shields.io/codecov/c/github/Ryan-rsm-McKenzie/mmio?logo=codecov&logoColor=white&style=flat)](https://app.codecov.io/gh/Ryan-rsm-McKenzie/mmio)

## What is `mmio`?
`mmio` is a really simple, really small library for handling memory-mapped io on windows and linux.

## Why not [`boost::iostreams::mapped_file`](https://www.boost.org/doc/libs/1_76_0/libs/iostreams/doc/classes/mapped_file.html)?
A couple reasons:
- It requires you to use Boost.
- Passing unicode paths requires you to use `boost::filesystem`, which requires you to use _even more_ of Boost, and interops pretty terribly with `std::filesystem`.

## Why not [`mio`](https://github.com/mandreyel/mio)?
Being a header only library, it includes system headers into your code, and I don't enjoy fighting tooth and nail with `windows.h` for common identifiers.
