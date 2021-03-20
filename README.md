## What?

Chatter is a lightweight scripting language modeled after [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk), Apple's scripting language built for [HyperCard](https://en.wikipedia.org/wiki/HyperCard). This is currently a passion project for me, and very much in progress. Currently only the parser is working, and only for a subset of the HyperTalk grammar.

## Why?

When I was young I learned to program by playing around with HyperCard. At first I simply drew pictures and used some of the builtin actions (e.g. changing cards) to make short adventure games similar to Myst (indeed, Myst was originally built on top of HyperCard). Eventually I learned the scripting language built into the tool, and that was truly my entry into a life-long love of programming.

I don't expect this language to be particularly useful to anybody, but I find it appealing as a learning tool due to its very english-like syntax. I always wanted to implement a programming language as an exercise, and HyperTalk seemed like a great way to combine that goal with a feeling of nostalgia and general love for HyperCard.

### How?

From Wikipedia's article on [HyperTalk](https://en.wikipedia.org/wiki/HyperTalk):
```
on mouseUp
  put "100,100" into pos
  repeat with x = 1 to the number of card buttons
    set the location of card button x to pos
    add 15 to item 1 of pos
  end repeat
end mouseUp
```
