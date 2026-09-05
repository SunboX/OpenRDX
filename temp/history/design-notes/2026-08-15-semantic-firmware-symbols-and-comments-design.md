# Semantic Firmware Symbols and Explanatory Comments Design

## Goal

Replace remaining numbered firmware symbols wherever their role can
be supported by source, call-site, address, or TI reference evidence. Add
useful inline comments throughout the existing firmware so readers can follow
control flow, state transitions, memory layouts, and hardware interaction
without changing executable behavior.

## Current Inventory

The absolute-symbol interface contains 534 numbered linker bindings:

- 139 fixed-address branch or label targets;
- 58 MMIO register aliases;
- 290 state symbols;
- 34 storage symbols;
- seven firmware label-data symbols;
- six additional pointer-typed bindings whose declarations do not match the
  simple scalar declaration parser.

All 528 numbered symbols declared in `include/firmware_symbols.h` are referenced
by the sources. They contain 1,014 repeated generic
comments across approximately 23,060 C source lines. These comments mark broad
operation classes but often do not explain the specific data or state involved.

## Evidence and Confidence Rules

Every rename must be supported by one or more of the following:

1. a documented peripheral register and exact absolute address in the supplied
   TI reference headers;
2. a stable protocol field, command, response, or record role visible in reads,
   writes, sizes, constants, and callers;
3. an unambiguous control-flow role, such as a retry exit, validation failure,
   state dispatch, or shared cleanup block;
4. an exact structural role, such as a callback slot, ring cursor, byte count,
   response buffer, saved register value, or per-device flag.

An address or declaration order alone is not semantic evidence. When the exact
domain meaning cannot be proven, the symbol keeps its numbered name and is
recorded as unresolved. Comments use factual structural wording and never
promote a guess into a protocol claim.

## Rename Scope

### Functions

Audit every C and assembly callable symbol again. Any remaining numeric
discriminator is renamed from its body and callers. Literal values that are
part of an operation are expressed by their effect rather than by a numeric
suffix.

### MMIO registers

Resolve register aliases by absolute address against supplied TUSB9261, AHCI,
USB, MWW, SCI, SPI, RTI, VIM/NVIC, and GIO definitions. When no official field
name is available, use the observed access role, width, and direction. Never
infer a peripheral merely from nearby address ordering.

### State and storage

Infer roles from all reads, writes, pointer arithmetic, field offsets, buffer
sizes, functions, and subsystem call sites. Related members use a shared domain
prefix and distinct role suffixes. Packed records and arrays are named as the
whole resource rather than pretending each address is an independent scalar.

### Branch targets and label data

C labels are named after the block they enter: retry, validation failure,
fallback, dispatch case, shared cleanup, or completion. Matching absolute
linker symbols are renamed atomically. Firmware label-data symbols are named
from their consumers and represented payload; unidentified constant blobs stay
numbered.

### Atomic symbol families

A rename updates every member of its symbol family:

- C and assembly references;
- `include/firmware_symbols.h` declarations;
- `linker/firmware_absolute_symbols.cmd` bindings;
- C labels and `goto` statements;
- snapshots, pointers, indices, and other derived local identifiers;
- documentation references.

The numeric address and ABI type remain unchanged.

## Commenting Strategy

Comments are added where they reduce code-analysis effort:

- before state-machine branches, explain the state being tested and the
  transition or early exit;
- around MMIO reads and writes, identify the register role, relevant mask, and
  expected hardware effect;
- beside fixed-address pointer arithmetic, describe the base record and field
  offset without inventing an unsupported field name;
- before loops, explain the collection, bound, cursor, and termination rule;
- around callbacks and indirect calls, identify how the target is selected and
  what context is passed;
- around validation and fallback paths, state what was checked and why the
  fallback is safe;
- around byte-order, checksum, CRC, SHA, flash, and SCSI transformations,
  explain input and output representation.

Repeated comments such as "execute the procedure" or "update the register" are
replaced when specific evidence exists. Obvious assignments are not narrated.
Uncertain details use wording such as "observed", "appears to", or "role not
yet identified" only when that uncertainty is relevant to understanding the
code.

## Regression Tests

`tests/test_semantic_identifiers.py` gains three checks before implementation:

1. reject newly introduced numeric callable suffixes;
2. reject every old symbol selected for rename in sources, declarations, linker
   bindings, and derived identifiers;
3. require an explicit allowlist entry and evidence note for every numbered
   absolute symbol intentionally left unresolved.

The new tests must fail against the current tree before any production symbol
is changed.

## Verification

The completed pass must provide all of the following evidence:

1. all 28 C sources parse in freestanding GNU89 mode;
2. the full Python unit suite passes;
3. every selected old symbol family has zero remaining occurrences;
4. renamed linker bindings retain their fixed numeric addresses;
5. unresolved numbered symbols exactly match the documented allowlist;
6. a clean TI ARM CGT 5.2.9 PlatformIO build succeeds with warnings as errors;
7. the saved pre-change tree builds with the same toolchain;
8. the two Intel HEX files compare byte-for-byte and have the same SHA-256.

No upload or hardware flashing is part of this work. The directory has no Git
metadata, so no commit, branch, merge, or pull request operation is available.

## Acceptance Criteria

- Every remaining callable numeric discriminator is removed.
- Every evidence-supported numbered global, register, state, storage, label,
  and linker symbol has a unique descriptive name.
- Every unresolved numbered symbol has a concise evidence note explaining why
  it remains unresolved.
- Non-obvious firmware behavior is explained by specific inline comments
  throughout all affected subsystems.
- Fixed addresses, ABI types, source order, build flags, linker behavior, and
  generated firmware bytes remain unchanged.
