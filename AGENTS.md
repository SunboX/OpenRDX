# AGENTS.md

## Project overview
PlatformIO wrapper for TUSB9261 RDX firmware. PlatformIO orchestrates
the build, but compilation, assembly, linking, the required ABI, and HEX
conversion remain TI ARM Code Generation Tools responsibilities.

## Key files
- `src/rdx_mount/`: active production build tree with 37 C sources and two TI
  assembly sources, producing 39 objects.
- Top-level `src/`: 28 C sources and two TI assembly sources retained as source
  recovery and reference evidence; they are not part of the active build.
- `include/rdx_mount/`: active production headers.
- Top-level `include/`: retained firmware and TI reference headers outside the
  active include tree.
- `linker/firmware_absolute_symbols.cmd`: fixed absolute linker symbols.
- `linker/tusb9260_link.cmd`: TI linker command file.
- `scripts/ti_cgt_build.py`: PlatformIO/SCons build adapter.
- `scripts/ti_cgt_tools.py`: host-specific TI CGT path resolver.
- `platformio.ini`: PlatformIO environment and Windows/macOS toolchain roots.
- `tests/test_ti_cgt_tools.py`: toolchain-resolution unit tests.
- `docs/development/building.md`: common build, test, and artifact guide.
- `docs/development/building-on-macos.md`: detailed verified Apple Silicon
  build route.

## Build and flash
- macOS build: `~/.platformio/penv/bin/pio run -e tusb9261_ti_cgt`
- Windows build:
  `C:\Users\andre\.platformio\penv\Scripts\pio.exe run -e tusb9261_ti_cgt`
- Alternate toolchain root: set `TI_CGT_ROOT` for the build command.
- There is no validated upload target. Do not flash these firmware artifacts
  unless the user explicitly requests it and supplies a separately
  validated safety procedure.

## Tests
- Run: `~/.platformio/penv/bin/python -m unittest discover -s tests -v`
- The suite covers toolchain resolution, release packaging, product language,
  and source-level firmware behavior contracts.

## Notes
- Keep the default PlatformIO environment named `tusb9261_ti_cgt` unless the
  user explicitly requests a different name.
- Preserve the Windows default `C:\ti\ti-cgt-arm_5.2.5`, `armcl.exe`, and
  `armhex.exe` behavior.
- Preserve the macOS default `~/ti/ti-cgt-arm_5.2.9` and extensionless launcher
  names. The verified Apple Silicon route runs Windows TI ARM CGT 5.2.9 through
  Wine. Rosetta runs the Intel-based Wine application; it does not make the
  native 32-bit macOS TI tools usable.
- Do not replace TI ARM CGT with native GCC, Clang, or a newer TI compiler.
  Newer TI CGT releases removed the required `ti_arm9_abi` COFF support.
- Do not change `ti_arm9_abi`, TI assembly syntax, linker command syntax,
  fixed absolute symbols, compiler/linker flags, or artifact names unless
  explicitly requested and validated on every supported host.
- Keep warnings promoted to errors.
- The recorded validation results apply only to the validated Windows TI ARM
  CGT 5.2.5 build. Never claim macOS 5.2.9 artifacts are byte-identical
  without direct hash evidence.
- Do not add model-specific source workarounds without support from the
  firmware behavior, TI references, or a reproducible failing test.

## Coding style and naming
- Always spell the project author's name “André Fiedler”, including copyright
  headers, documentation, and generated metadata.
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
- Before Git work, inspect the current branch, status, and remotes. Preserve
  unrelated user changes and do not assume a clean worktree.
- Commit messages must start with a prefix such as `fix:`, `feature:`, `docs:`,
  `test:`, or another agreed label, followed by a short imperative summary.
- Always create and push an explicit version-number Git tag for every GitHub
  release, using `v<MAJOR.MINOR>` (for example, `v1.06`). The tag must identify
  the exact validated release commit, and its version must match `VERSION` and
  the compiled firmware version. Publish the release against that tag and title
  it `OpenRDX v<MAJOR.MINOR>` (for example, `OpenRDX v1.06`).
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
- The active session's available-skills catalog is the source of truth; skill
  bodies live at their listed paths.
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
