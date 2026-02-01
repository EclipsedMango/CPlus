## C+ Introduction

-----

**C+** is my solution to C's and C++'s problems.

The syntax of **C+** is similar to C, but it includes features like: 
- Built-in string type 
- Struct implementations (similar to Rust)  
- And much more to come!

-----
### Warning!!!
This project is a **work in progress!** Currently supported features:
- Function declarations and calls
- Variable declarations with initizlization
- Basic arithmetic and comparison operations
- If statements
- while statements
- for statements
- continue statements
- break statements
- return statments
- inline ASM using c like syntax and intel asm dialect
- pointers and addresses, returning pointers
- string, long, int, char, float, double, bool and void types
- comments
- arrays and arrays decaying to pointers
- scope defined variables and global variables
- const keyword, that makes variables immutable
- #define directive (which acts as constexpr)
- #include directive
- basic casting
- incrementing, decrementing, +=, -=, *=, /=, %= expressions
- builtin functions

-----
### Getting started
#### What you will need:
- GCC compiler
- LLVM development libraries
- ````llvm-config```` in your PATH

-----

### Creating your first C+ program
#### Firstly create your C+ file!

Create a file called main.cp in the C+ directory: (If you cloned this repo this will already be there under the name test.cp!)
````
int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 4;
    int y = add(x, 4);
    return y;
}
````
#### Compiling and running

- **Step 1:** Compile the C+ compiler
  - ````./compile.sh````

- **Step 2:** Compile your program, link your program and then run your program
  - ````./compiler examples/test.cp && gcc output.o -o program && ./program````

- **Step 3:** Celebrate!
  - if done correctly everything should work! and now you have a **C+** program!
