# RISC-V Shellcode

Change the working directory:

```sh
cd exercise_2
```

Build the `riscvm` interpreter for Linux:

```sh
cmake -S ../riscvm --preset linux
cmake --build ../riscvm/build-linux
```

Make sure the interpreter compiled correctly:

```sh
../riscvm/build-linux/riscvm
```

This should print:

```lua
please supply a RV64I program to run!
```

Compile `hello.c` for `rv64im`:

```sh
clang-20 -target riscv64 -march=rv64im -mcmodel=medany -Os -c hello.c -o hello.o
```

Godbolt link to quickly play around: https://godbolt.org/z/14aWYrx4M

Disassemble the object:

```sh
llvm-objdump-20 --disassemble hello.o
```

Link the shellcode with the linker script into an ELF container:

```sh
ld.lld-20 -o hello.elf --oformat=elf -emit-relocs -T ../riscvm/lib/linker.ld --Map=hello.map hello.o
```

Extract the shellcode from the ELF container:

```sh
llvm-objcopy-20 -O binary hello.elf hello.pre.bin
```

Run the shellcode in the `riscvm` interpreter:

```sh
../riscvm/build-linux/riscvm hello.pre.bin
```

**NOTE**: It is normal that you will see `no features in the file (unencrypted payload?)` at this stage. This message will disappear once we enable obfuscation features in a later exercise.

Exercises:
1. Run the interpreter with the `--trace` argument and inspect the trace with `code hello.pre.bin.trace`
2. Load `hello.elf` in Ghidra (make sure you match the base address from the trace)
3. Go over every instruction in your disassembler and write down a comment in pseudocode of what it does.
   - RISC-V Instruction Set Specifications: https://msyksphinz-self.github.io/riscv-isadoc/html/index.html
   - Official reference: https://five-embeddev.com/riscv-user-isa-manual/Priv-v1.12/rv64.html
   - Pseudo Instructions: https://stackoverflow.com/a/65008687/1806760
4. Why does the `_start` function have to be first in the file?
5. What happens if you comment out the call to `exit` and let `_start` return? Why does this happen?
6. Bonus: Complete the `build.py` script that automates the compilation steps