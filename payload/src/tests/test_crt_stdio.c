#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int call_vsnprintf(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    int     ret;
    va_start(args, format);
    ret = vsnprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}

static int call_vsprintf(char* buffer, const char* format, ...)
{
    va_list args;
    int     ret;
    va_start(args, format);
    ret = vsprintf(buffer, format, args);
    va_end(args);
    return ret;
}

int main()
{
    char snprintf_buffer[64]  = {};
    char vsnprintf_buffer[64] = {};
    char sprintf_buffer[64]   = {};
    char vsprintf_buffer[64]  = {};

    int snprintf_ret  = snprintf(snprintf_buffer, sizeof(snprintf_buffer), "snprintf:%d:%s", 7, "ok");
    int vsnprintf_ret = call_vsnprintf(vsnprintf_buffer, sizeof(vsnprintf_buffer), "vsnprintf:%d:%s", 11, "ok");
    int sprintf_ret   = sprintf(sprintf_buffer, "sprintf:%d:%s", 13, "ok");
    int vsprintf_ret  = call_vsprintf(vsprintf_buffer, "vsprintf:%d:%s", 17, "ok");

    printf("snprintf  -> '%s' (%d)\n", snprintf_buffer, snprintf_ret);
    printf("vsnprintf -> '%s' (%d)\n", vsnprintf_buffer, vsnprintf_ret);
    printf("sprintf   -> '%s' (%d)\n", sprintf_buffer, sprintf_ret);
    printf("vsprintf  -> '%s' (%d)\n", vsprintf_buffer, vsprintf_ret);

    printf("strlen=%lu strcmp=%d strncmp=%d\n",
        (unsigned long)strlen(sprintf_buffer),
        strcmp(sprintf_buffer, "sprintf:13:ok"),
        strncmp(vsprintf_buffer, "vsprintf", 8));

    puts("test_crt_stdio done");
    return 0;
}
