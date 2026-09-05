# Semantic Firmware Symbols and Explanatory Comments Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Rename every evidence-supported numbered firmware symbol and explain non-obvious firmware behavior with specific inline comments without changing generated firmware bytes.

**Architecture:** Treat each absolute symbol as an atomic family spanning source references, declarations, linker bindings, derived locals, and documentation. Keep a machine-readable evidence manifest containing every numbered binding, its unchanged address, and either a semantic replacement or an unresolved reason. Apply reviewed subsystem batches, then prove address and binary equivalence against a saved pre-change snapshot.

**Tech Stack:** Existing GNU89/TI C and TI assembly, JSON evidence manifest, Python `unittest`, Clang AST JSON, PlatformIO, TI ARM CGT 5.2.9, TI linker command syntax, Intel HEX comparison.

## Global Constraints

- Preserve `ti_arm9_abi`, TI C/assembly syntax, fixed absolute addresses, source order, flags, warnings-as-errors, and artifact names.
- Preserve Windows TI CGT 5.2.5 and macOS TI CGT 5.2.9 tool resolution behavior.
- Use only source, call-site, address, and supplied TI reference evidence; do not invent protocol or hardware meanings.
- Keep unresolved numbered symbols only with a concise evidence note.
- Replace generic comments with specific explanations; do not narrate obvious assignments.
- Do not flash hardware.
- This directory has no Git metadata, so snapshot and artifact comparisons replace commit steps.

---

### Task 1: Capture the Baseline and Complete Symbol Inventory

**Files:**
- Inspect: `src/*.c`
- Inspect: `src/*.asm`
- Inspect: `include/firmware_symbols.h`
- Inspect: `include/ti_reference/*.h`
- Inspect: `linker/firmware_absolute_symbols.cmd`
- Create: `tests/fixtures/firmware_symbol_manifest.json`

**Interfaces:**
- Consumes: all current numbered declarations, bindings, labels, and uses
- Produces: a 534-entry manifest keyed by old symbol name with `address`, `category`, `new_name`, `evidence`, and `confidence`

- [x] Save a full temporary project snapshot outside the workspace and record its path.
- [x] Parse all linker assignments matching `^_<name> = <address>;` and all matching C declarations and uses.
- [x] Record each symbol category and every source location that reads, writes, takes its address, uses it as a label, or derives a local name from it.
- [x] Populate the manifest with all 534 numbered linker bindings; leave `new_name` empty until evidence review.
- [x] Validate that every manifest address exactly matches the linker command file and every source-declared numbered symbol is represented.

### Task 2: Add Failing Naming, Coverage, and Comment Tests

**Files:**
- Modify: `tests/test_semantic_identifiers.py`
- Test: `tests/fixtures/firmware_symbol_manifest.json`

**Interfaces:**
- Consumes: the complete manifest
- Produces: regression gates for atomic rename coverage, unresolved evidence, numeric callable suffixes, and generic comments

- [x] Add a test that requires every numbered linker binding to have either a non-numbered `new_name` plus evidence or an unresolved reason.
- [x] Add a test that rejects each manifest `old_name` with a replacement anywhere under `src/`, `include/`, `linker/`, `README.md`, or `docs/` except historical design records.
- [x] Add a test that verifies every replacement retains the fixed numeric linker address.
- [x] Add a test that rejects the six repeated generic comment templates currently used by the existing sources.
- [x] Run `~/.platformio/penv/bin/python -m unittest tests.test_semantic_identifiers -v` and confirm failure reports incomplete manifest entries and generic comments.

### Task 3: Resolve MMIO Register Names

**Files:**
- Modify: `tests/fixtures/firmware_symbol_manifest.json`
- Modify: `include/firmware_symbols.h`
- Modify: `linker/firmware_absolute_symbols.cmd`
- Modify: `src/ahci.c`
- Modify: `src/c_int00.c`
- Modify: `src/mww.c`
- Modify: `src/rdx_core.c`
- Modify: `src/usb_hal.c`
- Modify: `src/usb_hal_isr.c`
- Modify: `src/usb_stack.c`
- Modify: `src/vim_intvecs.c`
- Modify: `src/vim_nvic.c`

**Interfaces:**
- Consumes: 58 numbered MMIO bindings and supplied reference-register definitions
- Produces: exact peripheral register names or explicitly unresolved register aliases

- [x] Match each numbered MMIO address against supplied TI register definitions and existing named absolute symbols.
- [x] Use read/write masks and callers to distinguish registers sharing a peripheral block.
- [x] Record the evidence and confidence for every MMIO decision.
- [x] Apply each approved register rename atomically to declarations, bindings, source uses, and derived snapshot identifiers.
- [x] Parse affected sources and run the semantic tests.

### Task 4: Resolve State, Storage, Callback, and Label-Data Names by Subsystem

**Files:**
- Modify: `tests/fixtures/firmware_symbol_manifest.json`
- Modify: `include/firmware_symbols.h`
- Modify: `linker/firmware_absolute_symbols.cmd`
- Modify: `src/ahci.c`
- Modify: `src/mww.c`
- Modify: `src/rdx_core.c`
- Modify: `src/rti.c`
- Modify: `src/scsi.c`
- Modify: `src/spi.c`
- Modify: `src/ums_bot.c`
- Modify: `src/usb_hal.c`
- Modify: `src/usb_hal_isr.c`
- Modify: `src/usb_stack.c`
- Modify: `src/c_int00.c`
- Modify: `src/gio.c`
- Modify: `src/main.c`
- Modify: `src/one_touch.c`
- Modify: `src/pwm.c`
- Modify: `src/reg_io.c`
- Modify: `src/scsi_data.c`
- Modify: `src/string.c`
- Modify: `src/system.c`
- Modify: `src/system_init.c`
- Modify: `src/ums_uas.c`
- Modify: `src/usb_chap9.c`
- Modify: `src/usb_hid.c`
- Modify: `src/usb_vendor.c`
- Modify: `src/vim_intvecs.c`
- Modify: `src/vim_nvic.c`
- Modify: `src/wdt.c`

**Interfaces:**
- Consumes: 290 state, 34 storage, seven label-data, and pointer/callback bindings
- Produces: semantic state and storage names grounded in all reads, writes, sizes, offsets, and callers

- [x] Analyze AHCI symbols as a group and name device records, command/status words, port registers, polling storage, and queue state.
- [x] Analyze MWW symbols as a group and name descriptor rings, transfer cursors, channel state, saved registers, and address registers.
- [x] Analyze USB symbols as a group and name controller state, endpoint data, event queues, reset state, callbacks, and descriptor storage.
- [x] Analyze RTI, SCI, SPI, interrupt, and SCSI symbols as groups and name timers, watchdog state, pin callbacks, transfer buffers, and protocol records.
- [x] Analyze core symbols by coherent records and state machines rather than declaration order; identify persistent identity data, SCSI response state, cartridge/mechanism state, thermal monitoring, ATA command state, flash state, and callback tables.
- [x] Name each firmware label-data symbol from its consumer and payload when proven; otherwise record it unresolved.
- [x] Apply reviewed names atomically and rename derived pointers, snapshots, counters, and result variables.
- [x] Run Clang parsing and semantic tests after every subsystem batch.

### Task 5: Rename Fixed-Address Branch and Label Targets

**Files:**
- Modify: `tests/fixtures/firmware_symbol_manifest.json`
- Modify: `include/firmware_symbols.h`
- Modify: `linker/firmware_absolute_symbols.cmd`
- Modify: `src/ahci.c`
- Modify: `src/mww.c`
- Modify: `src/rdx_core.c`
- Modify: `src/sci.c`
- Modify: `src/spi.c`
- Modify: `src/usb_stack.c`
- Inspect: `src/exceptions_isr.asm`
- Inspect: `src/intvecs.asm`

**Interfaces:**
- Consumes: 139 numbered branch/label bindings and local incoming/outgoing blocks
- Produces: unique structural labels describing retry, fallback, dispatch, cleanup, and completion roles

- [x] For each C label, inspect every incoming `goto`, the condition guarding it, and the first operation in the target block.
- [x] Assign a local control-flow name without asserting higher-level semantics unsupported by the block.
- [x] Rename matching linker bindings and declarations while retaining their exact addresses.
- [x] Inspect assembly-only and linker-only targets and either name them from surrounding vector/branch behavior or mark them unresolved.
- [x] Run source scans proving every selected old label family is absent.

### Task 6: Replace Generic Comments and Add Behavioral Explanations

**Files:**
- Modify: `src/ahci.c`
- Modify: `src/c_int00.c`
- Modify: `src/gio.c`
- Modify: `src/main.c`
- Modify: `src/mww.c`
- Modify: `src/one_touch.c`
- Modify: `src/pwm.c`
- Modify: `src/rdx_core.c`
- Modify: `src/reg_io.c`
- Modify: `src/rti.c`
- Modify: `src/sci.c`
- Modify: `src/scsi.c`
- Modify: `src/scsi_data.c`
- Modify: `src/spi.c`
- Modify: `src/string.c`
- Modify: `src/system.c`
- Modify: `src/system_init.c`
- Modify: `src/ums_bot.c`
- Modify: `src/ums_uas.c`
- Modify: `src/usb_chap9.c`
- Modify: `src/usb_hal.c`
- Modify: `src/usb_hal_isr.c`
- Modify: `src/usb_hid.c`
- Modify: `src/usb_stack.c`
- Modify: `src/usb_vendor.c`
- Modify: `src/vim_intvecs.c`
- Modify: `src/vim_nvic.c`
- Modify: `src/wdt.c`
- Modify: relevant public headers under `include/`
- Modify: `docs/RDX_SPECIFIC_FUNCTIONALITY.md`

**Interfaces:**
- Consumes: semantic symbol names and existing control/data-flow evidence
- Produces: specific Doxygen summaries and inline explanations throughout all existing subsystems

- [x] Replace every generic procedure comment with a function-specific explanation of inputs, state, hardware effects, and outputs.
- [x] Explain each non-obvious state-machine branch and transition, including terminal and retry paths.
- [x] Explain MMIO masks, polling conditions, and required ordering where visible from the code.
- [x] Explain packed-record offsets, fixed-address buffers, byte order, checksums, hashes, flash operations, and SCSI response construction.
- [x] Explain indirect callback selection and context passing.
- [x] Add uncertainty wording only where the structural effect is known but the domain role is not.
- [x] Review comments to remove repetition, unsupported claims, and narration of obvious assignments.

### Task 7: Document Unresolved Symbols and Verify Equivalence

**Files:**
- Create: `docs/UNRESOLVED_FIRMWARE_SYMBOLS.md`
- Verify: `tests/fixtures/firmware_symbol_manifest.json`
- Verify: `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex`

**Interfaces:**
- Consumes: final manifest, renamed tree, and saved baseline
- Produces: a complete evidence record and byte-equivalence proof

- [x] Generate the unresolved-symbol document with name, address, category, observed use, and reason a semantic name is not yet justified.
- [x] Confirm the unresolved document exactly matches manifest entries with an empty `new_name`.
- [x] Parse all 28 C sources in freestanding GNU89 mode.
- [x] Run `~/.platformio/penv/bin/python -m unittest discover -s tests -v` and confirm zero failures.
- [x] Run a clean `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt` build.
- [x] Build the saved baseline with the same environment and toolchain.
- [x] Compare the current and baseline Intel HEX files with `cmp`, SHA-256, and byte counts.
- [x] Confirm no source, declaration, linker address, ABI, build option, artifact name, or hardware state changed outside the approved naming and comment scope.
