# Eta Compiler

Chenwei Hou (ch2352), Thomas Lu (cl2625), Steven Yu (ssy38)

## Overview

This repository contains a C++23 Eta/Rho compiler. The compiler frontend, IR
passes, assembly generation, register allocation, and driver live under `src/`
and `include/`. Third-party source dependencies are kept under `libs/`, and the
Eta runtime is under `runtime/`.

The old Docker-based workflow is deprecated. Build and test this project
directly from the repository root.

## Project Layout

- `include/` - compiler headers
- `src/` - compiler implementation and generated ANTLR sources
- `libs/` - vendored third-party libraries
- `runtime/` - Eta runtime library and linker support
- `tests/` - all current test cases and `ethScript` files
- `tools/` - ANTLR and IR interpreter jars

## Prerequisites

- CMake 3.25 or newer
- Ninja, or another CMake-supported build tool
- A C++23-capable compiler
- Java, for regenerating ANTLR sources and running `--irrun`
- GCC/binutils, for linking generated assembly with the runtime
- `eth`, for running the test harness scripts

## Build

From the repository root:

```sh
bash etac-build
```

This script regenerates the ANTLR lexer/parser sources, configures CMake,
builds the compiler, builds the Eta runtime, and copies the resulting `etac`
binary to `./etac`.

## Running

Basic form:

```sh
./etac [options] <source files>
```

For a normal compile, `etac` now behaves like a C compiler: it generates
assembly internally, links it against the Eta runtime, and writes an executable
binary. If `-d <dir>` is provided and `-o` is not, the default executable is
`<dir>/binary`.

Example:

```sh
./etac \
  -sourcepath tests/pa6/extra_testcases \
  -libpath tests/pa6/extra_testcases \
  -d output \
  sort.eta

./output/binary
```

Use `-o` to choose a binary path:

```sh
./etac -sourcepath tests/pa6/extra_testcases \
  -libpath tests/pa6/extra_testcases \
  -d output \
  -o output/sort \
  sort.eta
```

Use `--asmgen` to preserve the old assembly-only behavior:

```sh
./etac --asmgen \
  -sourcepath tests/pa6/extra_testcases \
  -libpath tests/pa6/extra_testcases \
  -d output \
  sort.eta
```

This writes `output/sort.s` and does not link a binary.

Diagnostic modes still write their corresponding files:

```sh
./etac --lex       -D diagnose tests/pa1/add.eta
./etac --parse     -D diagnose tests/pa2/programs/test1.eta
./etac --typecheck -D diagnose -libpath tests/pa3 tests/pa3/programs/test1.eta
./etac --irgen     -D diagnose -libpath tests/pa4 tests/pa4/programs/primes.eta
```

If you use `--irrun`, run `./etac` from the repository root so the interpreter
jar path resolves correctly.

## Testing

All active test cases are in `tests/`. Each assignment directory has an
`ethScript` file that can be run from the repository root.

```sh
eth -compilerpath . -testpath ./tests/pa1 ./tests/pa1/ethScript
eth -compilerpath . -testpath ./tests/pa2 ./tests/pa2/ethScript
eth -compilerpath . -testpath ./tests/pa3 ./tests/pa3/ethScript
eth -compilerpath . -testpath ./tests/pa4 ./tests/pa4/ethScript
eth -compilerpath . -testpath ./tests/pa5 ./tests/pa5/ethScript
eth -compilerpath . -testpath ./tests/pa6 ./tests/pa6/ethScript
```

The `tests/provided/` tree contains the provided tests. Run them the same way:

```sh
eth -compilerpath . -testpath ./tests/provided/pa6 ./tests/provided/pa6/ethScript
```

For a quick direct integration check without `eth`, run:

```sh
./run_single.sh
```

That script compiles `tests/pa6/extra_testcases/sort.eta` directly to
`output/binary` and executes it.
