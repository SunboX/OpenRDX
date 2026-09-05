# Semantic suffix names design

## Goal

Replace artificial local-variable and parameter names ending in `_3` with
names derived from their complete function-level data flow.

## Scope

- Rename every current local variable and function parameter whose `_3` suffix
  merely distinguishes it from another local.
- Include generic parameters such as `condition_3`, `buffer_3`, and `offset_3`.
- Preserve numbered functions, globals, labels, registers, and storage symbols
  such as `core_process_clear_memory_primary_response_buffer`, `core_state_003`, and `spi_storage_003`.
- Preserve types, expressions, control flow, memory layouts, ABI, fixed
  addresses, compiler and linker options, and artifact names.
- Update matching Doxygen parameter names and descriptions.

## Naming method

Analyze each affected function independently. Trace assignments, reads,
call arguments, return paths, pointer arithmetic, register accesses, field
offsets, and constants. Choose the narrowest supported role name. Protocol- or
product-level names require direct evidence; otherwise use an honest
implementation-level role such as `sense_code`, `descriptor_cursor`,
`response_length`, or `secondary_status`.

No local rename may be produced by simply removing `_3` or replacing it with
another numeric suffix.

## Regression coverage

Extend the semantic-identifier test to reject the complete set of artificial
`_3` local and parameter families found in this checkout. The test must not
reject established numbered firmware symbols.

## Verification

Run the complete Python unit suite, perform a clean PlatformIO TI CGT build,
and compare the resulting Intel HEX with a separately built snapshot from
before this rename. The HEX must remain byte-identical. This directory has no
Git metadata, so no commit, branch, or Git-diff claim is possible.

## Success criteria

- All 68 identified artificial `_3` declaration sites have semantic names.
- No affected artificial `_3` identifier remains in `src/`.
- Established numbered firmware symbols remain unchanged.
- Unit tests pass, the clean TI build succeeds, and the Intel HEX matches the
  pre-change snapshot byte for byte.
