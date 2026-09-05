# Semantic Suffix Names Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace all artificial `_3` local-variable and parameter names with function-specific semantic names while preserving genuine numbered firmware symbols.

**Architecture:** Extend the source guard with the exact artificial name families, then process the three affected source files one function at a time. Preserve every expression and validate behavior with a clean TI build and a byte-identical Intel HEX comparison.

**Tech Stack:** TI ARM CGT C dialect, PlatformIO/SCons, Python `unittest`, source-level regular-expression checks.

## Global Constraints

- Rename all 68 current artificial `_3` declaration sites in `src/ahci.c`, `src/mww.c`, and `src/rdx_core.c`.
- Include `condition_3`, `buffer_3`, and `offset_3` parameters.
- Preserve numbered functions, globals, labels, registers, and storage symbols.
- Do not alter types, expressions, control flow, layouts, ABI, addresses, flags, or artifacts.
- Use product-level terminology only when directly supported by code or existing documentation.
- This directory has no Git metadata; do not add commit or branch steps.

---

### Task 1: Add the failing `_3` local-name guard

**Files:**
- Modify: `tests/test_semantic_identifiers.py`

**Interfaces:**
- Consumes: all `src/*.c` files.
- Produces: rejection of the 23 artificial `_3` identifier families without matching `core_state_003` or other established numbered symbols.

- [ ] **Step 1: Add the artificial suffix expression**

```python
ARTIFICIAL_SUFFIX_PATTERN = re.compile(
    r"\b(?:working_result|data_cursor|operation_status|status_bits|condition_met|"
    r"working_buffer|buffer(?:_[12])?_cursor|buffer(?:_[12])?_field|"
    r"condition_2_field|callback_result|context_derived|register_bits|"
    r"core_get_0xc_for_device_descriptor_response_result|core_release_record_buffer_result|core_read_big_endian_record_field_result|"
    r"core_process_remaining_descriptor_space_for_copy_copy_resource_result)_3\b|\b(?:condition_3|buffer_3|offset_3)\b"
)
```

Search each source line with both `PLACEHOLDER_PATTERN` and
`ARTIFICIAL_SUFFIX_PATTERN`, appending matches to the existing diagnostic list.

- [ ] **Step 2: Run the focused test and verify red state**

Run: `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`

Expected: `FAIL`, listing matches from `src/ahci.c`, `src/mww.c`, and `src/rdx_core.c`.

### Task 2: Rename AHCI and MWW suffix locals

**Files:**
- Modify: `src/ahci.c`
- Modify: `src/mww.c`

**Interfaces:**
- Consumes: complete affected function bodies and their direct callers.
- Produces: semantic names for AHCI request fields/status and MWW transfer state.

- [ ] **Step 1: Trace every affected function**

Run: `rg -n -C 12 '\b[A-Za-z_]\w*_3\b' src/ahci.c src/mww.c`

Expected: complete local contexts for the affected AHCI and MWW functions.

- [ ] **Step 2: Rename each identifier within its function scope**

Update the definition and every use together. Rename the `condition_3`
parameter and its Doxygen entry from its actual AHCI role. Do not modify
numbered function or global symbols.

- [ ] **Step 3: Verify the two modules are clear**

Run: `rg -n '\b(?:working_result_3|condition_3)\b' src/ahci.c src/mww.c`

Expected: no output.

### Task 3: Rename core suffix locals and parameters

**Files:**
- Modify: `src/rdx_core.c`

**Interfaces:**
- Consumes: each complete affected core function, direct call sites, register/field offsets, and existing RDX documentation.
- Produces: semantic names for all core collision-suffixed locals and generic `_3` parameters.

- [ ] **Step 1: Generate the affected-function review list**

Run: `rg -n -C 8 '\b(?:working_result|data_cursor|operation_status|status_bits|condition_met|working_buffer|buffer(?:_[12])?_cursor|buffer(?:_[12])?_field|condition_2_field|callback_result|context_derived|register_bits|core_get_0xc_for_device_descriptor_response_result|core_release_record_buffer_result|core_read_big_endian_record_field_result|core_process_remaining_descriptor_space_for_copy_copy_resource_result)_3\b|\b(?:condition_3|buffer_3|offset_3)\b' src/rdx_core.c`

Expected: every core occurrence grouped by surrounding function context.

- [ ] **Step 2: Apply per-function semantic mappings**

For each identifier, trace all assignments and uses. Prefer names tied to a
known register, buffer role, protocol field, callback, state, length, mask, or
status. When the high-level role is unresolved, use a narrow representation
name based on the exact field offset or call result rather than a number.

- [ ] **Step 3: Update parameter documentation**

For every renamed K&R parameter, update the function header, declaration,
body, and Doxygen `@param` entry together.

- [ ] **Step 4: Verify the complete suffix scan is clear**

Run the `ARTIFICIAL_SUFFIX_PATTERN` over `src/`.

Expected: no output while searches for `core_state_003` and
`core_process_clear_memory_primary_response_buffer` still return their established symbols.

### Task 4: Verify behavior and artifacts

**Files:**
- Test: `tests/test_semantic_identifiers.py`
- Test: `tests/test_ti_cgt_tools.py`

**Interfaces:**
- Consumes: all edited sources and a separately retained pre-change source snapshot.
- Produces: test, clean-build, and firmware-image equivalence evidence.

- [ ] **Step 1: Run the complete unit suite**

Run: `~/.platformio/penv/bin/python -m unittest discover -s tests -v`

Expected: all tests pass.

- [ ] **Step 2: Perform a clean configured TI build**

Run: `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt -t clean && ~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`

Expected: compile, link, and Intel HEX generation finish with `SUCCESS`.

- [ ] **Step 3: Build the pre-change snapshot separately and compare HEX hashes**

Build the retained snapshot with the same PlatformIO environment. Compare
SHA-256 hashes of both `TUSB9261_RDX.hex` files.

Expected: the hashes are identical.

- [ ] **Step 4: Re-run the complete source guard**

Run: `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`

Expected: `OK`.
