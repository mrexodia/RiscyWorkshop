#include <stdint.h>

static uint64_t resolve_import(const char* module, const char* function);
static uint64_t host_call(uint64_t fn, uint64_t args[13]);
static uint64_t exit(int exit_code);

// NOTE: This has to be first definition in the file
void _start()
{
    // TODO: call resolve_import and host_call here
    exit(0);
    asm volatile("ebreak");
}

static __attribute((noinline)) uint64_t resolve_import(const char* module, const char* function)
{
    asm volatile("ebreak"); // TODO: replace with scall
}

static __attribute((noinline)) uint64_t host_call(uint64_t fn, uint64_t args[13])
{
    asm volatile("ebreak"); // TODO: replace with scall
}

static __attribute((noinline)) uint64_t exit(int exit_code)
{
    register uint64_t a0 asm("a0") = exit_code;
    register uint64_t a1 asm("a1") = 0; // unused
    register uint64_t a7 asm("a7") = 10000;
    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}
