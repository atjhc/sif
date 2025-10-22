# Repository Guidelines

## Project Structure & Module Organization
Core source lives in `src/` with submodules for `ast`, `compiler`, `runtime`, and `utilities`; tooling entry points sit in `src/tools/`. Public headers live in `include/sif/`, and integration assets land in `src/tests/resources/`. Docs and sample scripts are under `docs/` and `examples/`, while build outputs accumulate in `build/` (ignored by Git).

## Build, Test, and Development Commands
- `make` — configure with CMake and emit release binaries in `build/release/`.
- `make debug` — generate a Debug tree used by tests and instrumentation.
- `make test` — build the debug tree and run `ctest` (mirrors CI).
- `./build/release/sif_tool <script.sif>` — execute a Sif program; add `-i` for the REPL.
- `make format` — run clang-format across `src/` and `include/`.

## Coding Style & Naming Conventions
Formatting is enforced by the repository `.clang-format` (LLVM-derived, 4 spaces, 100-column limit). Match existing PascalCase header/source pairs, and keep test files in snake_case (for example `annotation_tests.cc`). Prefer expressive identifiers and mirror prevailing namespace usage. Run `make format` before pushing to keep canonical include ordering.

## Testing Guidelines
Unit and integration tests sit in `src/tests/` alongside custom fixtures. After `make debug`, run `./build/debug/sif_tests resources` to execute the suite, or pass `-g` / `-t` for a specific group or case (e.g. `./build/debug/sif_tests -g DebugInfoIntegration`). REPL transcript checks live in `src/tests/resources/transcripts/` and execute via `./build/debug/repl_tests.sh build/debug/sif_tool`. Aim to cover new bytecode paths or parser constructs with both assertions and transcript updates.

## Commit & Pull Request Guidelines
Commits follow the Git history: imperative, sentence case summaries without trailing periods (`Implement runtime error highlighting`). Group related edits and include rationale in the body when touching the compiler or VM. Pull requests should describe user-visible impact, list test coverage (`make test` or targeted runs), and link relevant issues or docs. Add screenshots or transcript diffs when altering diagnostics or REPL output, and flag migration notes for reviewers on larger changes.

## Environment & Tooling Notes
CI builds run via the repository Dockerfile, so validate Docker workflows when changing dependencies. Keep helper scripts in `scripts/` instead of relying on system-wide installs. Container builds mount the workspace under `/mnt/build`; verify relative paths when adding new artifacts. Reuse existing assets in `support/` rather than introducing new binaries elsewhere.
