# Complete semantic local names design

## Goal

Replace every artificial numeric suffix on a C local variable or function
parameter with a name derived from that identifier's complete function-level
data flow.

## Inventory and scope

Clang AST inspection of all 28 C translation units identifies 492 affected
declarations in 189 functions. They occur only in:

- `src/ahci.c`
- `src/mww.c`
- `src/rdx_core.c`
- `src/spi.c`

The scope includes generic parameters such as `buffer_1`, collision names such
as `working_result_2` and `operation_status_4`, derived result names such as
`core_release_record_buffer_result_2`, offset-derived locals such as `offset_field_6`, and
remaining numbered stack names such as `iStack_18`.

The scope excludes functions, globals, labels, MMIO registers, state objects,
and storage objects whose three-digit number is part of the established
existing firmware interface. Examples that must remain unchanged include
`core_process_clear_memory_primary_response_buffer`, `core_state_003`, `core_mmio_register_014`, and
`spi_storage_003`. The non-local helper function `ti_memset_alternate_entry_wrapper`
also remains unchanged because this task is limited to variables and
parameters.

## Naming method

Analyze each affected function independently. Trace assignments, reads,
pointer arithmetic, constants, structure offsets, call arguments, result use,
and return paths before choosing a replacement. Prefer protocol or hardware
terminology only when the code or existing documentation supports it.
Otherwise use a precise implementation-level role such as
`remaining_transfer_bytes`, `descriptor_cursor`, `secondary_copy_status`, or
`command_byte_six`.

No replacement may be produced by merely deleting the suffix, converting the
number to a word, or assigning a new numeric suffix. A former identifier may
receive different names in different functions when its role differs.

## Source guard

Extend the portable Python source guard to reject every C identifier ending in
a one- or two-digit numeric suffix and the two inventoried
`offset_field_100` locals. The only allowed short-suffix token is the
established non-local function `ti_memset_alternate_entry_wrapper`. Other three-digit
firmware symbols remain accepted.

The test must first fail against the current sources and then pass only after
the complete rename set is applied.

## Safety and verification

- Preserve types, qualifiers, expressions, control flow, declaration order,
  volatile stack layout, ABI, fixed addresses, compiler flags, linker flags,
  and artifact names.
- Update K&R function headers, parameter declarations, uses, and Doxygen
  `@param` entries together.
- Do not flash or upload firmware.
- Run the complete Python unit suite.
- Perform a clean TI ARM CGT 5.2.9 PlatformIO build.
- Build a separately retained pre-change source snapshot with the same
  environment and require byte-identical Intel HEX output.

## Success criteria

- All 492 inventoried numbered locals and parameters have semantic names.
- No one- or two-digit suffix remains on a variable or parameter.
- Established numbered firmware symbols are unchanged.
- The source guard and complete unit suite pass.
- The clean TI build succeeds and the final Intel HEX is byte-identical to the
  pre-change snapshot.
