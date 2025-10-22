#include <stdint.h>

static __attribute((noinline)) void print_string(const char* str)
{
    asm volatile("ebreak"); // TODO: replace with proper implementation
}

static __attribute((noinline)) uint64_t exit(int exit_code)
{
    register uint64_t a0 asm("a0") = exit_code;
    register uint64_t a1 asm("a1") = 0; // unused
    register uint64_t a7 asm("a7") = 10000;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}

void _start() __attribute__((section(".text.start")));

void _start()
{
    print_string("Hello from RISC-V!");
    exit(0);
    asm volatile("ebreak");
}
