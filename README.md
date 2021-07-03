<a href="https://en.wikipedia.org/wiki/Sif"><img src="support/Sif.jpg" width=200></a>

Sif is a scripting language inspired by [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk), [AppleScript](https://en.wikipedia.org/wiki/AppleScript), and [Jinx](https://www.jinx-lang.org).

Since you probably just want to see the language, here is a sample Sif script:
```
function factorial {x}
  if x = 0 or x = 1 then return 1
  return x * factorial x - 1
end function
print factorial 10
```
More thorough documentation is forthcoming. The language is currently in a very unstable pre-alpha stage. You should be able to play around with it, but you'll likely encounter unexpected bugs.

### How to

Building should work pretty much out of the box:
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
