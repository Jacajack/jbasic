# jbasic

JBasic does not serve any practical purpose. I created this thing as an excercise in programming, because I simply got bored with this quarantine thing. Initially, I wanted to have my own Basic interpreter. Then I started adding more and more unrelated stuff and ended up with this...

Some facts about this project:
 - I always swore at languages that have `then` keyword. Guess what? This one doesn't! :tada:
 - I also prefer C-style logical operators (`&&`, `||`) so they are understood by the interpreter
 - The code can be easily altered to transform JBasic code into portable precompiled binary code
 - Writing this thing in C++ would have saved me a lot of time (due to the amount of boilerplate code)
 - JBasic support tuples - I guess that's kinda nice
 - ~~~I don't even know if it fits into an AVR anymore...~~~ Almost certainly it doesn't. Even if it did, it's likey too bloated anyway (it uses floats) 

The state of thing:
 - [x] - if statements
 - [x] - while loops
 - [x] - integer/floating point arithmetic
 - [x] - global variables
 - [x] - calling C functions from JBasic code
 - [ ] - arrays
 - [ ] - functions 
 - [ ] - string operations

It's a bit messy, but I guess it works... at least to certain extent... :')
