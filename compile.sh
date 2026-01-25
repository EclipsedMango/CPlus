#!/bin/bash
echo "Compiling compiler with compiler..."
echo "Finding LLVM version..."

# get ldflags/libs for core and capture output; fail if llvm-config fails
libs_output=$(llvm-config --ldflags --libs core 2>/dev/null)
if [ $? -ne 0 ] || [ -z "$libs_output" ]; then
  echo "Error: llvm-config failed to provide ldflags/libs for core. Make sure llvm-config is installed and on PATH."
  exit 1
fi

# extract the LLVM library token (e.g. -lLLVM-14 or -lLLVM-15)
llvm_lib=$(echo "$libs_output" | tr ' ' '\n' | grep -E '^-lLLVM' | head -n 1)
if [ -z "$llvm_lib" ]; then
  echo "Error: could not find -lLLVM... token in llvm-config output."
  exit 1
fi

echo "Using: $llvm_lib"

gcc src/*.c src/codegen/codegen.c src/lexer/*.c src/parser/*.c src/semantic/*.c src/util/*.c -o compiler $(llvm-config --cflags) $llvm_lib
echo "Compiler successfully compiled!"
echo "Run with ./compiler <file-to-compile> [flags]"
