# Semantic Function Names Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Replace all 681 numbered firmware function names and their dependent identifiers with evidence-based semantic names without changing the generated firmware.

**Architecture:** Build a complete old-to-new symbol manifest from function bodies and callers, validate it for uniqueness and coverage, and apply it token-wise across sources and headers. A regression test and independent AST/source audits enforce that no numbered function family remains while numbered data symbols stay valid.

**Tech Stack:** C89-style TI firmware sources, Python `unittest`, Clang AST JSON, PlatformIO, TI ARM CGT 5.2.9, Intel HEX comparison.

## Global Constraints

- Preserve `ti_arm9_abi`, TI syntax, fixed linker symbols, flags, source order, and artifact names.
- Preserve the Windows TI CGT 5.2.5 and macOS TI CGT 5.2.9 tool resolution behavior.
- Keep warnings promoted to errors.
- Do not rename numbered state, storage, branch-target, label, or MMIO symbols.
- Do not invent protocol semantics unsupported by function bodies or call sites.
- Do not flash hardware.
- This directory has no Git metadata, so no commit step is available.

---

### Task 1: Capture the Baseline and Symbol Inventory

**Files:**
- Inspect: `src/*.c`
- Inspect: `include/*.h`
- Preserve externally: a temporary full-project source snapshot

**Interfaces:**
- Consumes: current post-local-rename firmware tree
- Produces: exact sets of numbered definitions, declarations, uses, and derived identifiers

- [x] Save a temporary snapshot of all source and header files before renaming.
- [x] Parse all 28 C files with Clang and record numbered function definitions.
- [x] Record all textual occurrences and function-derived identifier families.
- [x] Confirm the baseline contains exactly 681 numbered definitions and 131 derived identifiers.

### Task 2: Add the Regression Test

**Files:**
- Modify: `tests/test_semantic_identifiers.py`

**Interfaces:**
- Consumes: the numbered-function AST/source naming rule
- Produces: a test that fails on every current numbered function family

- [x] Add a function-name pattern that identifies callable symbols ending in decimal digits.
- [x] Add a derived-identifier check based on the captured old-function manifest.
- [x] Run `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`.
- [x] Confirm the new test fails against the pre-rename tree and reports numbered functions.

### Task 3: Build and Validate the Semantic Rename Manifest

**Files:**
- Create temporarily: `scripts/_rename_numbered_functions.py`
- Inspect: all affected `src/*.c` and `include/*.h`

**Interfaces:**
- Consumes: function bodies, callers, signatures, hardware/state usage, old symbol families
- Produces: one unique semantic name for every old function plus use-site names for derived values

- [x] Extract each function body, signature, direct callees, callers, and touched globals.
- [x] Assign names using protocol evidence, state transitions, exact operations, or structural behavior in that priority order.
- [x] Reject duplicate new names, missing old names, numbered suffixes, and collisions with existing symbols.
- [x] Validate that the manifest covers all 681 definitions.

### Task 4: Apply Symbol and Derived-Identifier Renames

**Files:**
- Modify: `src/ahci.c`
- Modify: `src/gio.c`
- Modify: `src/main.c`
- Modify: `src/mww.c`
- Modify: `src/pwm.c`
- Modify: `src/rdx_core.c`
- Modify: `src/rti.c`
- Modify: `src/sci.c`
- Modify: `src/scsi.c`
- Modify: `src/spi.c`
- Modify: `src/string.c`
- Modify: `src/ums_bot.c`
- Modify: `src/usb_hal.c`
- Modify: `src/usb_hal_isr.c`
- Modify: `src/usb_stack.c`
- Modify: `src/vim_intvecs.c`
- Modify: `src/vim_nvic.c`
- Modify: `include/ahci.h`
- Modify: `include/gio.h`
- Modify: `include/mww.h`
- Modify: `include/rdx_core.h`
- Modify: `include/rti.h`
- Modify: `include/sci.h`
- Modify: `include/spi.h`
- Modify: `include/string.h`
- Modify: `include/usb_hal.h`
- Modify: `include/usb_stack.h`
- Modify: `include/vim_nvic.h`

**Interfaces:**
- Consumes: validated rename manifest
- Produces: consistently renamed definitions, declarations, calls, casts, comments, and result variables

- [x] Apply longest-token-first replacements to function symbols and dependent identifiers.
- [x] Update Doxygen summaries where the firmware behavior supports a clearer description.
- [x] Parse every changed C source with Clang after each subsystem batch.
- [x] Delete the temporary rename helper after the mapping is fully applied.

### Task 5: Verify Coverage and Firmware Equivalence

**Files:**
- Test: `tests/test_semantic_identifiers.py`
- Verify: `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex`

**Interfaces:**
- Consumes: renamed source tree and saved baseline
- Produces: evidence that naming changed and firmware bytes did not

- [x] Run the semantic identifier test and confirm it passes.
- [x] Parse all 28 C sources and confirm zero numbered function definitions.
- [x] Search all sources and headers for every removed function-family token.
- [x] Run `~/.platformio/penv/bin/python -m unittest discover -s tests -v` and confirm zero failures.
- [x] Run a clean `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt` build.
- [x] Build the saved pre-rename snapshot with the same environment.
- [x] Compare both Intel HEX files with `cmp` and SHA-256.
- [x] Confirm numbered data symbols remain present and no hardware operation occurred.
