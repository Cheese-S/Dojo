# Dojo
 
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
 
