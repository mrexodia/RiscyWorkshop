# Host interaction

**Change the working directory:**

```sh
cd exercise_3
```

Remember: look at `exercise_2/shellcode.md` for compilation instructions. You can use `./build.py --run dbgprint.c` to quickly compile and run your payloads.

Exercises:
1. Look in `riscvm.cpp` and make a list of all the available syscalls and their numbers
1b. What is the calling convention for the `ecall` instruction? Where does the syscall number come from, where is the result stored?
2. Implement the `print_string` syscall stub in `dbgprint.c`
3. In `import.c`, resolve the `puts` function (`module=0`) and print `Hello from RISC-V!` to the console using `host_call` (you will again need to implement the syscall stubs)
4. Bonus: create a new `secret.c` payload that opens and reads `secret.txt` using `resolve_import` and `host_call` (hard)
