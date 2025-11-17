# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

- `make` - Build release artifacts in `build/release/`
- `make debug` - Build debug version in `build/debug/`
- `make test` - Run all tests (builds debug first)
- `make format` - Format C++ code using clang-format
- `make clean` - Remove all build artifacts

### Running Sif
- `./build/release/sif_tool <file>` - Execute a Sif script
- `./build/release/sif_tool -i` - Interactive REPL mode
- `./build/release/sif_tool -e "code"` - Execute code string directly

### Testing
- Unit tests: `build/debug/sif_tests resources`
- REPL tests: `build/debug/repl_tests.sh build/debug/sif_tool`
- Individual test transcripts in `src/tests/resources/transcripts/`

#### Running Specific Tests
- Run specific test group: `./build/debug/sif_tests -g GroupName`
- Run specific test case: `./build/debug/sif_tests -g GroupName -t TestName`
- List available options: `./build/debug/sif_tests --help`
- Examples:
  - `./build/debug/sif_tests -g DebugInfoIntegration`
  - `./build/debug/sif_tests -g NativeCallContext -t ErrorMethodWithRanges`

#### Transcript Test Format
- `(-- expect --)` blocks validate stdout output
- `(-- error --)` blocks validate stderr output and compilation errors
- `print error (the error)` prints to stderr, so use `(-- error --)`
- `print` statements print to stdout, so use `(-- expect --)`
- Expected output blocks should immediately follow each test case (no blank line between)

#### Fuzz Testing

Fuzz testing uses LibFuzzer with AddressSanitizer to find bugs by feeding random/mutated inputs to the Sif interpreter.

**Building the fuzzer:**
- `make fuzz` - Build fuzzer binary in `build/fuzz/sif_fuzz`
- Requires Homebrew LLVM on macOS (automatically detected)

**Running the fuzzer:**
```bash
# Run for 60 seconds, pipe output to file to avoid token usage
ASAN_OPTIONS=detect_container_overflow=0 ./build/fuzz/sif_fuzz -max_total_time=60 -artifact_prefix=build/fuzz/fuzz/ build/fuzz/fuzz/corpus > tmp/fuzz_output.txt 2>&1
tail -100 tmp/fuzz_output.txt  # Review results

# Run with custom corpus directory
./build/fuzz/sif_fuzz -max_total_time=30 /path/to/corpus > tmp/fuzz_output.txt 2>&1

# Test a specific input
./build/fuzz/sif_fuzz /path/to/input/file
```

**Verifying crashes (efficient workflow):**
1. After fuzzer run, check for crash files: `ls -lh build/fuzz/fuzz/crash-* 2>/dev/null | wc -l`
2. For each crash file, test if it reproduces: `./build/fuzz/sif_fuzz path/to/crash-file 2>&1 | grep -i "aborting\|summary"`
3. If no "ABORTING" or "SUMMARY" in output, the crash is NOT reproducible (fuzzer artifact)
4. Only investigate crashes that consistently reproduce

**Understanding results:**
- Crash files are saved to `build/fuzz/fuzz/crash-*`
- **Most fuzzer crashes are NOT reproducible** - they occur due to memory state from previous iterations
- Only crashes that reproduce when run in isolation are real bugs worth investigating
- The fuzzer mutates test files from `src/tests/resources/transcripts/` as the initial corpus
- AddressSanitizer detects memory safety issues (buffer overflows, use-after-free, etc.)

**Notes:**
- The fuzzer catches UTF-8 decoding exceptions and reports them as compiler errors (no crashes)
- Container overflow detection is disabled (`detect_container_overflow=0`) to reduce false positives
- The fuzzer is built with both fuzzer and address sanitizer instrumentation (`-fsanitize=fuzzer,address`)

### Testing with Temporary Files

When testing or experimenting with Sif code:
- **ALWAYS** use the `tmp/` directory for temporary test files
- **NEVER** use `cat` or heredocs to create temporary files in `/tmp` or elsewhere
- Use the normal `Write` tool to create files in `tmp/` (e.g., `tmp/test.sif`)
- The `tmp/` directory is gitignored and cleaned by `make clean`
- Example: Write test code to `tmp/test_parse.sif` and run with `./build/debug/sif_tool tmp/test_parse.sif`

## Architecture Overview

Sif is a natural language-like scripting language with a three-stage execution pipeline:

### Core Components

**Parser → Compiler → Virtual Machine**

1. **Parser** (`include/sif/compiler/Parser.h`)
   - Transforms source code into Abstract Syntax Tree (AST)
   - Handles natural language syntax with multi-word function names
   - Supports interactive parsing for REPL mode
   - **Error Recovery**: When errors occur, the parser constructs partial AST nodes with Optional fields for missing data. If any errors are emitted during parsing, the AST is invalid for compilation but can still be traversed by visitors (e.g., for syntax highlighting). Visitors must be fault-tolerant to Optional/empty fields.

2. **Compiler** (`include/sif/compiler/Compiler.h`)
   - Compiles AST to bytecode using visitor pattern
   - Manages local variables, captures, and scope resolution
   - Handles function declarations and variable assignments

3. **Virtual Machine** (`include/sif/runtime/VirtualMachine.h`)
   - Stack-based bytecode interpreter
   - Manages call frames, global variables, and execution state
   - Special handling for "it" variable (automatic result capture)

### Value System

**Value** (`include/sif/runtime/Value.h`) - Variant type supporting:
- Empty, Bool, Integer, Float, Object
- Objects include String, List, Dictionary, Function, Native, Range

### Module System

- **Core Module** - Built-in functions (print, read, type operations)
- **System Module** - System information and environment access
- **ModuleLoader** - Handles module imports and dependencies

### Key Design Patterns

- **Visitor Pattern**: AST traversal in compiler and pretty printer
- **Strong<T>**: Smart pointer system for memory management
- **Result<T, Error>**: Error handling without exceptions
- **Natural Language Syntax**: Multi-word function names like "the factorial of {x}"

### Special Features

- **"it" Variable**: Automatically captures last expression result
- **Interactive Mode**: REPL with persistent state across evaluations
- **Natural Syntax**: English-like control structures and function calls
- **Bytecode Compilation**: Intermediate representation for efficient execution

## Language Documentation

@docs/introduction.md

## C++ Style Guidelines

### Naming Conventions

**Functions:**
- **Free functions/utilities**: PascalCase (e.g., `Format()`, `MakeStrong()`, `GenerateSignatureVariations()`)
- **Member functions**: camelCase (e.g., `handleInitialize()`, `openDocument()`, `getDocument()`)
- **External library compatibility**: Follow library conventions (e.g., `to_json()`, `from_json()` for nlohmann::json)

**Variables:**
- **Local/member variables**: camelCase with underscore prefix for members (e.g., `_documentManager`, `_rpc`)
- **Parameters**: camelCase (e.g., `textBeforeCursor`, `completionText`)

**Types:**
- **Classes/structs**: PascalCase (e.g., `DocumentManager`, `CompletionItem`, `SourceRange`)
- **Type aliases**: PascalCase (e.g., `Strong<T>`, `Result<T, E>`)

**Constants:**
- **Macros**: SCREAMING_SNAKE_CASE (e.g., `MAJOR_VERSION`, `SIF_NAMESPACE_BEGIN`)
- **Static constants**: PascalCase (e.g., `MajorVersion`, `Version`)

### Code Style

**Control Flow:**
- Prefer guard clauses over nested conditionals
- Early returns for error conditions
```cpp
// Good
if (terms.empty()) {
    return {{}};
}
// ... rest of logic

// Avoid
if (!terms.empty()) {
    // ... lots of nested logic
}
```

**Comments:**
- Use sparingly; prefer self-documenting code
- When needed, explain _why_ not _what_
- Use `//` for single-line comments
```cpp
// Good: Explains reasoning
// Parser manages interpolation state through its call stack to maintain proper nesting

// Avoid: States the obvious
// Set result to empty vector
```

**Includes:**
- Order: System headers, then external libraries, then project headers
- Use `#pragma once` for header guards
```cpp
#include <iostream>
#include <vector>

#include "sif/Common.h"
#include "sif/lsp/Protocol.h"
```

**Formatting:**
- Opening braces on same line for functions and control structures
- Spaces around operators
- No spaces inside parentheses
- Use clang-format via `make format`

### Code Organization

**Files:**
- Header files in `include/sif/`
- Implementation files in `src/`
- One class per file (generally)
- Utility functions grouped by purpose (e.g., `CompletionUtils`, `strings`)

**Namespaces:**
- Use `SIF_NAMESPACE_BEGIN` and `SIF_NAMESPACE_END` macros
- Sub-namespaces for modules (e.g., `namespace lsp`)

## Commit Message Guidelines

When writing commit messages:

**Structure:**
- **Title**: Brief description of the main focus (imperative mood, e.g., "Add support for..." not "Added support for...")
- **Description**: Brief explanation of the new feature or change
- **Details**: List-style description of related changes

**Content:**
- Focus ONLY on what is being changed in this commit
- NEVER reference bugs found during development or implementation decisions made along the way
- Describe the final state, not the journey to get there

**Example:**
```
Add structured parameter destructuring for functions

Functions can now destructure parameters using the same syntax as
assignments. This allows natural unpacking of list arguments.

- Add AssignmentTarget hierarchy to FunctionDecl
- Update Parser to convert Signature arguments to AssignmentTargets
- Update Compiler to use visitor pattern for parameter locals
- Make Variable.name Optional for partial AST construction
```

**Anti-pattern:**
```
Fix function parameters (had to refactor because of a bug we found)

We discovered that the Signature approach wasn't working, so we
changed to use AssignmentTarget instead. Also fixed some issues
with the parser that came up during testing.
```

## Important File Locations

- Main entry point: `src/tools/sif.cc`
- Core language headers: `include/sif/`
- Test resources: `src/tests/resources/transcripts/`
- Language examples: `examples/`
- External dependencies: `src/extern/`