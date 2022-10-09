<a href="https://en.wikipedia.org/wiki/Sif"><img src="support/Sif.jpg" width=200></a>

### Intro

Sif is a scripting language inspired by [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk), [AppleScript](https://en.wikipedia.org/wiki/AppleScript), and [Jinx](https://www.jinx-lang.org).

Since you probably just want to see the language, here is a sample Sif script:

```
function the factorial of {x}
  if x = 0 or x = 1 then return 1
  return x * the factorial of x - 1
end function
print the factorial of 10
```

More thorough documentation is forthcoming. The language is currently considered a prototype. You should be able to play around with it, but you'll likely encounter unexpected bugs.

### How to

For most POSIX systems, building should work pretty much out of the box:

```sh
make
```

This will produce artifacts in the `build` directory. To run tests:

```sh
make test
```

To run a script:

```sh
./build/sif <file> [ <args> ... ]
```

### Quick Tour

```
-- Print a line of text using the "print" function:
print "Hello, World!"

-- Print text, without a newline, using the "write" function:
write "Type something: "

-- Read a line of text (note: "a" is optional):
read a line

-- The value is automatically captured as the variable "it":
print it

-- Though less idiomatic, you may also compose these functions:
write "Type something: "
print read line

-- Set a variable using the "set" command:
set firstName to "Hari"

-- Variable names may be non-ASCII characters:
set 名前 to "Hari"

-- "if" statements can be a single line:
if firstName = "Hari" then set lastName to "Seldon"

— ...or span multiple lines as a block:
if firstName = "Hari" then
  set lastName to "Seldon"
  set nickname to "Raven"
end if

-- You can print multiple things using commas:
print "Welcome,", firstName, nickname, lastName

-- Use "repeat" to repeat a block of code:
print "Ask a question: "
repeat
  write "> "
  read a line
  set input to it
  if input = empty then exit repeat
  else print "I don't understand", input
end repeat

-- Functions may have multiple words in their name:
function the password
  return "12345"
end function

write "Password: "
read a line
if it = the password then print "Correct"
else print "Incorrect, but trusting you for now..."
```