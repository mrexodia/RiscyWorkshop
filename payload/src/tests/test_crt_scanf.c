#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

static int call_vsscanf(const char* input, const char* format, ...)
{
    va_list args;
    int     ret;
    va_start(args, format);
    ret = vsscanf(input, format, args);
    va_end(args);
    return ret;
}

static int call_vswscanf(const wchar_t* input, const wchar_t* format, ...)
{
    va_list args;
    int     ret;
    va_start(args, format);
    ret = vswscanf(input, format, args);
    va_end(args);
    return ret;
}

int main()
{
    int     a = 0;
    int     b = 0;
    int     c = 0;
    wchar_t word[16] = {};

    int sscanf_ret   = sscanf("12 34", "%d %d", &a, &b);
    int vsscanf_ret  = call_vsscanf("56", "%d", &c);
    int swscanf_ret  = swscanf(L"wide", L"%15ls", word);
    int vswscanf_ret = call_vswscanf(L"78", L"%d", &c);

    printf("sscanf=%d a=%d b=%d\n", sscanf_ret, a, b);
    printf("vsscanf=%d c=%d\n", vsscanf_ret, c);
    printf("swscanf=%d first=%lu\n", swscanf_ret, (unsigned long)word[0]);
    printf("vswscanf=%d c=%d\n", vswscanf_ret, c);
    puts("test_crt_scanf done");
    return 0;
}
