#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

static int append_wide_line(wchar_t* buffer, size_t size, const wchar_t* format, ...)
{
    size_t used = wcslen(buffer);
    va_list args;
    int ret;

    if (used >= size)
        return -1;

    va_start(args, format);
    ret = vswprintf(buffer + used, size - used, format, args);
    va_end(args);
    return ret;
}

int main()
{
    char    path_a[MAX_PATH]    = {};
    wchar_t path_w[MAX_PATH]    = {};
    wchar_t message[512]        = {};
    char    roundtrip[512]      = {};

    if (GetModuleFileNameA(NULL, path_a, MAX_PATH) == 0)
    {
        puts("GetModuleFileNameA failed");
        return 1;
    }

    MultiByteToWideChar(CP_UTF8, 0, path_a, -1, path_w, MAX_PATH);
    swprintf(message, 512, L"module=%ls\n", path_w);
    append_wide_line(message, 512, L"length=%lu equal=%d\n", (unsigned long)wcslen(path_w), wcscmp(path_w, path_w));

    WideCharToMultiByte(CP_UTF8, 0, message, -1, roundtrip, (int)sizeof(roundtrip), NULL, NULL);
    printf("%s", roundtrip);
    return 0;
}
