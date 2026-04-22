#include <stdint.h>

static __attribute((noinline)) uint64_t resolve_import(const char* module, const char* function)
{
    register uint64_t a0 asm("a0") = (uint64_t)module;
    register uint64_t a1 asm("a1") = (uint64_t)function;
    register uint64_t a7 asm("a7") = 10105;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
}

static __attribute((noinline)) uint64_t host_call(uint64_t fn, uint64_t args[13])
{
    register uint64_t a0 asm("a0") = fn;
    register uint64_t a1 asm("a1") = (uint64_t)args;
    register uint64_t a7 asm("a7") = 20000;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");
    return a0;
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
    uint64_t args[13];
    char     buffer[256];

    uint64_t open_addr = resolve_import("libc.so.6", "open");
    args[0]            = (uint64_t)"secret.txt";
    args[1]            = 0; // flags (O_RDONLY)
    args[2]            = 0; // mode (unused)
    uint64_t fd        = host_call(open_addr, args);

    uint64_t read_addr = resolve_import("libc.so.6", "read");
    args[0]            = fd;
    args[1]            = (uint64_t)buffer; // buffer
    args[2]            = sizeof(buffer);   // count
    host_call(read_addr, args);

    uint64_t puts_addr = resolve_import("libc.so.6", "puts");
    args[0]            = (uint64_t)buffer;
    host_call(puts_addr, args);

    exit(0);
    asm volatile("ebreak");
}
