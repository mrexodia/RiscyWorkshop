#include <stdint.h>

static __attribute((noinline)) uint64_t exit(int exit_code)
{
    register uint64_t a0 asm("a0") = exit_code;
    register uint64_t a1 asm("a1") = 0; // unused
    register uint64_t a7 asm("a7") = 10000;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}

int _start() __attribute__((section(".text.start")));

int _start()
{
    int result = 0;
    for (int i = 0; i < 52; i++)
    {
        result += *(volatile int*)&i;
    }
    result += 11;
    exit(result);
    return result;
}
