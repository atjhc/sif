<a href="https://en.wikipedia.org/wiki/Sif"><img src="support/Sif.jpg" width=200></a>

### Intro

Sif is a scripting language inspired by [NLPs](https://en.wikipedia.org/wiki/Natural-language_programming) like [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk), [AppleScript](https://en.wikipedia.org/wiki/AppleScript), and [Jinx](https://www.jinx-lang.org).

Since you probably just want to see the language, here is a sample Sif script:

```
function the factorial of {x}
  if x = 0 or x = 1 then return 1
  return x * the factorial of x - 1
end function
print the factorial of 10
```

For a comprehensive introduction to the language, see the [Introduction to Sif](docs/introduction.md). The language is currently considered a prototype. You should be able to play around with it, but you'll likely encounter unexpected bugs.

### How to

For most POSIX systems, building should work pretty much out of the box:

```sh
make
```

This will produce release artifacts in the `build` directory. To run tests:

```sh
make test
```

To run a script:

```sh
./build/release/sif_tool <file> [ <args> ... ]
```
