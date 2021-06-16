<a href="https://en.wikipedia.org/wiki/Sif"><img src="support/Sif.jpg" width=200></a>

Sif is a scripting language inspired by [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk), [AppleScript](https://en.wikipedia.org/wiki/AppleScript), and [Jinx](https://www.jinx-lang.org).

Since you probably just want to see the language, here is a sample Sif script that solves FizzBuzz:
```
repeat for each x in 1 to 100
  if x mod 15 is 0 then put "FizzBuzz" else
    if x mod 5 is 0 then put "Buzz" else
      if x mod 3 is 0 then put "Fizz" else put x
    end if
  end if
end repeat
```
### How to

Building should work pretty much out of the box:
```sh
make
```
This will produce artifacts in the `build` directory. To run tests:
```sh
make test
```
To run the parser:
```sh
./build/sif <file> [ <args> ... ]
```
