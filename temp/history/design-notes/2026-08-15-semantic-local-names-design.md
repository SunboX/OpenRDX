# Semantic local names design

## Goal

Replace placeholder-style local identifiers and generic parameters in `src/`
with names derived from each value's behavior, callers, constants, register
accesses, and surrounding firmware state.

Examples of names being removed include `unsigned_value_1`,
`unsigned_pointer_1`, `signed_value_1`, and `temporary_1`. The replacement for
each occurrence is function-specific; the integer type alone never determines
the name.

## Scope

- Cover every C source under `src/` that contains placeholder-style local names.
- Rename generic parameters such as `condition`, `value`, `length`, `buffer`,
  and `flags` when their role can be established from the implementation and
  call sites.
- Improve associated Doxygen parameter text when a parameter is renamed.
- Preserve public and internal function names, global symbols, memory-mapped
  register names, fixed addresses, data layouts, types, control flow, ABI,
  compiler and linker flags, and artifact names.
- Do not modify TI reference headers or introduce inferred product behavior.

## Naming method

Analyze one function at a time. Determine a variable's role using its complete
definition-use chain, callees and callers, register or structure offsets,
protocol constants, and documented firmware behavior. Prefer a concrete domain
name such as `watchdog_preload`, `device_id`, `remaining_bytes`,
`command_status`, or `gpio_input_register`.

When the precise product-level meaning is not proven, use the narrowest honest
implementation-level name supported by the code, such as `register_value`,
`table_entry`, `byte_offset`, `callback_result`, or `transfer_descriptor`.
Names must not claim unverified RDX behavior.

Each placeholder identifier is scoped to its function. Renames therefore use
per-function mappings rather than repository-wide substitutions.

## Safety and verification

The change is intended to be identifier-only apart from matching comments and
Doxygen parameter names. Review the resulting source for remaining placeholder
patterns and for accidental symbol or expression changes.

Run the Python unit suite, then run the PlatformIO TI CGT build. If the required
compiler is unavailable, report the exact resolved path and do not substitute
another compiler. Because this directory has no Git metadata, verification
must not rely on a Git diff or commit.

## Success criteria

- No `unsigned_value_N`, `unsigned_pointer_N`, `signed_value_N`,
  `signed_pointer_N`, or `temporary_N` local identifiers remain in `src/`.
- Generic parameters are renamed wherever their implementation establishes a
  meaningful role.
- Every replacement is consistent within its function and supported by code
  usage.
- Unit tests pass and the available build verification is reported exactly.
