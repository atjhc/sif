# Introduction to Sif

Sif is a toy natural language scripting language that emphasizes readability and intuitive syntax. Inspired by languages like HyperTalk and AppleScript, Sif allows you to write code that reads almost like English prose, making it accessible to both programmers and non-programmers alike.

## Getting Started

### Your First Sif Program

```sif
-- This is a comment in Sif
print "Hello, World!"
```

Let's break this down:
- Comments start with `--` and continue to the end of the line
- The `print` function outputs text followed by a newline
- Strings are enclosed in double quotes

### Variables and Assignment

In Sif, you assign values to variables using natural language:

```sif
set name to "Alice"
set age to 30
set isStudent to yes

print "Name: {name}"
print "Age: {age}"
print "Student: {isStudent}"
```

Notice how:
- Variables are assigned with `set variable to value`
- String interpolation uses `{variable}` syntax
- Boolean values can be `yes`/`no`, `true`/`false`, or `True`/`False`

## Data Types

### Basic Types

**Numbers:**
```sif
set count to 42
set price to 19.99
set percentage to 0.85
```

**Strings:**
```sif
set message to "Hello, World!"
set name to "Sif"
set greeting to "Welcome to {name}!"  -- String interpolation
```

**Booleans:**
```sif
set isComplete to yes    -- or true, True, YES
set isActive to no       -- or false, False, NO
```

**Empty Values:**
```sif
set result to empty
if result = empty then
    print "No result yet"
end if
```

### Collections

**Lists:**
```sif
set fruits to ["apple", "banana", "cherry"]
set numbers to [1, 2, 3, 4, 5]
set mixed to [1, "hello", yes, empty]

-- Accessing items
print item 0 in fruits           -- "apple"
print the first item of fruits   -- "apple"
print the last item of numbers   -- 5
```

**Dictionaries:**
```sif
set person to ["name": "Alice", "age": 30, "city": "Portland"]

-- Accessing values
print item "name" in person      -- "Alice"
print person["age"]              -- 30

-- Getting all keys or checking existence
print sort the keys of person    -- age city name
print person contains "email"    -- no
```

**Ranges:**
```sif
set range1 to 1...10      -- Inclusive range: 1, 2, 3, ..., 10
set range2 to 1..<10      -- Exclusive range: 1, 2, 3, ..., 9

print 5 is in range1      -- yes
print the size of range1  -- 10
```

## Control Flow

### Conditional Statements

Sif supports natural language conditionals:

```sif
-- Simple if statement
if age >= 18 then
    print "You are an adult"
end if

-- If-else
if temperature > 30 then
    print "It's hot!"
else
    print "It's not that hot"
end if

-- Single-line if
if name = "Alice" then set greeting to "Hello, Alice!"
```

### Loops

**Repeat Loops:**
```sif
-- Infinite loop (use with exit repeat)
repeat
    set input to read a line
    if input = "quit" then exit repeat
    print "You said: {input}"
end

-- Conditional loops
set count to 0
repeat while count < 10
    print count
    set count to count + 1
end

set done to no
repeat until done = yes
    -- do something
    set done to yes
end
```

**For-Each Loops:**
```sif
-- Iterate over a list
repeat for fruit in ["apple", "banana", "cherry"]
    print "I like {fruit}"
end

-- Iterate over a range
repeat for i in 1...5
    print "Number: {i}"
end

-- Iterate over dictionary keys
repeat for key in the keys of person
    print "{key}: {item key in person}"
end
```

**Loop Control:**
```sif
repeat for i in 1...10
    if i = 3 then next repeat     -- continue to next iteration
    if i = 8 then exit repeat     -- break out of loop
    print i
end
-- Prints: 1, 2, 4, 5, 6, 7
```

## Functions

Functions in Sif use natural language syntax and can be called in multiple ways:

```sif
function greet {name}
    print "Hello, {name}!"
end function

function add {a} and {b}
    return a + b
end function

function the square of {number}
    return number * number
end function

-- Calling functions
greet "Alice"                   -- Hello, Alice!
print add 5 and 3               -- 8
print the square of 4           -- 16
```

**Optional Words:**
```sif
function (the) factorial of {n}
    if n <= 1 then return 1
    return n * factorial of n - 1
end function

-- Both of these work:
print factorial of 5            -- 120
print the factorial of 5        -- 120
```

## The Special `it` Variable

Sif automatically stores the result of the last expression in a special variable called `it`:

```sif
5 + 3
print it                        -- 8

read a line
print "You entered: {it}"

the square of 6
print "The result was {it}"     -- The result was 36
```

## String Operations

### String Interpolation

Sif supports rich string interpolation with expressions:

```sif
set x to 10
set y to 5
print "The sum of {x} and {y} is {x + y}"
-- Output: The sum of 10 and 5 is 15

-- Escape braces to prevent interpolation
print "Use \{variable\} for interpolation"
-- Output: Use {variable} for interpolation
```

### String Methods

```sif
set text to "Hello, World!"

-- Length and basic info
print the size of text          -- 13
print text contains "World"     -- yes
print text starts with "Hello"  -- yes
print text ends with "!"        -- yes

-- Finding substrings
print the offset of "World" in text         -- 7
print the last offset of "o" in text        -- 8

-- Splitting by delimiter (see String Parsing and Manipulation for details)
print items of "a,b,c"                      -- ["a", "b", "c"]
print item 1 of "a,b,c"                     -- "b"
```

### String Parsing and Manipulation

**Splitting Strings with Items**

Inspired by HyperTalk, Sif provides `item` syntax for splitting strings by delimiters. By default, strings are split on commas:

```sif
-- Default comma delimiter
set csv to "Alice,Bob,Charlie"
print item 0 of csv                    -- "Alice"
print item 2 of csv                    -- "Charlie"
print items of csv                     -- ["Alice", "Bob", "Charlie"]
print items 0 to 1 of csv              -- ["Alice", "Bob"]

-- Custom delimiter
set path to "usr/local/bin"
print item 0 of path using delimiter "/"       -- "usr"
print item 2 of path using delimiter "/"       -- "bin"
print items of path using delimiter "/"        -- ["usr", "local", "bin"]

-- Multi-character delimiters
set data to "name::Alice::age::30"
print items of data using delimiter "::"       -- ["name", "Alice", "age", "30"]

-- Edge cases
print items of ""                              -- []
print items of "single"                        -- ["single"]
print items of "a,b,"                          -- ["a", "b", ""]
```

**Converting Between Types**
```sif
-- Number to string and back
set num to 42
set str to num as a string                  -- "42"
set back to str as an integer               -- 42

-- String to list of characters
set text to "Hello"
set chars to the list of characters in text -- ["H", "e", "l", "l", "o"]

-- Join characters back into string
set result to ""
repeat for char in chars
  set result to result + char
end repeat
-- result is "Hello"

-- String to list of words
set sentence to "Hello wonderful world"
set words to the list of words in sentence  -- ["Hello", "wonderful", "world"]

-- String to list of lines
set multiline to "Line 1\nLine 2\nLine 3"
set lines to the list of lines in multiline -- ["Line 1", "Line 2", "Line 3"]
```

**Character Codes**
```sif
-- Convert between characters and their numeric codes
print ord of "A"                -- 65
print char of 65                -- "A"

print ord of "💐"               -- 128144
print char of 128144            -- "💐"
```

**String Building Patterns**
```sif
-- Build strings incrementally
set result to ""
repeat for i in 1...5
  set result to result + (i as a string) + " "
end repeat
print result                    -- "1 2 3 4 5 "

-- Concatenate with operator
set firstName to "Alice"
set lastName to "Smith"
set fullName to firstName + " " + lastName
print fullName                  -- "Alice Smith"
```

## Collection Operations

### List Operations

```sif
set numbers to [1, 2, 3]

-- Adding items
insert 0 at the beginning of numbers    -- [0, 1, 2, 3]
insert 4 at the end of numbers          -- [0, 1, 2, 3, 4]
insert 1.5 at index 2 into numbers      -- [0, 1, 1.5, 2, 3, 4]

-- Removing items
remove the first item from numbers      -- [1, 1.5, 2, 3, 4]
remove the last item from numbers       -- [1, 1.5, 2, 3]
remove item 1 from numbers              -- [1, 2, 3]

-- Other operations
reverse numbers                         -- [3, 2, 1]
sort numbers                            -- [1, 2, 3]
print the size of numbers               -- 3
```

### Dictionary Operations

```sif
set config to ["debug": yes, "port": 8080]

-- Adding/updating
set config["host"] to "localhost"
insert item "production" with key "environment" into config

-- Checking and accessing
if config contains "debug" then
    print "Debug mode: {config["debug"]}"
end if

print the keys of config           -- debug port host environment
```

## Slicing and Subscripting

Sif provides multiple ways to extract portions of strings and lists. Choose the style that feels most natural for your code.

### List Slicing

**Method 1: Range Subscripts** (Most Concise)
```sif
set numbers to [10, 20, 30, 40, 50]

set slice1 to numbers[1...3]        -- [20, 30, 40] (inclusive)
set slice2 to numbers[0..<3]        -- [10, 20, 30] (exclusive end)
set slice3 to numbers[2...4]        -- [30, 40, 50]
```

**Method 2: Natural Language**
```sif
set numbers to [10, 20, 30, 40, 50]

set slice to items 1 to 3 in numbers    -- [20, 30, 40]
set first3 to items 0 to 2 in numbers   -- [10, 20, 30]
```

### String Slicing

**Method 1: Range Subscripts** (Most Concise)
```sif
set text to "Hello World"

set hello to text[0...4]            -- "Hello"
set world to text[6...10]           -- "World"
set first5 to text[0..<5]           -- "Hello"
```

**Method 2: Natural Language**
```sif
set text to "Hello World"

set hello to characters 0 to 4 in text      -- "Hello"
set world to characters 6 to 10 in text     -- "World"
```

### Common Slicing Patterns

```sif
set myList to [1, 2, 3, 4, 5]
set myString to "abcdef"

-- Get all but last element
set allButLast to myList[0..< (the size of myList) - 1]

-- Get last N elements
set last3 to myList[(the size of myList) - 3 ... (the size of myList) - 1]

-- Split string in half
set halfway to (the size of myString) / 2
set firstHalf to myString[0..< halfway]
set secondHalf to myString[halfway...(the size of myString) - 1]

-- Get middle element
set middle to myList[(the size of myList) / 2]
```

### Understanding Range Operators

Sif supports multiple range operators for different use cases:

```sif
-- Inclusive range (both ends included)
repeat for i in 1...5         -- 1, 2, 3, 4, 5
  print i
end repeat

-- Exclusive range (end not included)
repeat for i in 0..<5         -- 0, 1, 2, 3, 4
  print i
end repeat

-- Both work with subscripts
set list to [10, 20, 30, 40, 50]
print list[0...2]             -- [10, 20, 30]
print list[0..<3]             -- [10, 20, 30]
```

## Debugging and Development

### The REPL

Sif includes an interactive Read-Eval-Print Loop for experimentation and debugging. Launch it by running `sif_tool` without any arguments:

```bash
$ sif_tool
>
```

In the REPL, you can:
- Test expressions interactively
- Inspect variable values
- Experiment with language features
- Quickly prototype solutions

```sif
> set x to 42
> print x * 2
84
> set words to the list of words in "Hello World"
> print the description of words
["Hello", "World"]
```

### Error Logging

Sif provides two output streams following standard Unix conventions:

```sif
-- Standard output (for program results)
print "Result: {answer}"

-- Standard error (for error messages and diagnostics)
print error "Error: Invalid input value"
print error "Warning: Configuration file not found"
```

Example error handling with logging:
```sif
function process file {filename}
  try
    set contents to the contents of file filename
    return contents
  end try

  if the error is not empty then
    print error "Error reading file '{filename}': {the error}"
    return empty
  end if
end function

set data to process file("config.txt")
if data is not empty then
  print "Successfully loaded configuration"
else
  print "Failed to load configuration"
end if
```

Stream redirection:
```bash
# Save only results to a file
sif_tool myprogram.sif 2>/dev/null > results.txt

# Save only errors to a log file
sif_tool myprogram.sif 2>errors.log

# Redirect both streams separately
sif_tool myprogram.sif 1>output.txt 2>errors.log
```

## Type System and Conversion

Sif has a dynamic type system with explicit type checking and conversion:

```sif
-- Type checking
print 42 is an integer             -- yes
print "hello" is a string          -- yes
print [1, 2, 3] is a list          -- yes

-- Type names
print the type name of 42          -- "integer"
print the type name of "hello"     -- "string"

-- Type conversion
print "123" as an integer          -- 123
print 42 as a string               -- "42"
print yes as a string              -- "yes"
```

## Input and Output

### Output

```sif
print "Hello"                      -- Prints with newline
write "Hello"                      -- Prints without newline
print "Count: {count}"             -- With interpolation
```

### Input

```sif
print "What's your name?"
set name to read a line
print "Hello, {name}!"

print "Enter a number:"
set number to read a word as an integer
print "Double that is {number * 2}"
```

## Error Handling

Sif provides simple error handling with try blocks:

```sif
try
    set result to 10 / 0           -- This will cause an error
end try

if the error is not empty then
    print "An error occurred: {the error}"
end if

-- You can also create custom errors
if age < 0 then
    error with "Age cannot be negative"
end if
```

## Modules

You can organize code into modules and import them:

```sif
-- math_utils.sif
function the square of {n}
    return n * n
end function

function the cube of {n}
    return n * n * n
end function
```

```sif
-- main.sif
use "math_utils.sif"

print the square of 5              -- 25
print the cube of 3                -- 27
```

## Advanced Features

### Variable Scoping

```sif
set global counter to 0            -- Explicit global

function increment
    set local temp to 5            -- Explicit local
    set global counter to counter + 1     -- Accesses global counter
end function
```

### Unicode Support

Sif fully supports Unicode in variable names and strings:

```sif
set 名前 to "Alice"
set 年齢 to 30
print "こんにちは, {名前}!"
```

### Case Insensitivity

All identifiers in Sif are case-insensitive:

```sif
set myVariable to 5
print MyVariable                   -- 5
print MYVARIABLE                   -- 5
```

## Example Programs

### Simple Calculator

```sif
print "Simple Calculator"
print "Enter two numbers:"

set a to read a line as a number
set b to read a line as a number

print "Results:"
print "{a} + {b} = {a + b}"
print "{a} - {b} = {a - b}"
print "{a} * {b} = {a * b}"
if b != 0 then print "{a} / {b} = {a / b}"
```

### Fibonacci Sequence

```sif
function the fibonacci number {n}
    if n <= 1 then return n
    return (the fibonacci number n - 1) + (the fibonacci number n - 2)
end function

print "First 10 Fibonacci numbers:"
repeat for i in 0..<10
    print "F({i}) = {the fibonacci number i}"
end
```

### Word Counter

```sif
print "Enter some text:"
set text to read a line

set words to the list of words in text
set wordCount to number of words in text

print "You entered {wordCount} words:"
repeat for word in words
    print "- {word}"
end
```
