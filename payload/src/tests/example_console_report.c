#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static int append_line(char* buffer, size_t size, const char* format, ...)
{
    size_t used = strlen(buffer);
    va_list args;
    int ret;

    if (used >= size)
        return -1;

    va_start(args, format);
    ret = vsnprintf(buffer + used, size - used, format, args);
    va_end(args);
    return ret;
}

int main()
{
    char  temp[64]   = {};
    char* report     = (char*)calloc(1, 256);
    char* grown      = NULL;
    int   count      = 0;
    int   threshold  = 0;

    if (report == NULL)
    {
        puts("calloc failed");
        return 1;
    }

    snprintf(temp, sizeof(temp), "items=%d threshold=%d", 7, 42);
    sscanf(temp, "items=%d threshold=%d", &count, &threshold);

    append_line(report, 256, "Report\n");
    append_line(report, 256, "parsed: %s\n", temp);
    append_line(report, 256, "count=%d threshold=%d\n", count, threshold);

    if (strstr(report, "parsed") != NULL)
    {
        memcpy(report + strlen(report), "status=ok\n", strlen("status=ok\n"));
    }

    grown = (char*)realloc(report, 512);
    if (grown == NULL)
    {
        free(report);
        puts("realloc failed");
        return 2;
    }
    report = grown;

    memmove(report + 7, report + 6, strlen(report + 6) + 1);
    report[6] = ' ';

    printf("prefix=%d cmp=%d\n", strncmp(report, "Report", 6), strcmp("ok", "ok"));
    puts(report);
    free(report);
    return 0;
}
