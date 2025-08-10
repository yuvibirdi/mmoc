# MMOC Test Suite

## Structure

Organized by feature area:
- `Arithmetic/`
- `Arrays/`
- `Basic/`
- `C11/`
- `ControlFlow/`
- `Functions/`
- `Operators/`
- `Pointers/`
- `Preprocessor/`
- `Variables/`
- Top-level feature progression tests (for_loop*, ternary*, temp_test.c while under development)

## Directives
// RUN: command
// XFAIL: * (expected failure placeholder)
// UNSUPPORTED: platform

## Substitutions
- `%ccomp` compiler path
- `%s` source file
- `%t` temp output file
- `%T` temp directory

## Running
```
python3 tests/test_runner.py            # all
python3 tests/test_runner.py -f Pointers # filtered
python3 tests/test_runner.py -c ./build/ccomp
```

## Current Status
All listed tests pass (34/34). While/for/if constructs, _Bool, pointers (including double pointer), arrays, ternary, and preprocessor macros implemented.

No current XFAIL entries.
