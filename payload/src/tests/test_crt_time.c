#include <stdio.h>
#include <time.h>
#include <string.h>

int main()
{
    __time64_t now = 1710000000;
    struct tm  gm  = {};
    struct tm  loc = {};
    char       ctime_buffer[64] = {};
    char       date_buffer[16]  = {};
    char       time_buffer[16]  = {};

    printf("_gmtime64_s=%d\n", _gmtime64_s(&gm, &now));
    printf("_localtime64_s=%d\n", _localtime64_s(&loc, &now));
    printf("_ctime64_s=%d\n", _ctime64_s(ctime_buffer, sizeof(ctime_buffer), &now));
    printf("_strdate_s=%d\n", _strdate_s(date_buffer, sizeof(date_buffer)));
    printf("_strtime_s=%d\n", _strtime_s(time_buffer, sizeof(time_buffer)));

    printf("gm=%04d-%02d-%02d %02d:%02d:%02d\n",
        gm.tm_year + 1900,
        gm.tm_mon + 1,
        gm.tm_mday,
        gm.tm_hour,
        gm.tm_min,
        gm.tm_sec);
    printf("local=%04d-%02d-%02d %02d:%02d:%02d\n",
        loc.tm_year + 1900,
        loc.tm_mon + 1,
        loc.tm_mday,
        loc.tm_hour,
        loc.tm_min,
        loc.tm_sec);
    printf("ctime=%s", ctime_buffer);
    printf("date=%s time=%s\n", date_buffer, time_buffer);

    puts("test_crt_time done");
    return 0;
}
