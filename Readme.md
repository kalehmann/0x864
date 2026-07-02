# 0x864 - selfh0sted.x86.4ssembler

_0x864_ is a tiny x86 assembler, that can assemble itself.

* [Description](#description)
* [Usage](#usage)
    * [Supported instructions](#supported-instructions)
    * [Creating executables](#creating-executables)
* [Development](#development)
    * [Architecture](#architecture)
    * [Building from Source](#building-from-source)
    * [Docker](#docker)
    * [Testing](#testing)

## Description

The parser and assembler of _0x864_ are themselves written in x86-64 assembly
language (Intel Syntax) and can be assembled using _0x864_.
_0x864_ supports only a small subset of the x86 ISA, but should still be usefull
enough to assemble some demos.

Refer to the [Supported instructions](#supported-instructions) section and the
source code, to get a list of supported features and their quirks.

**Milestones:**

- [`2234b86bbc`](/../../commit/2234b86bbc272bf60adbb5a9485ff68d06f07bdd):
  Assembled a relocatable elf file for hello world.
- [`f63de09031`](/../../commit/f63de090311f011ac148e7bc81742c18ed4698d5):
  Reached ability to self-host the projects assembly code.

## Usage

### Supported instructions

See [`docs/supported_instructions.md`](docs/supported_instructions.md).

### Creating executables

See [`docs/executables.md`](docs/executables.md).

## Development

### Architecture

[`src/0x864.s`](src/0x864.s) contains the mains parts of _0x864_ - namely the
parser and assembler - and is written in x86-64 assembly language.
All code in this file **does not rely** on any **ABI** - that means no OS
specific syscalls - or any **API** - so no _libc_ either.
The functions implemented in that file are declared in
[`src/0x864.h`](src/0x864.h) and follow the calling conventions of the
[System V ABI][system_v_abi].
Therefore they can be linked against and called from C code on any Unix or
Unix-like OS.

[`src/0x864.c`](src/0x864.c) contains support functions, which rely on tools from
the host platform like `calloc` and `free`.

[`src/harness.c`](src/harness.c) is the middleman between the host platform and
the assembler.
It provides command line argument parsing, file-IO and terminal output using the
host specific ABI and APIs.
That part of the application is still written in C to be somewhat portable across
Unix and Unix-like platforms and shall be kept as small as possible.

### Building from source

Clone the repository with the `--recurse-submodules` flag or update the
submodules afterwards using

```
git submodule update --init
```

Building _0x864_ then requires a C-compiler (**gcc** or **tcc** are supported),
**nasm** and **GNU Make**.
Running

```
make
```

then creates the binary at `src/0x864`.

### Docker

The repository contains a [`Dockerfile`](Dockerfile) to create a minimal docker
image for development and a [`docker-compose.yaml`](docker-compose.yaml)
configuration for common commands.

The intended usage is

```
docker compose run --rm --remove-orphans make       # Build the 0x864 assembler
docker compose run --rm --remove-orphans make tests # Execute the tests
docker compose run --rm --remove-orphans shell      # Open bash inside the docker
                                                    # image for debugging
```

### Testing

_0x864_ includes a bunch of tests to simplify development and catch bugs early
on.
The file [`tests/unit_test_suite.c`](tests/unit_test_suite.c) lists a bunch of
unit tests created with [**Acutest**][acutest].

They can are executed with

```
make test-unit
```

Furthermore the directory [`tests/demos`](tests/demos) contains a bunch of
assembly files, which are assembled with _0x864_ and **nasm** to ELF-files
and then binary-compared by running

```
make test-binary
```

The shorthand

```
make test
```

executes the unit test suite as well as the binary test suite.


  [acutest]: https://github.com/mity/acutest
  [system_v_abi]: https://refspecs.linuxbase.org/elf/x86_64-abi-0.99.pdf
