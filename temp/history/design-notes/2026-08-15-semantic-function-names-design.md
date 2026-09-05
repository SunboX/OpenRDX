# Semantic Function Names Design

## Goal

Replace every existing function name that ends in a numeric discriminator with
a unique name derived from the function's observable behavior and usage. Rename
all declarations, call sites, casts, documentation references, and identifiers
derived from the old function name at the same time.

## Current Scope

The source tree contains 681 numbered function definitions:

- 617 in `src/rdx_core.c`
- 64 across AHCI, GIO, MWW, RTI, SCI, SPI, string, USB, and interrupt modules
- matching declarations in eleven public headers
- 131 additional identifiers derived from numbered function names, including
  result and derived-value locals

The pass covers all of them, including `ti_memset_alternate_entry_wrapper`. There are no
exceptions for numbered function symbols.

## Naming Evidence

Names are selected using the strongest available evidence, in this order:

1. protocol, subsystem, or hardware behavior visible in the body and callers;
2. a well-defined state transition or validation condition;
3. the data operation performed, such as copying words, extracting a field,
   masking a register, dispatching a callback, or saturating arithmetic;
4. an exact structural description when higher-level intent is not proven.

Names must not assert an RDX command, device state, record type, or protocol role
that is not supported by code or call-site evidence. Structural names are
preferred over guesses.

## Symbol-Family Renaming

Each function and its dependent identifiers form one rename family. For example,
renaming a function also renames forms such as:

- `<old_function>_result`
- `<old_function>_derived`
- `<old_function>_result_2`

The new dependent name describes the value represented at that use site rather
than mechanically appending the old suffix. A function used through a cast or as
a callback target is renamed exactly like a direct call.

## Exclusions

This pass does not rename numbered firmware state, storage, branch targets,
labels, or MMIO register symbols. Those names identify fixed-address data and
fixed firmware resources rather than callable behavior. Existing ABI, calling
conventions, source order, compiler flags, linker files, fixed absolute symbols,
and artifact names remain unchanged.

## Safety and Verification

A source regression test rejects:

- function declarations or definitions whose names end in decimal digits;
- references to the complete set of removed function symbols;
- identifiers derived from a removed numbered function symbol.

Verification consists of:

1. a Clang AST inventory proving 681 numbered definitions before and zero after;
2. source scans proving no old function family remains;
3. all Python unit tests;
4. a clean TI ARM CGT 5.2.9 PlatformIO build with warnings as errors;
5. a clean build from the saved pre-rename source snapshot;
6. byte-for-byte comparison of the generated Intel HEX artifacts.

No firmware flashing is part of this work.

## Acceptance Criteria

- Every numbered function definition has a unique evidence-based name.
- Headers and all call sites use the new names.
- Function-derived result variables use semantic names.
- No numbered function declaration remains in the Clang AST.
- Numbered data symbols remain untouched.
- The full test suite and TI build succeed.
- The renamed and pre-rename HEX files are byte-identical.
