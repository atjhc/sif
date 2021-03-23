## Chatter

Chatter is a lightweight scripting language modeled after [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk), Apple's scripting language built for [HyperCard](https://en.wikipedia.org/wiki/HyperCard). This is very much a work in progress; the grammar is not complete, and the runtime is pretty unstable. It's also pretty limited in functionality, and mostly just useful for scratching the itch of nostalgia.

Since you probably just want to see the language, here is a sample chatter script that solves FizzBuzz:
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
./build/chatter <file> [ <args> ... ]
```
By default, this will load the specified file, and send it the `begin` message. You can change the default message using the `-m` flag. Additionally, you may pass along command line arguments to the script.

To get started, here is a simple example:
```
on begin
  put "Hello, World!"
end begin
```
Message handlers can also receive a comma seperated list of arguments:
```
on begin name
  put "Hello, " & name & "!"
end begin
```

## Ok, but why?

When I was young I learned to program by playing around with HyperCard. At first I simply drew pictures and used some of the builtin actions (e.g. changing cards) to make short point-and-click adventure games similar to Myst (indeed, Myst was originally built on top of HyperCard). I gradually began adding more complexity into my games (tracking items, stats, and even building simple animations), and that was truly my entry into a life-long love of programming.

I don't expect this language to be particularly useful to anybody, but I find it appealing as a learning tool due to its very english-like syntax. I always wanted to implement a programming language as an exercise, and HyperTalk seemed like a great way to combine that goal with a feeling of nostalgia and my general love for HyperCard.
