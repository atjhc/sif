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

## Architecture Overview

Sif is a natural language-like scripting language with a three-stage execution pipeline:

### Core Components

**Parser → Compiler → Virtual Machine**

1. **Parser** (`include/sif/compiler/Parser.h`)
   - Transforms source code into Abstract Syntax Tree (AST)
   - Handles natural language syntax with multi-word function names
   - Supports interactive parsing for REPL mode

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

## Important File Locations

- Main entry point: `src/tools/sif.cc`
- Core language headers: `include/sif/`
- Test resources: `src/tests/resources/transcripts/`
- Language examples: `examples/`
- External dependencies: `src/extern/`