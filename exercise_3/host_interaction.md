# Host interaction

Rmember: look at `exercise_2/shellcode.md` for compilation instructions.

Exercises:
1. Look in `riscvm.cpp` and make a list of all the available syscalls and their numbers
2. Implement the `print_string` syscall stub in `dbgprint.c`
3. Resolve the `puts` function (`module=0`) and print `Hello from RISC-V!` to the console using `host_call` (you will again need to implement the syscall stubs)
4. Bonus: create a new `secret.c` that opens and reads `secret.txt` using `resolve_import` and `host_call` (hard)
