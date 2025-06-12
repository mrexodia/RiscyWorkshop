# Exercise 1: VM Obfuscation Basics

Change the working directory:

```sh
cd exercise_2
```

In this exercise we are going to explore the concept of VM obfuscation.

1. Build `minivm.cpp`:
   ```sh
   clang-20 -O3 -fno-slp-vectorize -std=c++17 minivm.cpp -o minivm
   ```
2. Run the VM (`./minivm 1 2 3 4`) and do the following exercises:
   - Extract the active VM bytecode.
   - How many registers does the VM have?
   - Make a list of all the operations and what they do.
   - What does the active VM bytecode do? Manually decompile it to C.
3. For this exercise you will write your own bytecode. Look at the commented-out example code to figure out how to do it.
   - Implement a function that adds the first two arguments together: `return a + b`
   - Implement a function that multiplies the first two arguments together: `return a * b`
   - Implement a function that implements: `return a - b`
   - Implement a function that implements: `return a == 42 ? 1337 : 0`

If you finish early, here are a few bonus exercises (not required):

1. Extend `minivm.cpp` so it becomes possible to implement `fib(n)` (iterative).
2. Implement the `fib(n)` function to verify your new opcodes.
3. Analyze the `minivm` binary in you favorite disassembler/decompiler. Would it be difficult to write a disassembler for the VM bytecode if the opcodes were switched around?
