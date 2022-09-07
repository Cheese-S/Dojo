
<p align="center">
  <img width="256" height="256" src="./dojo.png">
  <h1 align="center">Dojo</h1>
</p>

## What is Dojo?
 
Dojo is a dynamic scripting language that runs on a bytecode machine. As long as the platform can compile C, it can also run DOJO!
 
## Why Dojo?
 
As the name suggests, Dojo is a product of learning and it should be used for similar reasons as well.
 
The implementation of Dojo is written with clarity in mind. It's neatly divided into five distinct parts, **scanner**, **parser**, **compiler**, **garbage collector**, and **virtual machine**. Here is a basic rundown of how each part works.
 
- Scanner
    - Consume the source file and spit out a chain of tokens
- Parser
    - Consume the tokens and spit out a chain of AST nodes
- Compiler
    - Consume the AST nodes and spit out the corresponding bytecodes
- Virtual Machine
    - Consume the bytecodes and execute the users' program
- Garbage Collector
    - Free resources that are no longer needed during runtime

I hope Dojo can give you a basic idea of how a programming language can be created.
 
Note: The frontend (**Scanner** and **Parser**) is completely separate from the backend. One can write a backend that compiles the AST into assembly or machine code.
 
### Features
 
Dojo supports most things that a user might expect from a modern language. Most importantly, it supports Object Oriented programming by allowing classes as well as functional programming by allowing closures and having functions as first-class objects.
 
See examples in the test/examples folder for more details.
```
// Classic Hello World in dojo
print("Hello World")
```

### Internals
For people who want to understand the internals of Dojo. Here are the things that you should pay close attention to.
#### **Replace semicolons with newlines**

Converting a C-style language that relies on ';' to identify the end of a statement to a modern language that no longer requires ';' is actually non-trivial. 

Consider the following examples:

```C
while (true) {
    doSomething();
    doAnotherThing();
}

while (true) { doSomething(); doAnotherThing(); }
```
With the help of our handy semicolons, it's easy to tell each statement apart. One might say that we can do the same thing except we swap out semicolons and newlines. While this is partially correct, this also gives rise to new edge cases. For example
```C
// A
while (true) {
    doSomething();
}
// B
while (true) { doSomething() }
// C
while (true) { doSomething() 
}
// D
doSomething(
    true,
    false
)
```
Which one is correct and which one is wrong? For Dojo, I deemed there needs to be a newline character after every statement. Therefore, A would be *valid* and B would be *invalid*. C is valid even though it's quite ugly. For D, I took the same approach as GO; I consider that is a compiler error.

There is one more thing, one newline character is meaningful, are multiple consecutive newline characters meaningful? If a file starts with 10000 (an arbitrary number) empty lines, should we allocate new tokens for all 10000 newlines? My answer is no. So, each statement only expects one newline character instead of an *x* amount of newline characters.

#### **Pratt Parser**
Pratt Parser is an algorithm that significantly reduces the complexity of designing and implementing a language's grammar. It differs from a traditional recursive descent parser in how it calls different parsing functions. 

I recommend these two articles to help you understand the ins and outs of pratt parser.

https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html

https://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/

#### What to Improve?
Besides adding more features, one thing I can immediately think of is how ugly our AST node struct is. Some fields are multi-purpose and therefore have bad names to indicate what is really going on. I would suggest you to clean it up using union to save extra space and clearly label each field.
 

## Build
 
**Note: the commands listed below should be run at the project root directory**
 
Build Dojo by
```
make build
```
 
Run the tests by
```
make test
```
 
## Credit
This project is inspired by Lox, the language described in *Crafting Interpreters*, and Typescript.
I highly recommend reading *Crafting Interpreters* if you haven't already.
 
