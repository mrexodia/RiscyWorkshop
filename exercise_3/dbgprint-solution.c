#include <stdint.h>

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

void _start() __attribute__((section(".text.start")));

void _start()
{
    print_string("Hello from RISC-V!");
    exit(0);
    asm volatile("ebreak");
}
