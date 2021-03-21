## What?

Chatter is a lightweight scripting language modeled after [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk), Apple's scripting language built for [HyperCard](https://en.wikipedia.org/wiki/HyperCard). This is very much a work in progress, as currently only the parser is working, and only for a subset of the HyperTalk grammar. Support is planned for a runtime and lightweight frontend (which will handle common commands like `put`, `get`, `ask`, `answer`, etc.).

Here is a sample chatter script that solves FizzBuzz:
```
on begin
  repeat with x = 1 to 100
    if x mod 15 is 0 then put "FizzBuzz" else
      if x mod 5 is 0 then put "Buzz" else
        if x mod 3 is 0 then put "Fizz" else put x
      end if
    end if
  end repeat
end begin
```

### How?

Building should work pretty much out of the box:
```
make
```
This will produce artifacts in the `build` directory. To run tests:
```
make tests
```
To run the parser:
```
./build/chatter -p <file>
```
The `-p` option pretty prints the AST, which is all it can do so far.

## Why?

When I was young I learned to program by playing around with HyperCard. At first I simply drew pictures and used some of the builtin actions (e.g. changing cards) to make short adventure games similar to Myst (indeed, Myst was originally built on top of HyperCard). Eventually I learned the scripting language built into the tool, and that was truly my entry into a life-long love of programming.

I don't expect this language to be particularly useful to anybody, but I find it appealing as a learning tool due to its very english-like syntax. I always wanted to implement a programming language as an exercise, and HyperTalk seemed like a great way to combine that goal with a feeling of nostalgia and general love for HyperCard.
