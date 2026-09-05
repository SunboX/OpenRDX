# RDX Core Module Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Split `rdx_core.c` into sub-1,000-line responsibility-named fragments while preserving the exact TI translation unit and firmware output.

**Architecture:** Keep `src/rdx_core.c` as the sole compiled translation unit and include 24 ordered `.inc` fragments from `src/rdx_core/`. Track the fragments as PlatformIO dependencies, extend semantic scans to them, and prove textual and binary equivalence against a saved baseline.

**Tech Stack:** Existing GNU89/TI C, Python `unittest`, Clang, PlatformIO, TI ARM CGT 5.2.9, Intel HEX and linker-map comparison.

## Global Constraints

- Keep every source and implementation-fragment file below 1,000 lines.
- Preserve function order and exact implementation text.
- Preserve `ti_arm9_abi`, compiler/linker flags, fixed symbols, artifact names, and Windows/macOS tool selection.
- Do not add the fragments to `C_SOURCES`; TI CGT must still emit one `rdx_core.obj`.
- Do not flash hardware.
- This checkout has no Git metadata, so use an external snapshot and artifact comparisons instead of commits.

---

### Task 1: Add failing structural and dependency tests

**Files:**
- Create: `tests/test_rdx_core_fragments.py`
- Modify: `tests/test_semantic_identifiers.py`
- Test: `scripts/ti_cgt_build.py`

**Interfaces:**
- Consumes: the approved 24-fragment manifest and current monolithic source
- Produces: regression gates for fragment order, line limits, scan coverage, and build dependencies

- [x] Add a test constant listing the 24 exact fragment paths in include order.
- [x] Assert that the wrapper includes each fragment exactly once and contains no `core_*` function definition.
- [x] Assert that every wrapper/fragment file is below 1,000 lines.
- [x] Assert that the build adapter discovers `rdx_core/*.inc` and appends the list to `dependencies` without adding it to `C_SOURCES`.
- [x] Extend semantic identifier and generic-comment scans from root `*.c` files to root `*.c` plus recursive `*.inc` files.
- [x] Run `~/.platformio/penv/bin/python -m unittest tests.test_rdx_core_fragments -v` and confirm failure because the fragments and dependency integration do not yet exist.

### Task 2: Split the implementation without rewriting it

**Files:**
- Modify: `src/rdx_core.c`
- Create: `src/rdx_core/01_image_validation.inc` through `src/rdx_core/24_reconnect_state_machine.inc`
- Modify: `scripts/ti_cgt_build.py`

**Interfaces:**
- Consumes: the existing ordered function stream beginning at the first Doxygen block
- Produces: the same stream through ordered preprocessor includes

- [x] Save `rdx_core.c`, the current HEX, the linker map, and `rdx_core.obj` in a temporary external baseline directory.
- [x] Move only complete Doxygen-plus-function blocks into the approved fragments, preserving every byte within the moved blocks.
- [x] Replace the moved implementation in `rdx_core.c` with the 24 ordered includes.
- [x] Add `RDX_CORE_FRAGMENTS = sorted((SOURCE_DIR / "rdx_core").glob("*.inc"))` and append it to the build dependencies.
- [x] Assemble the implementation by concatenating fragments and compare it byte-for-byte with the saved implementation tail.
- [x] Run the focused fragment tests and confirm they pass.

### Task 3: Verify source and firmware equivalence

**Files:**
- Verify: `src/rdx_core.c`
- Verify: `src/rdx_core/*.inc`
- Verify: `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.hex`
- Verify: `.pio/build/tusb9261_ti_cgt/TUSB9261_RDX.map`

**Interfaces:**
- Consumes: the completed split and saved baseline artifacts
- Produces: source-order, compile, link-layout, and firmware-byte evidence

- [x] Run `~/.platformio/penv/bin/python -m unittest discover -s tests -v` and require zero failures.
- [x] Parse all 28 root C sources with `clang -std=gnu89 -ffreestanding -fsyntax-only -Wno-everything -Iinclude`.
- [x] Run a clean `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt` build.
- [x] Compare the current and baseline HEX files with SHA-256 and `cmp`.
- [x] Compare the `rdx_core.obj (.text)` origin and size in both linker maps.
- [x] Confirm no wrapper or fragment reaches 1,000 lines and no hardware action occurred.
