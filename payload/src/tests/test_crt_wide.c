#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>
#include <windows.h>

static int call_vswprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...)
{
    va_list args;
    int     ret;
    va_start(args, format);
    ret = vswprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}

static void print_wide(const wchar_t* text)
{
    char buffer[128] = {};
    WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer, (int)sizeof(buffer), NULL, NULL);
    printf("%s\n", buffer);
}

int main()
{
    wchar_t swprintf_buffer[64]  = {};
    wchar_t vswprintf_buffer[64] = {};

    int swprintf_ret  = swprintf(swprintf_buffer, 64, L"swprintf:%d:%ls", 21, L"ok");
    int vswprintf_ret = call_vswprintf(vswprintf_buffer, 64, L"vswprintf:%d:%ls", 23, L"ok");

    printf("swprintf_ret=%d vswprintf_ret=%d wcslen=%lu wcscmp=%d\n",
        swprintf_ret,
        vswprintf_ret,
        (unsigned long)wcslen(swprintf_buffer),
        wcscmp(swprintf_buffer, L"swprintf:21:ok"));

    print_wide(swprintf_buffer);
    print_wide(vswprintf_buffer);
    puts("test_crt_wide done");
    return 0;
}
