# Complete Semantic Local Names Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace all 492 artificial numeric-suffix local variables and parameters with function-specific semantic names without changing firmware bytes.

**Architecture:** Use the Clang AST inventory as the declaration boundary and a portable token-level regression guard as the lasting test. Apply token-only renames within individual function bodies, update matching Doxygen parameter tags, and prove equivalence with separate TI CGT builds of the pre-change and final sources.

**Tech Stack:** TI ARM CGT C dialect, PlatformIO/SCons, Clang AST diagnostics, Python `unittest`, SHA-256 and byte comparison.

## Global Constraints

- Rename all 492 current artificial numbered local and parameter declarations in `src/ahci.c`, `src/mww.c`, `src/rdx_core.c`, and `src/spi.c`.
- Preserve established numbered functions, globals, labels, MMIO registers, state objects, and storage objects.
- Preserve the non-local helper function `ti_memset_alternate_entry_wrapper`.
- Do not alter types, qualifiers, expressions, control flow, declaration order, layouts, ABI, addresses, flags, or artifact names.
- Base each replacement on complete function-level usage; never mechanically strip or replace the number.
- Update all affected K&R declarations and Doxygen parameter tags.
- This directory has no Git metadata, so do not create commits or branches.

---

### Task 1: Add the complete failing numbered-local guard

**Files:**
- Modify: `tests/test_semantic_identifiers.py`

**Interfaces:**
- Consumes: every line in `src/*.c`.
- Produces: a regression failure for every identifier ending in `_1` through `_99` and `offset_field_100`, except `ti_memset_alternate_entry_wrapper`.

- [ ] **Step 1: Replace the curated suffix-family expression**

Define `NUMBERED_LOCAL_PATTERN` to match
`[A-Za-z_][A-Za-z0-9_]*_[0-9]{1,2}` or the exact `offset_field_100` token, and define
`ALLOWED_NUMBERED_IDENTIFIERS = {"ti_memset_alternate_entry_wrapper"}`. Continue using
the existing placeholder expression and add only non-allowed numeric matches
to the diagnostic list.

- [ ] **Step 2: Run the focused test and verify the red state**

Run: `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`

Expected: `FAIL` with matches in all four inventoried source files.

### Task 2: Rename AHCI, MWW, and SPI declarations

**Files:**
- Modify: `src/ahci.c`
- Modify: `src/mww.c`
- Modify: `src/spi.c`

**Interfaces:**
- Consumes: the complete affected function bodies and their direct call sites.
- Produces: semantic token-only replacements for 38 inventoried declarations.

- [ ] **Step 1: Review each affected function's full data flow**

Use the AST inventory to group declarations by function, then inspect every
assignment and use before recording the function-scoped mapping.

- [ ] **Step 2: Apply function-scoped token replacements**

Replace each declaration and all uses within that function. Update affected
function headers and Doxygen tags without modifying expressions or declaration
order.

- [ ] **Step 3: Run the focused source guard**

Run: `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`

Expected: remaining failures come only from `src/rdx_core.c`.

### Task 3: Rename all firmware-core declarations

**Files:**
- Modify: `src/rdx_core.c`

**Interfaces:**
- Consumes: 454 inventoried declarations across 173 core functions.
- Produces: semantic token-only replacements with no one- or two-digit local suffixes.

- [ ] **Step 1: Process coordinate and configuration functions**

Trace and rename every affected declaration in `core_coordinate_*` and
`core_configure_*` functions, updating all parameter documentation.

- [ ] **Step 2: Process transfer and forwarding functions**

Trace and rename every affected declaration in `core_transfer_*` and
`core_forward_*` functions, preserving pointer direction, byte counts, and
MMIO semantics.

- [ ] **Step 3: Process update and state-update functions**

Trace and rename every affected declaration in `core_update_*` and
`core_update_state_*` functions, preserving volatile declaration order and
implicit packed stack layouts.

- [ ] **Step 4: Process process, get-value, run, and remaining functions**

Trace and rename every affected declaration in all remaining inventoried
functions, including raw numbered stack identifiers.

- [ ] **Step 5: Verify the comprehensive source guard is green**

Run: `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`

Expected: `OK` and an independent AST inventory reports zero numbered local or
parameter declarations.

### Task 4: Verify behavior and binary equivalence

**Files:**
- Test: `tests/test_semantic_identifiers.py`
- Test: `tests/test_ti_cgt_tools.py`

**Interfaces:**
- Consumes: the final sources and a separately retained pre-change snapshot.
- Produces: source-audit, unit-test, clean-build, and Intel HEX equivalence evidence.

- [ ] **Step 1: Run the complete unit suite**

Run: `~/.platformio/penv/bin/python -m unittest discover -s tests -v`

Expected: all tests pass.

- [ ] **Step 2: Perform a clean configured TI build**

Run: `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt -t clean && ~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`

Expected: compile, link, and Intel HEX generation finish with `SUCCESS`.

- [ ] **Step 3: Build the retained pre-change snapshot separately**

Copy the checkout to an ASCII temporary path, restore the four retained
pre-change source files, and build environment `tusb9261_ti_cgt` there.

Expected: the baseline build finishes with `SUCCESS`.

- [ ] **Step 4: Compare final and baseline images**

Run `cmp` and SHA-256 over both `TUSB9261_RDX.hex` files.

Expected: `cmp` succeeds and both hashes are identical.
