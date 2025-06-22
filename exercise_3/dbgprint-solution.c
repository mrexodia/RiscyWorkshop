#include <stdint.h>

static void print_string(const char* str);
static void exit(int exit_code);

// NOTE: This has to be first definition in the file
void _start()
{
    print_string("Hello from RISC-V!");
    exit(0);
    asm volatile("ebreak");
}

static __attribute((noinline)) void print_string(const char* str)
{
    register uint64_t a0 asm("a0") = (uint64_t)str;
    register uint64_t a7 asm("a7") = 10101;
    asm volatile("scall" : "+r"(a0) : "r"(a7) : "memory");
}

static __attribute((noinline)) void exit(int exit_code)
{
    register uint64_t a0 asm("a0") = exit_code;
    register uint64_t a7 asm("a7") = 10000;
    asm volatile("scall" : "+r"(a0) : "r"(a7) : "memory");
}
