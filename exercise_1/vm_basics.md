# Exercise 1: VM Obfuscation Basics

Change the working directory:

```sh
cd exercise_1
```

In this exercise we are going to explore the concept of VM obfuscation.

1. Build `minivm.cpp`:
   ```sh
   clang-20 -O3 -fno-slp-vectorize -std=c++17 minivm.cpp -o minivm
   ```
2. Run the VM (`./minivm 1 2 3 4`) and do the following exercises:
   - Extract the active VM bytecode (look at the source code).
```cpp
constexpr uint8_t bytecode1[] = {
    OR(REG(4), REG(0), REG(1)),
    XOR(REG(5), REG(2), REG(3)),
    ADD(REG(6), REG(4), REG(5)),
    RET(REG(6)),
};
```
   - How many registers does the VM have?
Answer: 256
   - Make a list of all the operations and what they do.
```cpp
// The opcode is the index into this handlers array
static VMHandler handlers[] = {
    handler_label, // does nothing
    handler_ret, // returns value of op1 and stops VM
    handler_add, // dst = op1 + op2
    handler_movimm, // dst = imm
    handler_cmp, // dst = op1 == op2
    handler_jcc, // if(op1) { pc = op2 }
    handler_xor, // dst = op1 ^ op2
    handler_or, // dst = op1 | op2
    handler_mul, // dst = op1 * op2
};
```
   - What does the active VM bytecode do? Write it as C pseudocode.
```cpp
constexpr uint8_t bytecode1[] = {
    OR(REG(4), REG(0), REG(1)),
    XOR(REG(5), REG(2), REG(3)),
    ADD(REG(6), REG(4), REG(5)),
    RET(REG(6)),
};

uint64_t do_bytecode1(uint64_t r0, uint64_t r1, uint64_t r2, uint64_t r3)
{
    uint64_t r4 = r0 | r1;
    uint64_t r5 = r2 ^ r3;
    uint64_t r6 = r4 + r5;
    return r6;
}
```
3. For this exercise you will write your own bytecode. Look at the commented-out example code to figure out how to do it.
   - Implement a function that adds the first two arguments together: `return a + b`
```cpp
constexpr uint8_t bytecode1[] = {
    ADD(REG(2), REG(0), REG(1)), // r2 = r0 + r1
    RET(REG(2)), // ret r2
};
```
   - Implement a function that multiplies the first two arguments together: `return a * b`
```cpp
constexpr uint8_t bytecode1[] = {
    MUL(REG(2), REG(0), REG(1)), // r2 = r0 * r1
    RET(REG(2)), // ret r2
};
```
   - Implement a function that implements: `return a - b`
```cpp
constexpr uint8_t bytecode1[] = {
    MOVIMM(REG(2), -1), // r2 = -1
    MUL(REG(1), REG(1), REG(2)), // r1 = r1 * r2 (negate r1)
    ADD(REG(2), REG(0), REG(1)), // r2 = r0 + r1 (effectively sub)
    RET(REG(2)), // ret r2
};
```
   - Implement a function that implements: `return a == 42 ? 1337 : 0`
```cpp
constexpr uint8_t bytecode1[] = {
    MOVIMM(REG(2), 42), // r2 = 42
    CMP(REG(3), REG(2), REG(0)), // if(r2 == r0) { r3 = 1 } else { r3 = 0 }
    MOVIMM(REG(2), 1337), // r2 = 1337
    MUL(REG(2), REG(2), REG(3)), // r2 = r2 * r3 (0*1337 = 0 and 1*1337 = 1337)
    RET(REG(2)), // ret r2
};
```

If you finish early, here are a few bonus exercises (not required):

1. Extend `minivm.cpp` so it becomes possible to implement `fib(n)` (iterative).
2. Implement the `fib(n)` function to verify your new opcodes.
3. Analyze the `minivm` binary in you favorite disassembler/decompiler. Would it be difficult to write a disassembler for the VM bytecode if the opcodes were switched around?
4. Which C++ attribute is responsible for the `jmp reg` opcode at the end of the handler?