# Sif Programming Language - Style and Convention Guide

This document outlines the coding standards, style conventions, and best practices for the Sif programming language implementation.

## Table of Contents

1. [Naming Conventions](#naming-conventions)
2. [Code Formatting](#code-formatting)
3. [File Organization](#file-organization)
4. [Memory Management](#memory-management)
5. [Error Handling](#error-handling)
6. [Template Usage](#template-usage)
7. [Comments and Documentation](#comments-and-documentation)
8. [Testing](#testing)
9. [Architecture Patterns](#architecture-patterns)

## Naming Conventions

### Classes and Types

Use **PascalCase** for all classes, structs, and type names:

```cpp
class VirtualMachine;
class Parser;
struct Token;
struct SourceLocation;
enum class Type { Integer, String, Bool };
```

### Functions and Methods

Use **camelCase** for all functions and methods:

```cpp
// Public methods
std::string typeName() const;
bool isEmpty() const;
Value asObject();

// Private methods (with underscore prefix)
void _trace(const std::string &message);
bool _isValidToken() const;
```

### Variables

- **Local variables and parameters**: camelCase
- **Private member variables**: camelCase with underscore prefix
- **Public member variables**: camelCase (no prefix)

```cpp
class Example {
public:
    SourceRange range;        // Public member
    std::string text;         // Public member

private:
    std::string _value;         // Private member
    size_t _index;              // Private member
    std::vector<Value> _values; // Private member
};

void function(const std::string &parameterName) {
    int localVariable = 42;
    auto anotherLocal = getValue();
}
```

### Files and Directories

- **Headers**: PascalCase with `.h` extension
- **Source files**: PascalCase with `.cc` extension
- **Directories**: lowercase with underscores if needed

```
include/sif/runtime/Object.h
src/runtime/Object.cc
src/runtime/objects/String.cc
src/tests/resources/
```

## Code Formatting

### Indentation and Spacing

- Use **4 spaces** for indentation (no tabs)
- Opening braces on the same line for functions and classes
- Consistent spacing around operators and after commas

```cpp
class Object {
public:
    virtual ~Object() = default;

    virtual std::string typeName() const = 0;
    virtual bool equals(Strong<Object> object) const;

private:
    bool _visited = false;
};

if (condition) {
    doSomething();
} else {
    doSomethingElse();
}

auto result = lhs + rhs;
function(arg1, arg2, arg3);
```

### Line Length and Wrapping

- Keep lines reasonable in length
- Break long parameter lists and template declarations across multiple lines
- Align continued lines appropriately

```cpp
// Long parameter lists
Result<Value, Error> subscript(VirtualMachine &vm,
                              SourceLocation location,
                              const Value &value) const;

// Long template declarations
template <class T, class... Args>
std::enable_if_t<!std::is_array<T>::value, std::shared_ptr<T>>
MakeStrong(Args &&...args);
```

## File Organization

### Header Structure

Every header file should follow this structure:

```cpp
#pragma once

#include <sif/Common.h>
// Other includes...

SIF_NAMESPACE_BEGIN

// Content...

SIF_NAMESPACE_END
```

### Include Order

1. **Primary header** (in .cc files)
2. **Sif headers** using angle brackets: `<sif/runtime/Object.h>`
3. **System headers**: `<iostream>`, `<string>`, `<memory>`
4. **Local utility headers** using quotes: `"utilities/strings.h"`

```cpp
#include "sif/runtime/objects/String.h"

#include <sif/runtime/Value.h>
#include <sif/compiler/Parser.h>

#include <iostream>
#include <sstream>
#include <string>

#include "utilities/strings.h"
```

### Directory Structure

Mirror the include and source directory structures:

```
include/sif/
├── Common.h
├── ast/
├── compiler/
├── runtime/
│   ├── Object.h
│   ├── Value.h
│   └── objects/
└── sif.h

src/
├── ast/
├── compiler/
├── runtime/
│   ├── Object.cc
│   ├── Value.cc
│   └── objects/
└── tests/
```

## Memory Management

### Smart Pointer Aliases

Use the project's type aliases for consistent memory management:

```cpp
template <class T> using Strong = std::shared_ptr<T>;
template <class T> using Weak = std::weak_ptr<T>;
template <class T> using Owned = std::unique_ptr<T>;
```

### Factory Functions

Use `MakeStrong` and `MakeOwned` instead of `std::make_shared`/`std::make_unique`:

```cpp
// Preferred
auto object = MakeStrong<String>("hello");
auto parser = MakeOwned<Parser>(config);

// Avoid
auto object = std::make_shared<String>("hello");
```

### Casting

Use the project's `Cast` function for safe downcasting:

```cpp
if (auto string = Cast<String>(object)) {
    // Use string...
}
```

### Memory Safety Guidelines

- Prefer RAII patterns and smart pointers
- Use weak references to break cycles when appropriate
- Avoid raw pointers except for non-owning references

## Error Handling

### Result Type

Use the `Result<T, E>` type for operations that can fail:

```cpp
Result<Value, Error> parseExpression();
Result<Integer, Error> convertToInteger(const std::string &str);

// Usage
if (auto result = parseExpression()) {
    Value value = result.value();
    // Success path
} else {
    Error error = result.error();
    // Error handling
}
```

### Error Construction

Use the templated Error constructor for formatted messages:

```cpp
// With formatting
return Fail(Error(location, "expected {} but got {}",
                  expectedType, actualType));

// Simple message
return Fail(Error(location, "unexpected token"));

// Using error constants
return Fail(Error(location, Errors::ExpectedAnExpression));
```

### Error Constants

Define error messages in the `Errors` namespace:

```cpp
namespace Errors {
    inline constexpr std::string_view ExpectedAnExpression = "expected an expression";
    inline constexpr std::string_view UnknownVariable = "unknown variable";
    inline constexpr std::string_view TypeMismatch = "type mismatch";
}
```

## Template Usage

### Type Aliases

Use type aliases for common template instantiations:

```cpp
using Integer = int64_t;
using Float = double;
using Optional = std::optional;

template <class T, class E>
using Result = tl::expected<T, E>;
```

### Template Functions

Use perfect forwarding for factory functions:

```cpp
template <class T, class... Args>
std::shared_ptr<T> MakeStrong(Args &&...args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}
```

## Comments and Documentation

### Code Comments

- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Prefer self-documenting code over excessive comments
- Comment the "why", not the "what"

```cpp
// Good: Explains the design rationale
void VirtualMachine::trackObject(Strong<Object> object) {
    // Only track container objects that can form cycles
    if (Cast<List>(object) || Cast<Dictionary>(object)) {
        _trackedContainers[object.get()] = object;
    }
}

// Good: Explains performance optimization
if (literal.token.type == Token::Type::IntLiteral) {
    auto value = std::stol(literal.token.text);
    if (value <= USHRT_MAX) {
        // Special case: inline small integers for better performance
        bytecode().add(literal.range.start, Opcode::Short, value);
        return;
    }
}

// Poor: Comments the obvious
size_t hash() const {
    hasher hasher;                          // Create hasher
    hasher.combine(_start, _end, _closed);  // Hash all components
    return hasher.value();                  // Return hash value
}

// Better: Self-documenting code needs no comments
size_t hash() const {
    hasher hasher;
    hasher.combine(_start, _end, _closed);
    return hasher.value();
}
```

### Documentation Comments

Use `///` for API documentation:

```cpp
/// @brief Parse and return a Statement object.
/// @return Returns a strong reference to the Statement object, or nullptr on failure.
Strong<Statement> statement();
```

### Debug Tracing

Use the trace macro for debug output:

```cpp
#if defined(DEBUG)
#define trace(msg) _trace(msg)
#else
#define trace(msg)
#endif

// Usage
trace("Parsing expression: " + token.text);
```

## Testing

### Test Organization

- Group tests by functionality
- Use descriptive test names
- Follow the naming pattern: `feature_tests.cc`

```cpp
TEST_CASE(Parser, ParsesSimpleExpression) {
    // Test implementation
}

TEST_CASE(Parser, HandlesErrors) {
    // Test implementation
}
```

### Test Macros

Use the project's assertion macros:

```cpp
ASSERT_TRUE(condition);
ASSERT_EQ(expected, actual);
ASSERT_FALSE(condition);
```

### Transcript Testing

For language feature testing, use transcript-based tests:

```sif
-- Test description
set variable to "value"
print variable
(-- expect
value
--)
```

## Architecture Patterns

### Protocol Implementation

Use multiple inheritance for protocol implementation:

```cpp
class String : public Object,
               public Enumerable,
               public Subscriptable,
               public Copyable,
               public NumberCastable {
    // Implementation
};
```

### Virtual Methods

- Use `override` keyword for overridden methods
- Prefer pure virtual for abstract base classes
- Use `const` for methods that don't modify state

```cpp
class Object {
public:
    virtual ~Object() = default;
    virtual std::string typeName() const = 0;
    virtual std::string description() const = 0;
};

class String : public Object {
public:
    std::string typeName() const override { return "string"; }
    std::string description() const override;
};
```

### Namespace Usage

Use the project's namespace macros:

```cpp
SIF_NAMESPACE_BEGIN

class MyClass {
    // Implementation
};

SIF_NAMESPACE_END
```

## Tools and Automation

### Code Formatting

The project uses `clang-format` for consistent formatting:

```bash
make format
```

### Build System

Use the provided Makefile targets:

```bash
make debug     # Debug build
make release   # Release build
make test      # Run tests
make clean     # Clean build artifacts
```

This style guide should be followed for all contributions to maintain consistency and readability across the Sif codebase.