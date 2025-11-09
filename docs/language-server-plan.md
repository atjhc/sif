# Sif Language Server Plan

## Overview

This document outlines the plan for building a Language Server Protocol (LSP) implementation for Sif. The language server will provide IDE features like syntax highlighting, autocomplete, diagnostics, and more.

## Architecture Analysis

### Existing Codebase Components

#### 1. Scanner (Lexer)
- **Location**: `include/sif/compiler/Scanner.h`
- **Capabilities**:
  - Tokenizes source code into typed tokens
  - Tracks precise source locations (line, column) for each token
  - Supports Unicode characters
  - Handles string interpolation
  - Provides token types (keywords, literals, operators, etc.)
- **LSP Usage**: Perfect for semantic token highlighting and initial syntax validation

#### 2. Parser
- **Location**: `include/sif/compiler/Parser.h`
- **Capabilities**:
  - Builds Abstract Syntax Tree (AST) from tokens
  - Tracks all declared functions via `Signature` objects
  - Maintains variable declarations with scope information
  - Records comment ranges
  - Provides checkpoint/rewind for error recovery
  - Supports interactive mode (useful for incremental parsing)
- **LSP Usage**:
  - AST for semantic analysis
  - Function signatures for autocomplete
  - Variable tracking for autocomplete and go-to-definition
  - Error reporting with precise source ranges

#### 3. Token System
- **Location**: `include/sif/compiler/Token.h`
- **Token Types**: 91 distinct token types including:
  - Keywords (function, if, repeat, return, etc.)
  - Operators (+, -, *, /, ^, etc.)
  - Literals (string, integer, float, boolean)
  - Structural elements (brackets, parens, braces)
  - Natural language connectors (an, and, as, in, is, to, etc.)
- **LSP Usage**: Direct mapping to semantic token types for syntax highlighting

#### 4. Signature System
- **Location**: `include/sif/compiler/Signature.h`
- **Capabilities**:
  - Describes function signatures with natural language patterns
  - Supports arguments, choices, and optional terms
  - Used by Grammar for function call matching
- **LSP Usage**: Generate completion items for available functions

#### 5. Grammar
- **Location**: `include/sif/compiler/Grammar.h`
- **Capabilities**:
  - Tree structure for matching function calls
  - Stores all valid function signatures
  - Enables natural language function call parsing
- **LSP Usage**: Context-aware autocomplete suggestions

#### 6. Compiler
- **Location**: `include/sif/compiler/Compiler.h`
- **Capabilities**:
  - Semantic analysis via AST visitor pattern
  - Type checking and validation
  - Scope resolution (local/global variables)
  - Captures and closure handling
  - Error collection with source ranges
- **LSP Usage**: Provides diagnostics (errors/warnings) for the editor

#### 7. AST Nodes
- **Locations**: `include/sif/ast/Statement.h`, `include/sif/ast/Expression.h`
- **Capabilities**:
  - Complete representation of program structure
  - All nodes contain `SourceRange` for precise location tracking
  - Visitor pattern for traversal
  - Detailed range information for all syntax elements
- **LSP Usage**:
  - Code navigation (go to definition, find references)
  - Symbol outline
  - Hover information

## Network Library Choice

**Selected Library: cpp-httplib**

### Rationale:
- **Header-only**: Easy to integrate, no complex build setup
- **Modern C++**: Uses C++11 features, clean API
- **JSON-RPC ready**: Handles HTTP POST requests needed for LSP
- **Lightweight**: Minimal dependencies
- **Well-maintained**: Active development, good documentation
- **LSP Compatible**: Many LSP servers use HTTP transport with JSON-RPC

### Alternative Considered:
- **stdio-based LSP**: Simpler but less flexible for debugging
- We'll start with stdio (standard LSP approach) and can add HTTP later if needed

**Decision**: Start with **stdio-based JSON-RPC** (standard LSP approach), use **nlohmann/json** for JSON serialization.

## Implementation Plan

### Phase 1: Foundation (Priority: High)

#### 1.1 LSP Infrastructure
- Set up JSON-RPC message handling (use nlohmann/json)
- Implement stdio-based communication (read/write JSON-RPC messages)
- Create LSP message types (requests, responses, notifications)
- Handle message routing and dispatching

#### 1.2 Basic Server Lifecycle
- `initialize` request handling
- `initialized` notification
- `shutdown` request
- `exit` notification
- Capability negotiation

#### 1.3 Document Synchronization
- `textDocument/didOpen` - Track opened documents
- `textDocument/didChange` - Incremental or full sync
- `textDocument/didClose` - Clean up document state
- Maintain document cache with parsed ASTs

### Phase 2: Syntax Highlighting (Priority: High - Most User Value)

#### 2.1 Semantic Tokens Provider
- Implement `textDocument/semanticTokens/full`
- Map Sif token types to LSP semantic token types:
  - **Keywords**: if, function, repeat, return, etc.
  - **Operators**: +, -, *, /, =, etc.
  - **String literals**: String tokens
  - **Number literals**: IntLiteral, FloatLiteral
  - **Comments**: Comment tokens
  - **Variables**: Word tokens (context-dependent)
  - **Functions**: Call expressions (from AST)
  - **Parameters**: Function parameter placeholders

#### 2.2 Token Classification Algorithm
```
For each document:
  1. Run Scanner to tokenize entire document
  2. Optionally run Parser to get AST for context
  3. Map each token to semantic token type:
     - Use token.type for basic classification
     - Use AST context for Variables vs Functions
     - Use Grammar for known function names
  4. Return semantic tokens in LSP format (line, column, length, type, modifiers)
```

### Phase 3: Autocomplete (Priority: High - Most User Value)

#### 3.1 Completion Items from Keywords
- Create completion items for all Sif keywords
- Include snippets for common patterns:
  - `function {name}...end function`
  - `if {condition} then...end if`
  - `repeat for {var} in {collection}...end repeat`

#### 3.2 Completion Items from Built-in Functions
- Extract signatures from Core module
- Extract signatures from System module
- Generate completion items with:
  - Label (function name)
  - Detail (full signature)
  - Documentation (from comments/docs)
  - Insert text (natural language pattern)

#### 3.3 Completion Items from User Functions
- Use Parser's `declarations()` to get all function signatures
- Generate completion items dynamically
- Include function parameters as placeholders

#### 3.4 Variable Name Completion
- Use Parser's `variables()` to get all declared variables
- Include both local and global variables
- Filter by current scope if available

#### 3.5 Context-Aware Completion
- Use Grammar tree to suggest valid next terms
- For partial function calls, suggest argument positions
- For partial expressions, suggest operators and functions

### Phase 4: Diagnostics (Priority: Medium)

#### 4.1 Syntax Error Reporting
- Run Parser on document changes
- Collect parse errors with source ranges
- Convert to LSP diagnostic format
- Publish via `textDocument/publishDiagnostics`

#### 4.2 Semantic Error Reporting
- Run Compiler on parsed AST
- Collect type errors, undefined variables, etc.
- Convert compiler errors to diagnostics
- Include severity levels (error, warning, info)

### Phase 5: Advanced Features (Priority: Low)

#### 5.1 Go to Definition
- Track symbol definitions during parsing
- Implement `textDocument/definition`
- Support functions, variables, modules

#### 5.2 Find References
- Build symbol usage index during compilation
- Implement `textDocument/references`

#### 5.3 Hover Information
- Implement `textDocument/hover`
- Show type information, function signatures, documentation

#### 5.4 Document Symbols
- Implement `textDocument/documentSymbol`
- Show outline of functions, variables

#### 5.5 Code Actions
- Quick fixes for common errors
- Refactoring suggestions

#### 5.6 Formatting
- Implement `textDocument/formatting`
- Use PrettyPrinter or create dedicated formatter

## Technical Decisions

### Document Management
- **Strategy**: Parse on every change, cache AST
- **Rationale**: Sif files are typically small, parsing is fast, full context available

### Error Recovery
- **Strategy**: Use Parser's checkpoint/rewind for partial parses
- **Rationale**: Provide best-effort completions even with syntax errors

### Incremental Updates
- **Phase 1**: Full document sync (simpler)
- **Phase 2**: Consider incremental sync if performance issues arise

### Testing Strategy
- Unit tests for LSP message handling
- Integration tests with real Sif files
- Manual testing with VS Code extension

## File Structure

```
src/
  lsp/
    Server.h/cc         - Main LSP server class
    Protocol.h/cc       - LSP message types and JSON serialization
    DocumentManager.h/cc - Track open documents and their ASTs
    SemanticTokens.h/cc - Semantic token provider
    Completion.h/cc     - Completion item provider
    Diagnostics.h/cc    - Diagnostic provider

tools/
  sif_lsp.cc           - Language server executable entry point
```

## Dependencies

- **nlohmann/json**: JSON serialization (already common in C++ projects)
- **Existing Sif code**: Scanner, Parser, Compiler, AST

## VS Code Extension

Create a minimal extension to test the language server:

```typescript
// extension.ts
import { ExtensionContext } from 'vscode';
import { LanguageClient, LanguageClientOptions, ServerOptions } from 'vscode-languageclient/node';

export function activate(context: ExtensionContext) {
  const serverOptions: ServerOptions = {
    command: 'sif_lsp',  // Path to language server executable
    args: []
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector: [{ scheme: 'file', language: 'sif' }],
    synchronize: {
      fileEvents: workspace.createFileSystemWatcher('**/*.sif')
    }
  };

  const client = new LanguageClient('sifLsp', 'Sif Language Server', serverOptions, clientOptions);
  client.start();
}
```

## Success Metrics

### Phase 1 & 2 (High Priority): ✅ COMPLETED
- ✅ Syntax highlighting works for all Sif constructs
- ✅ Keywords, operators, strings, numbers, comments are colored correctly
- ✅ Functions and variables have distinct colors
- ✅ Multi-word function names properly highlighted
- ✅ Function parameters colored as variables
- ✅ String interpolation correctly parsed and highlighted

### Phase 3 (High Priority): ⏳ NOT STARTED
- ⏳ Autocomplete suggests all keywords in appropriate contexts
- ⏳ Autocomplete suggests all built-in functions
- ⏳ Autocomplete suggests user-defined functions
- ⏳ Autocomplete suggests declared variables
- ⏳ Completions have useful documentation

### Phase 4 (Medium Priority): ✅ COMPLETED
- ✅ Syntax errors appear immediately in the editor
- ✅ Error locations are accurate
- ✅ Multiple errors displayed per document
- ⏳ Semantic errors are reported (only parser errors currently)

## Timeline Estimate

- **Phase 1** (Infrastructure): 2-3 days
- **Phase 2** (Syntax Highlighting): 1-2 days
- **Phase 3** (Autocomplete): 2-3 days
- **Phase 4** (Diagnostics): 1-2 days
- **Phase 5** (Advanced Features): 4-5 days per feature

**Total for Phases 1-3**: ~1 week (provides immediate user value)

## Completed Work

### Phase 1: Foundation ✅ (Completed)

#### LSP Infrastructure
- ✅ **JSON-RPC Implementation** (`{src|include}/lsp/JsonRPC.{h|cc}`)
  - Stdio-based communication with proper message framing
  - Content-Length header handling
  - Request/response/notification routing
  - Uses nlohmann/json for serialization

#### Server Lifecycle
- ✅ **Basic Lifecycle** (`src/lsp/Server.h/cc`)
  - `initialize` request with capability negotiation
  - `initialized` notification
  - `shutdown` request handling
  - `exit` notification with proper exit codes
  - Registered handlers for all lifecycle methods

#### Document Synchronization
- ✅ **Document Manager** (`src/lsp/DocumentManager.h/cc`)
  - `textDocument/didOpen` - Full document sync
  - `textDocument/didChange` - Full document replacement
  - `textDocument/didClose` - Document cleanup
  - Document cache with parsed ASTs, signatures, and variables
  - Integration with Scanner, Parser, Core, and System modules

### Phase 2: Syntax Highlighting ✅ (Completed)

#### Semantic Tokens Provider
- ✅ **Implementation** (`src/lsp/SemanticTokens.h/cc`)
  - `textDocument/semanticTokens/full` handler
  - Dual-source token collection:
    1. **Scanner-based**: Comments, strings, numbers, operators
    2. **AST-based**: Keywords, functions, variables, parameters
  - Context-aware highlighting using AST visitor pattern

#### Token Type Mapping
- ✅ **Comprehensive Mapping**:
  - **Keywords**: function, if, then, else, end, repeat, return, etc.
  - **Operators**: +, -, *, /, =, !=, <, >, etc.
  - **Strings**: StringLiteral, OpenInterpolation, Interpolation, ClosedInterpolation
  - **Numbers**: IntLiteral, FloatLiteral
  - **Comments**: Single-line (--) and multi-line ((-- --))
  - **Functions**: Multi-word function names from Call expressions
  - **Variables**: Parameters and variable references

#### Parser Integration for Context
- ✅ **FunctionDecl Ranges** (`src/compiler/Parser.cc`)
  - Extracts source ranges for function name words
  - Tracks parameter variable ranges
  - Populates `ranges.words` for function names
  - Populates `ranges.variables` for parameters

- ✅ **Call Ranges**
  - Function call name highlighting
  - Proper range tracking for multi-word function calls

### Phase 4: Diagnostics ✅ (Completed)

#### Diagnostic Publishing
- ✅ **Implementation** (`src/lsp/Server.cc`)
  - `textDocument/publishDiagnostics` notifications
  - Automatic publishing on document open/change
  - Conversion from Sif Error to LSP Diagnostic format
  - Proper source range to line/column mapping
  - Error severity and source attribution

#### Error Reporting
- ✅ **Parser Errors**
  - Syntax errors with accurate locations
  - Detailed error messages (e.g., "expected end", "unterminated string")
  - Multiple diagnostics per document
  - Real-time error display in VS Code

### Bug Fixes

#### String Interpolation Scanner Bug ✅ (Fixed)
Multiple fixes to properly handle string interpolation:

1. **Scanner.cc** (`src/compiler/Scanner.cc`)
   - Fixed `scanString()` to include `{` and `}` in interpolation tokens
   - Removed code that excluded `}` from ClosedInterpolation
   - Added `interpolating` and `stringTerminal` reset in `reset()` method
   - Ensured `advance()` includes terminal characters in all token types

2. **Token.cc** (`src/compiler/Token.cc`)
   - Updated `encodedString()` to strip both first and last characters from all interpolation token types
   - Unified handling for OpenInterpolation, Interpolation, ClosedInterpolation

3. **Parser.cc** (`src/compiler/Parser.cc`)
   - Removed incorrect save/restore of `interpolating` and `stringTerminal` flags
   - Let Scanner fully manage interpolation state
   - Fixed state corruption that occurred after parsing interpolated strings

**Result**: Interpolation now works correctly in all contexts, including:
- Basic string interpolation: `"Hello, {name}!"`
- Function parameters with interpolation: `function greet {name} print "Hello, {name}!"`
- Nested interpolations and complex expressions

### VS Code Extension ✅ (Created)

- ✅ **Extension Structure** (`support/vscode-extension/`)
  - `package.json` with language server configuration
  - Language configuration for brackets, comments
  - Semantic token legend with proper token types

- ✅ **Language Server Client**
  - Connects to `sif_lsp` binary
  - Semantic tokens enabled
  - Diagnostic display enabled
  - Works with standard VS Code error display

### File Structure (Actual)

```
include/sif/lsp/
  Server.h             - Main LSP server class
  Protocol.h           - LSP message types and JSON serialization
  DocumentManager.h    - Document cache with ASTs, signatures, variables
  SemanticTokens.h     - Semantic token provider with Scanner + AST
  JsonRPC.h            - JSON-RPC message handling

src/lsp/
  Server.cc            - Request/notification handlers, lifecycle
  JsonRPC.cc           - Stdio communication, message framing
  DocumentManager.cc   - Parsing, module integration
  SemanticTokens.cc    - Token collection and LSP encoding

src/tools/
  sif_lsp.cc           - Language server executable entry point

support/vscode-extension/
  package.json         - Extension manifest
  language-configuration.json
```

### Current Status Summary

**Fully Working**:
- ✅ Syntax highlighting with context-aware coloring
- ✅ Keywords, operators, strings, numbers, comments properly colored
- ✅ Function names and parameters highlighted distinctly
- ✅ Multi-word function names fully supported
- ✅ String interpolation parsing and highlighting
- ✅ Real-time error display in Problems panel
- ✅ Accurate error locations with red squiggles

**Not Yet Implemented**:
- ⏳ Autocomplete (keyword, function, variable suggestions)
- ⏳ Go to definition
- ⏳ Find references
- ⏳ Hover information
- ⏳ Document symbols
- ⏳ Code actions/formatting

## Next Steps

1. ✅ Create this planning document
2. ✅ Set up project structure for LSP server
3. ✅ Integrate nlohmann/json library
4. ✅ Implement basic LSP message handling
5. ✅ Start with semantic tokens for syntax highlighting
6. ✅ Fix string interpolation Scanner bugs
7. ✅ Add diagnostic publishing
8. ✅ Create VS Code extension for testing
9. ⏳ Add completion providers (Phase 3 - next priority)
10. ⏳ Implement advanced features (Phase 5)

## References

- [LSP Specification](https://microsoft.github.io/language-server-protocol/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [VS Code Extension API](https://code.visualstudio.com/api)
