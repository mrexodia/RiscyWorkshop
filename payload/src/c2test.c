#include <windows.h>
#include <stdio.h>

typedef void (*append_response_t)(const char*);

void* riscvm_host_call(void* address, uintptr_t args[13]);

int main()
{
    puts("Hello from RISC-V!");
    MessageBoxA(0, "Hello from NoVNC!", "Wine", MB_SYSTEMMODAL);
    append_response_t append_response = (append_response_t)GetProcAddress(0, "append_response");
    printf("append_response: %p\n", append_response);
    if (append_response != NULL)
    {
        uintptr_t args[13] = {};
        args[0]            = (uintptr_t)"Hello from riscvm_host_call!";
        riscvm_host_call(append_response, args);
    }
}
