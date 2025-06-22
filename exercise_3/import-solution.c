#include <stdint.h>

static void     print_string(const char* str);
static uint64_t resolve_import(const char* module, const char* function);
static uint64_t host_call(uint64_t fn, uint64_t args[13]);
static uint64_t exit(int exit_code);

// NOTE: This has to be first definition in the file
void _start()
{
    uint64_t host_puts = resolve_import(0, "puts");
    uint64_t args[13];
    args[0] = (uint64_t)"Hello from RISC-V!";
    host_call(host_puts, args);
    exit(0);
    asm volatile("ebreak");
}

static __attribute((noinline)) void print_string(const char* str)
{
    register uint64_t a0 asm("a0") = (uint64_t)str;
    register uint64_t a7 asm("a7") = 10101;
    asm volatile("scall" : "+r"(a0) : "r"(a7) : "memory");
}

static __attribute((noinline)) uint64_t resolve_import(const char* module, const char* function)
{
    register uint64_t a0 asm("a0") = (uint64_t)module;
    register uint64_t a1 asm("a1") = (uint64_t)function;
    register uint64_t a7 asm("a7") = 10105;
    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}

static __attribute((noinline)) uint64_t host_call(uint64_t fn, uint64_t args[13])
{
    register uint64_t a0 asm("a0") = fn;
    register uint64_t a1 asm("a1") = (uint64_t)args;
    register uint64_t a7 asm("a7") = 20000;
    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}

static __attribute((noinline)) uint64_t exit(int exit_code)
{
    register uint64_t a0 asm("a0") = exit_code;
    register uint64_t a1 asm("a1") = 0; // unused
    register uint64_t a7 asm("a7") = 10000;
    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}
