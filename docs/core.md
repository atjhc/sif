Core
====

1. [the language version](#the-language-version)
2. [the language major version](#the-language-major-version)
3. [the language minor version](#the-language-minor-version)
4. [the language patch version](#the-language-patch-version)
5. [quit](#quit)
6. [quit with {}](#quit-with-)
7. [print {}](#print-)
8. [print error {}](#print-error-)
9. [write {}](#write-)
10. [write error {}](#write-error-)
11. [get {}](#get-)
12. [(the) type name (of) {}](#the-type-name-of-)
13. [(a) copy (of) {}](#a-copy-of-)
14. [(the) description of {}](#the-description-of-)
15. [(the) hash value of {}](#the-hash-value-of-)
16. [read (a) word](#read-a-word)
17. [read (a) line](#read-a-line)
18. [read (a) character](#read-a-character)

the language version
--------------------

### Usage

    the language version

### Description
Returns the current version of the Sif language as a string.


the language major version
--------------------------

### Usage

    the language major version

### Description
Returns the current major version of the Sif language as an integer.


the language minor version
--------------------------

### Usage

    the language minor version

### Description
Returns the currentminor  version of the Sif language as an integer.


the language patch version
--------------------------

### Usage

    the language patch version

### Description
Returns the current patch version of the Sif language as an integer.


quit
----

### Usage

    quit

### Description
Quit the program successfully.


quit with {}
------------

### Usage

    quit with errorCode

**errorCode** may be an integer.

### Description
Quit the program using **errorCode** as the status code.


print {}
--------

### Usage

    print value

**value** may be anything.

### Description
Prints a value to stdout, followed by a new line character.


print error {}
--------------

### Usage

    print error value

**value** may be anything.

### Description
Print a value to stderr, followed by a new line character.


write {}
--------

### Usage

    write value

**value** may be anything.

### Description
Write a value to stdout.


write error {}
--------------

### Usage

    wrote error value

**value** may be anything.

### Description
Write a value to stderr.


get {}
------

### Usage

    get value

**value** may be anything.

### Examples

    get the language version
    print it

### Description
Evaluates to **value**. This function has no affect on the value it returns, but may be used to make statements read more naturally.


(the) type name (of) {}
-----------------------

### Usage

    the type name of value

**value** may be anything.

### Description
This function evaluates to the type name of **value** as a string.


(a) copy (of) {}
----------------

### Usage

    a copy of value

**value** may be anything.

### Examples

    a copy of [1, 2, 3]
    a copy of "Hello, World!"

### Description
Evaluates to a copy of **value**.


(the) description of {}
-----------------------

### Usage

    the description of value

**value** may be anything.

### Examples

    the description of 1
    the description of [1, 2, 3]
    the description of ["one": 1, "two": 2]

### Description
Evaluates to a string describing **value**.


(the) hash value of {}
----------------------

### Usage

    the hash value of value

**value** may be anything.

### Examples

    the hash value of 42
    the hash value of "Hello, World!"
    the hash value of [1, 2, 3]

### Description
Evaluates to an integer representing a has of **value**.


read (a) word
-------------

### Usage

    read a word

### Description
Evaluates to a string read from stdin. This command will read one word from stdin, skipping whitespaces. A whitespace is defined as one of the following ASCII characters:
- space (0x20, " ")
- form feed (0x0c, "\f")
- line feed (0x0a, "\n")
- carriage return (0x0d, "\r")
- horizontal tab (0x09, "\t")
- or vertical tab (0x0b, "\v")


read (a) line
-------------

### Usage

    read a line

### Description
Evaluates to a string read from stdin. This command will read one whole line from stdin, stopping once the line feed (0x0d, "\n") character is encountered. The line feed character will be left in stdin.


read (a) character
------------------

### Usage

    read a character

### Description
Evaluates to a string read from stdin. This command will read exactly one UTF8-encoded character from stdin.
