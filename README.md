# jbasic

JBasic does not serve any practical purpose. I created this thing as an excercise in programming, because I simply got bored with this quarantine thing. Initially, I wanted to have my own Basic interpreter. Then I started adding more and more unrelated stuff and ended up with this...

Some facts about this project:
 - I always swore at languages that have `then` keyword. Guess what? This one doesn't! :tada:
 - I also prefer C-style logical operators (`&&`, `||`) so they are understood by the interpreter
 - The code can be easily altered to transform JBasic code into portable precompiled binary code
 - Writing this thing in C++ would have saved me a lot of time (due to the amount of boilerplate code)
 - JBasic support tuples and some tuple operations - I guess that's kinda nice
 - ~~I don't even know if it fits into an AVR anymore...~~ Almost certainly it doesn't. Even if it did, it's likey too bloated anyway (it uses floats) 

The state of things:
 - [x] - if statements
 - [x] - while loops
 - [x] - integer/floating point arithmetic
 - [x] - global variables
 - [x] - calling C functions from JBasic code
 - [x] - importing JBasic standard library from a shared library file
 - [x] - arrays
 - [x] - tuples
 - [ ] - functions 
 - [ ] - string operations

Usage: `JBASLIB=stdjbas.so ./jbi FILENAME [-debug]`

### Conclusions
I figured out I will leave it at that - it's just an excercise and not an actual project. I've learnt that creaing a programming language without a plan leads to a big mess. I think that I introduced too many token types - that leads to huge amount of boilerplate code, manual exception handling, and type conversions attempts. OOP would have been certainly helpful in this case. It doesn't mean it can't be done nicely with C, though.

I really like the idea of being able to provide standard library using dynamic linker. It can save a lot of time if you can write those functions in C instead of JBasic. It requires `dlfcn.h` so it works on POSIX  systems, though (I suppose).

Creating this thing has certainly been a very valuable experience for me. Maybe I'll start over someday and correct my mistakes...
