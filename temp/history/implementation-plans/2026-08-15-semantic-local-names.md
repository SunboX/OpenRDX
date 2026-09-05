# Semantic Local Names Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace placeholder-style locals and generic parameters in every affected C source with function-specific names supported by code usage.

**Architecture:** Work one function scope at a time so identical placeholder spellings in unrelated functions never share a mapping. Add a source-level regression test, then rename from small hardware-support modules through AHCI/MWW and finally `rdx_core.c`, checking syntax-sensitive identifier consistency after every group.

**Tech Stack:** TI ARM Code Generation Tools C dialect, PlatformIO/SCons, Python `unittest`, regular-expression source checks.

## Global Constraints

- Cover every C source under `src/` that contains placeholder-style local names.
- Derive names from complete definition-use chains, callers, callees, register accesses, constants, and documented behavior.
- Use narrow implementation-level names when product-level meaning is not proven.
- Preserve public and internal function names, global symbols, fixed addresses, layouts, types, control flow, ABI, build flags, and artifacts.
- Do not change TI reference headers or substitute another compiler.
- This directory has no Git metadata; do not add commit steps or claim Git-diff verification.

---

### Task 1: Add the placeholder-name regression check

**Files:**
- Create: `tests/test_semantic_identifiers.py`

**Interfaces:**
- Consumes: all `src/*.c` files.
- Produces: a `unittest` failure listing every remaining placeholder-style identifier and source path.

- [x] **Step 1: Write the failing structural test**

```python
"""Guard existing firmware sources against placeholder-style local names."""

import pathlib
import re
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
PLACEHOLDER_PATTERN = re.compile(
    r"\b(?:unsigned|signed)_(?:value|pointer)_\d+\b|\btemporary_\d+\b"
)


class SemanticIdentifierTests(unittest.TestCase):
    """Verify that existing C sources retain semantic local names."""

    def test_sources_do_not_contain_placeholder_local_names(self):
        """Reject numbered type-based local identifiers in firmware sources."""
        matches = []
        for source_path in sorted((PROJECT_ROOT / "src").glob("*.c")):
            for line_number, line in enumerate(
                source_path.read_text(encoding="utf-8").splitlines(), start=1
            ):
                for match in PLACEHOLDER_PATTERN.finditer(line):
                    matches.append(
                        f"{source_path.relative_to(PROJECT_ROOT)}:{line_number}:"
                        f"{match.group(0)}"
                    )
        self.assertEqual([], matches, "\n".join(matches))


if __name__ == "__main__":
    unittest.main()
```

- [x] **Step 2: Run the test and confirm the existing sources fail**

Run: `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`

Expected: `FAIL`, with matches beginning in affected files such as `src/ahci.c`, `src/mww.c`, and `src/rdx_core.c`.

### Task 2: Rename locals in the small hardware and runtime modules

**Files:**
- Modify: `src/wdt.c`
- Modify: `src/main.c`
- Modify: `src/gio.c`
- Modify: `src/sci.c`
- Modify: `src/string.c`
- Modify: `src/ums_bot.c`
- Modify: `src/usb_hal.c`
- Modify: `src/usb_hal_isr.c`

**Interfaces:**
- Consumes: existing register macros, function calls, and K&R parameter declarations.
- Produces: identifier-only source changes such as `watchdog_preload`, `device_id`, `logical_input`, `current_byte`, `buffer_end`, `remaining_bytes`, `endpoint_index`, and `interrupt_status` where those roles are demonstrated by each function.

- [x] **Step 1: Inspect each complete function and its direct callers**

Run: `rg -n 'wdt_start|main\(|gio_|sci_read_logical_input|ti_memset|usb_mass_storage_init|usb_transfer_endpoint_bank_controller_memory_copy_context|usb_hal_isr' src include docs doc`

Expected: declarations, definitions, and call sites used to establish each local's role.

- [x] **Step 2: Apply per-function mappings**

For each declaration, trace every use before choosing the name. Keep K&R argument names identical in the function header and declaration block. Do not rename functions, globals, registers, structure fields, or constants.

- [x] **Step 3: Check the completed file group**

Run: `rg -n '\b(?:unsigned|signed)_(?:value|pointer)_[0-9]+\b|\btemporary_[0-9]+\b' src/{wdt,main,gio,sci,string,ums_bot,usb_hal,usb_hal_isr}.c`

Expected: no output.

### Task 3: Rename locals in interrupt, timing, SPI, and USB-stack modules

**Files:**
- Modify: `src/rti.c`
- Modify: `src/spi.c`
- Modify: `src/vim_intvecs.c`
- Modify: `src/usb_stack.c`

**Interfaces:**
- Consumes: RTI, SPI, VIM, USB register definitions and direct call sites.
- Produces: role names for counters, status words, descriptor pointers, endpoint numbers, interrupt masks, transfer lengths, and callback results.

- [x] **Step 1: Trace definitions and call sites**

Run: `rg -n 'rti_|spi_|ahci_rx_error_isr|rti_compare0_isr|ahci_isr|gio_isr|usb_(stack_init|update_|process_001|get_value_001)' src include docs doc`

Expected: complete direct-use context for the affected functions.

- [x] **Step 2: Rename each placeholder within its own function scope**

Use protocol- or register-specific names where constants prove them. Use neutral names such as `interrupt_status`, `register_value`, `descriptor`, or `callback_result` where the higher-level meaning is not established.

- [x] **Step 3: Check the completed file group**

Run: `rg -n '\b(?:unsigned|signed)_(?:value|pointer)_[0-9]+\b|\btemporary_[0-9]+\b' src/{rti,spi,vim_intvecs,usb_stack}.c`

Expected: no output.

### Task 4: Rename AHCI locals and parameters

**Files:**
- Modify: `src/ahci.c`

**Interfaces:**
- Consumes: AHCI register macros, command-list/table layouts, transfer APIs, and callers.
- Produces: semantic names for port status, command slots, command headers, physical-region descriptors, byte counts, and command results without changing the AHCI data path.

- [x] **Step 1: Review every affected AHCI function with call sites**

Run: `sed -n '1,680p' src/ahci.c && rg -n 'ahci_(dispatch|coordinate|init|process|transfer|update|get_value)' src include`

Expected: the complete AHCI implementation and all repository-local callers.

- [x] **Step 2: Rename placeholders and generic parameters by function**

Keep bit arithmetic and structure offsets byte-for-byte equivalent. A variable that changes roles within a function receives the narrowest accurate name covering all of its uses, such as `operation_status` rather than a guessed ATA-specific result.

- [x] **Step 3: Check AHCI**

Run: `rg -n '\b(?:unsigned|signed)_(?:value|pointer)_[0-9]+\b|\btemporary_[0-9]+\b' src/ahci.c`

Expected: no output.

### Task 5: Rename MWW locals and parameters

**Files:**
- Modify: `src/mww.c`

**Interfaces:**
- Consumes: MWW state arrays, memory-mapped window registers, DMA/USB coordination calls, and all MWW callers.
- Produces: semantic names for channel numbers, window descriptors, transfer offsets, remaining lengths, buffer addresses, status codes, and flags.

- [x] **Step 1: Review the complete module and callers**

Run: `sed -n '1,980p' src/mww.c && rg -n 'mww_(coordinate|update|process|transfer|configure|init)' src include`

Expected: the complete MWW implementation and repository-local call sites.

- [x] **Step 2: Apply scoped names and update parameter documentation**

Use MWW register/layout evidence for concrete names. Where offsets are known but field semantics are not, name them by representation and use, such as `window_entry`, `transfer_offset`, or `descriptor_flags`.

- [x] **Step 3: Check MWW**

Run: `rg -n '\b(?:unsigned|signed)_(?:value|pointer)_[0-9]+\b|\btemporary_[0-9]+\b' src/mww.c`

Expected: no output.

### Task 6: Rename `rdx_core.c` in bounded function ranges

**Files:**
- Modify: `src/rdx_core.c`

**Interfaces:**
- Consumes: the complete function body, direct call sites, `docs/RDX_SPECIFIC_FUNCTIONALITY.md`, `doc/TUSB9261_RDX_TECHNICAL_REFERENCE.md`, register macros, and persistent-record offsets.
- Produces: semantic local and parameter names throughout the existing firmware core without adding unproven product interpretations.

- [x] **Step 1: Process functions at lines 1-3850**

Trace each value within one function and rename it before moving to the next. Re-run the placeholder search limited to the processed function names after the range is complete.

- [x] **Step 2: Process functions at lines 3851-7609**

Apply the same definition-use and caller evidence. Preserve expressions and casts exactly.

- [x] **Step 3: Process functions at lines 7610-11590**

Use SCSI, ATA, USB, persistent-record, mechanism-state, temperature, PWM, and indicator terminology only when the code or project documentation identifies that role.

- [x] **Step 4: Process functions at lines 11591-end**

Prefer honest representation-level names for unresolved existing helpers. Update Doxygen parameter names and descriptions to match the renamed signatures.

- [x] **Step 5: Check the core module**

Run: `rg -n '\b(?:unsigned|signed)_(?:value|pointer)_[0-9]+\b|\btemporary_[0-9]+\b' src/rdx_core.c`

Expected: no output.

### Task 7: Audit generic parameters and document the readability invariant

**Files:**
- Modify: all affected `src/*.c` files
- Modify: `README.md`

**Interfaces:**
- Consumes: generic parameter names (`condition`, `value`, `length`, `buffer`, `flags`, `offset`, `index`, `context`) and their full function usage.
- Produces: concrete parameter names wherever the role is established, matching K&R declarations and Doxygen `@param` entries; a README note explaining the evidence-bound naming rule.

- [x] **Step 1: Inventory generic parameters**

Run: `rg -n '^.*\(([^)]*\b(condition|value|length|buffer|flags|offset|index|context)\b[^)]*)\)$|^\s*\* @param (condition|value|length|buffer|flags|offset|index|context)\b' src`

Expected: a review list of signatures and documentation, not an instruction to rename unrelated established identifiers blindly.

- [x] **Step 2: Rename parameters supported by their complete usage**

For K&R functions, update the function header, declaration block, body, and Doxygen tag together. Keep an established generic term only when it is genuinely the clearest correct concept, such as a byte `length` used solely as a length.

- [x] **Step 3: Add the README recovery-source naming note**

Add this text in the source-layout guidance section:

```markdown
Existing local identifiers are named from their function-level data flow,
register use, protocol constants, and call sites. When the product-level
meaning is not proven, the source uses a narrower implementation-level name
instead of a speculative RDX label.
```

### Task 8: Verify the complete rename

**Files:**
- Test: `tests/test_semantic_identifiers.py`
- Test: `tests/test_ti_cgt_tools.py`

**Interfaces:**
- Consumes: all edited sources and the configured TI CGT resolver.
- Produces: structural-test, unit-test, and build evidence.

- [x] **Step 1: Run the placeholder regression test**

Run: `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v`

Expected: `OK` with one passing test.

- [x] **Step 2: Run the complete Python unit suite**

Run: `~/.platformio/penv/bin/python -m unittest discover -s tests -v`

Expected: all tests pass.

- [x] **Step 3: Run the PlatformIO TI CGT build**

Run: `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`

Expected: `SUCCESS` when `~/ti/ti-cgt-arm_5.2.9/bin/armcl` and `armhex` are available. If unavailable, capture and report the exact missing resolved path without changing compilers.

- [x] **Step 4: Perform final source searches**

Run: `rg -n '\b(?:unsigned|signed)_(?:value|pointer)_[0-9]+\b|\btemporary_[0-9]+\b' src`

Expected: no output.

Run: `rg -n 'TBD|TODO|unsigned_value|unsigned_pointer|signed_value|signed_pointer|temporary_[0-9]+' README.md docs/superpowers/specs/2026-08-15-semantic-local-names-design.md src tests`

Expected: only intentional explanatory/test-pattern occurrences outside firmware source, with no incomplete work markers.
