# Add Adapted AGENTS.md Rules Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a root `AGENTS.md` containing a faithful semantic adaptation of every rule category from the WIDI reference file.

**Architecture:** One root instruction file governs the complete project tree. Project-specific rules protect the TI CGT build contract, while the reference file's reusable development, testing, security, and skill rules remain as dedicated sections.

**Tech Stack:** Markdown, PlatformIO Core 6.x, Python `unittest`, TI ARM CGT 5.2.x.

## Global Constraints

- Preserve every rule category from `/Users/afiedler/Documents/projects/widi_core/AGENTS.md`.
- Remove stale ESP32, BLE MIDI, Bluetooth audio, partition-table, firmware API, and WIDI path assumptions.
- Preserve Windows TI ARM CGT 5.2.5 behavior and the legacy `ti_arm9_abi` build contract.
- Do not authorize flashing of the firmware.
- This directory has no Git metadata, so do not create a commit.

---

### Task 1: Create and verify the root instructions

**Files:**
- Create: `AGENTS.md`
- Compare: `/Users/afiedler/Documents/projects/widi_core/AGENTS.md`
- Verify against: `README.md`, `platformio.ini`, `scripts/ti_cgt_build.py`, `tests/test_ti_cgt_tools.py`

**Interfaces:**
- Consumes: reference rule categories and current project commands, paths, toolchain constraints, and safety status.
- Produces: root-scoped instructions for all future agents operating anywhere under this project.

- [ ] **Step 1: Create `AGENTS.md` with the complete adapted content**

The file must contain all of these sections and rules:

```markdown
# AGENTS.md

## Project overview
PlatformIO wrapper for TUSB9261 RDX firmware. PlatformIO orchestrates
the build, but compilation, assembly, linking, the legacy ABI, and HEX
conversion remain TI ARM Code Generation Tools responsibilities.

## Key files
- `src/`: 28 firmware C sources and two TI assembly sources.
- `include/`: firmware headers and TI reference headers.
- `linker/firmware_absolute_symbols.cmd`: fixed absolute linker symbols.
- `linker/tusb9260_link.cmd`: TI linker command file.
- `scripts/ti_cgt_build.py`: PlatformIO/SCons build adapter.
- `scripts/ti_cgt_tools.py`: host-specific TI CGT path resolver.
- `platformio.ini`: PlatformIO environment and Windows/macOS toolchain roots.
- `tests/test_ti_cgt_tools.py`: toolchain-resolution unit tests.
- `README.md` and `doc/PLATFORMIO_BUILD_VALIDATION.md`: requirements and
  validated Windows build evidence.

## Build and flash
- macOS build: `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`
- Windows build:
  `C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt`
- Alternate toolchain root: set `TI_CGT_ROOT` for the build command.
- There is no validated upload target. Do not flash these firmware
  artifacts unless the user explicitly requests it and supplies a separately
  validated safety procedure.

## Tests
- Run: `~/.platformio/penv/bin/python -m unittest discover -s tests -v`
- The current suite validates Windows, macOS, and environment-override TI CGT
  path selection.

## Notes
- Keep the default PlatformIO environment named `tusb9261_ti_cgt` unless the
  user explicitly requests a toolchain change.
- Preserve the Windows default `C:\ti\ti-cgt-arm_5.2.5`, `armcl.exe`, and
  `armhex.exe` behavior.
- Preserve the macOS default `~/ti/ti-cgt-arm_5.2.9` and extensionless tool
  names. Apple Silicon requires Rosetta for the Intel compiler binaries.
- Do not replace TI ARM CGT with native GCC, Clang, or a newer TI compiler.
  Newer TI CGT releases removed the required `ti_arm9_abi` COFF support.
- Do not change `ti_arm9_abi`, TI assembly syntax, linker command syntax,
  fixed absolute symbols, compiler/linker flags, or artifact names unless
  explicitly requested and validated on every supported host.
- Keep warnings promoted to errors.
- The recorded reproducibility hashes apply only to the validated Windows TI
  ARM CGT 5.2.5 build. Never claim macOS 5.2.9 artifacts are byte-identical
  without direct hash evidence.
- Do not add model-specific source workarounds without evidence from the
  firmware, TI references, or a reproducible failing test.

## Coding style and naming
- No repository-wide formatter is configured. Follow the surrounding C, TI
  assembly, linker-command, Python, and Markdown style.
- Use four spaces for Python indentation and keep names descriptive.
- Keep files under 1000 lines; split into focused modules when they grow.
- Add Doxygen-style comments for every C/C++ function or method, including
  private helpers. Add docstrings for Python functions and helpers.
- Add inline comments where decisions, firmware behavior, ABI constraints, or
  non-obvious hardware behavior need clarity.

## Testing guidelines
- Run the Python unit suite after changing build adapters, toolchain selection,
  or tests.
- Run the PlatformIO build after firmware, headers, linker files, build scripts,
  or configuration changes when the required TI CGT installation is available.
- If the required compiler is unavailable, report the exact resolved missing
  path. Do not substitute GCC, Clang, or an incompatible TI CGT release.
- For every new feature, fix, or behavior change, add an automated test under
  `tests/` when technically possible and document the change in `README.md`.
- Do not run hardware-dependent or flashing operations unless explicitly asked.

## Commit and pull request guidelines
- This directory currently has no Git metadata; do not assume commits, branches,
  remotes, or merge requests are available.
- If the project is placed under Git, commit messages must start with a prefix
  such as `fix:`, `feature:`, `docs:`, `test:`, or another agreed label,
  followed by a short imperative summary.
- For GitLab merge requests, provide a concise summary, affected repositories
  or paths, and exact testing performed. Attach UI screenshots when a change
  introduces or modifies a UI.

## Security and configuration tips
- `TI_CGT_ROOT` is an optional local filesystem path override; it is not a
  credential and must not encode credentials.
- Never commit myTI credentials, authentication cookies, tokens, licensed
  installer responses, or other secrets.
- Keep secrets out of version control. If `.env` files are introduced, add them
  to `.gitignore` before use.

## Skills
- `skill-creator`: Guide for creating effective skills. Use when a user wants
  to create or update a skill. (file:
  `/Users/afiedler/.codex/skills/.system/skill-creator/SKILL.md`)
- `skill-installer`: Install Codex skills into `$CODEX_HOME/skills` from a
  curated list or GitHub repository path. Use when a user asks to list or
  install skills. (file:
  `/Users/afiedler/.codex/skills/.system/skill-installer/SKILL.md`)
- The active session's available-skills catalog and the entries above are the
  source of truth; skill bodies live at their listed paths.
- No project-specific skills are present in this project unless a `SKILL.md`
  file is added later.

## Trigger rules
- If the user names a skill with `$SkillName` or plain text, or the task clearly
  matches a skill's description, use that skill for the turn.
- Multiple matches mean use all required skills. Do not carry skills across
  turns unless they are mentioned or triggered again.
- The YAML `description` in `SKILL.md` is the primary trigger signal. If the
  task remains genuinely ambiguous after inspecting available context, ask one
  brief clarification unless the user has instructed you not to ask questions.

## How to use a skill
- After selecting a skill, open and read its complete `SKILL.md` before taking
  task actions.
- Resolve relative references from the directory containing `SKILL.md`. Load
  only the specific referenced resources required for the task.
- If `scripts/` exist, prefer running or patching them instead of retyping
  substantial code.
- Reuse provided assets and templates instead of recreating them.

## Coordination and sequencing
- If multiple skills apply, choose the minimal set covering the request and
  state the order in which they will be used.
- Announce which skills are being used and why in one short progress update.
  If an obvious skill is skipped, state the reason.

## Context hygiene
- Keep context focused: summarize long sections and load only files needed for
  the current task.
- Avoid deeply nested reference chasing; prefer one-hop resources explicitly
  linked from `SKILL.md` unless blocked.
- When variants exist, choose only the relevant framework, provider, platform,
  or domain reference and note that choice.

## Missing or blocked skills
- If a named skill is unavailable or its path cannot be read, state that
  briefly and continue with the best available fallback.

## Safety and fallback
- If a skill cannot be applied cleanly because files are missing or instructions
  are unclear, state the issue, choose the safest in-scope alternative, and
  continue.
- Preserve unrelated user files and changes. Do not perform destructive Git,
  filesystem, hardware, release, or deployment actions without explicit scope.
```

- [ ] **Step 2: Verify all reference rule categories are represented**

Run:

```bash
for heading in \
  'Project overview' 'Key files' 'Build and flash' 'Tests' 'Notes' \
  'Coding style and naming' 'Testing guidelines' \
  'Commit and pull request guidelines' 'Security and configuration tips' \
  'Skills' 'Trigger rules' 'How to use a skill' \
  'Coordination and sequencing' 'Context hygiene' \
  'Missing or blocked skills' 'Safety and fallback'; do
  rg -q "^## ${heading}$" AGENTS.md || exit 1
done
```

Expected: exit code 0.

- [ ] **Step 3: Verify project-specific terms and removal of stale assumptions**

Run:

```bash
rg -n 'tusb9261_ti_cgt|ti_arm9_abi|ti-cgt-arm_5\.2\.[59]|TI_CGT_ROOT|unittest' AGENTS.md
if rg -n 'ESP32|BLE MIDI|Bluetooth audio|edrum-api|g1_bluetooth|firmware_version|esp-web-tools' AGENTS.md; then
  exit 1
fi
```

Expected: current TUSB9261 terms are present and stale WIDI-specific terms are absent.

- [ ] **Step 4: Verify referenced local files and commands**

Run:

```bash
for path in \
  src include linker/firmware_absolute_symbols.cmd linker/tusb9260_link.cmd \
  scripts/ti_cgt_build.py scripts/ti_cgt_tools.py platformio.ini \
  tests/test_ti_cgt_tools.py README.md doc/PLATFORMIO_BUILD_VALIDATION.md; do
  test -e "$path" || exit 1
done
~/.platformio/penv/bin/python -m unittest discover -s tests -v
```

Expected: every referenced path exists and all current unit tests pass.
