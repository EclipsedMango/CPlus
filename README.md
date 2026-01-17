## C+ Introduction

-----

**C+** is my solution to C's and C++'s problems. C with quality of life without the bloat of C++

The syntax of **C+** is similar to C, but it includes features like: 
- Built-in string type 
- Struct implementations (similar to Rust)  
- And much more to come!

-----
#### *⚠️ Warning*
This project is a **work in progress!** Currently supported features:
- Function declarations and calls
- Variable declarations with initizlization
- Basic arithmetic and comparison operations
- If statments
- return statments

-----
### Getting started
#### What you will need:
- GCC compiler
- LLVM development libraries
- llvm-config in your PATH

-----

### Creating your first C+ program
#### Firstly create your C+ file!

Create a file called main.cp in the C+ directory: (If you cloned this repo this will already be there!)
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

- **Step 1:** Compile the C+ compiler and compile your C+ program
  - ````gcc main.c common.c lexer/*.c parser/*.c semantic/*.c codegen/codegen.c -o compiler $(llvm-config --cflags) -lLLVM && ./compiler C+/main.cp````

- **Step 2:** Link and run your program
  - ````gcc output.o -o program && ./program````

- **Step 3:** Celebrate!
  - if done correctly everything should work! and now you have a **C+** program!
