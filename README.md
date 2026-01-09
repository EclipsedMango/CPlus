## C+ Introduction

-----

**C+** is my solution to C's and C++'s problems. C with quality of life without the bloat of C++

The syntax of **C+** is the same as C, but it will contain features like a string base type, struct impl similar to rust and much more

-----
#### *Warning!!!!*

This is still **WIP** and will be for a while! right now you can only really create functions and variables with returning.

-----
### How to run create and run a C+ program

-----

#### First create your C+ file!

Here's an example program:
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

Save this into a folder call **C+** (If you cloned this repo this will already be there!)

Now time to run some commands!

- Cmd 1:
  - This command compiles the compiler
  - ````gcc main.c lexer/*.c parser/*.c semantic/*.c codegen/*.c -o compiler $(llvm-config --cflags) -lLLVM````

- Cmd 2:
  - This command compiles your **C+** program with the **C+** compiler
  - ````./compiler C+/main.cp````

- Cmd 3:
  - This generates the program binary for you to run
  - ````gcc output.o -o program````

- Cmd 4:
  - Finally run your **C+** program!
  - ````./program````
