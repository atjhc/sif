# LSP Testing Scripts

This directory contains testing scripts for the Sif Language Server Protocol implementation.

## Scripts

### test_lsp_client.py

Full end-to-end test of the language server, simulating a complete LSP client session.

**Usage:**
```bash
./test_lsp_client.py
```

**What it tests:**
- Server initialization
- Document opening and synchronization
- Semantic tokens request/response
- Server shutdown sequence

### test_semantic_tokens.py

Focused test for semantic token generation with detailed output visualization.

**Usage:**
```bash
./test_semantic_tokens.py
./test_semantic_tokens.py --debug  # Include server debug output
```

**What it tests:**
- Token type mapping (keywords, strings, numbers, comments, etc.)
- Token position encoding (delta encoding)
- Token visualization against source code

## Requirements

- Python 3.6 or higher
- Sif language server built at `build/debug/sif_lsp`

To build the server:
```bash
cd <repo-root>
make debug
```

## Running Tests

The scripts automatically find the repository root and language server binary, so they can be run from anywhere:

```bash
# From the repository root
./scripts/lsp/test_lsp_client.py

# From within the scripts directory
cd scripts/lsp
./test_lsp_client.py
```

## C++ Unit Tests

In addition to these Python integration tests, there are C++ unit tests in `src/tests/lsp_tests.cc` that test:

- Document manager operations
- Semantic token encoding
- Token type mapping
- Multiple document handling

Run C++ tests with:
```bash
make test
# Or run specific LSP tests
./build/debug/sif_tests -g LSPTests
```

## Test Output

The scripts provide detailed output including:
- Request/response messages
- Decoded token information
- Server debug output (stderr)
- Test results and assertions

Example output:
```
Semantic Tokens Analysis
======================================================================

Test Content:
----------------------------------------------------------------------
   0: -- Comment
   1: function greet {name}
   2:     print "Hello, {name}!"
   3: end function

Decoded Tokens:
----------------------------------------------------------------------
Line  Col  Len | Type         | Text
----------------------------------------------------------------------
   0    0   11 | comment      | '-- Comment'
   1    0    8 | keyword      | 'function'
   1   18    1 | operator     | '{'
   1   19    4 | word         | 'name'
...
```

## Troubleshooting

**Language server not found:**
- Make sure you've built the debug version: `make debug`
- Check that `build/debug/sif_lsp` exists

**No response from server:**
- Check server stderr output for errors
- Try running with `--debug` flag to see verbose output
- Make sure test file exists (for test_lsp_client.py)

**Token mismatch errors:**
- This usually indicates a scanner or semantic tokens bug
- Check the decoded token positions against the source
- Run C++ unit tests to isolate the issue
