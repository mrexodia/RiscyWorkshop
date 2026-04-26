#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <time.h>

int main()
{
    char*    pgmptr   = NULL;
    wchar_t* wpgmptr  = NULL;
    int      daylight = 0;
    long     dstbias  = 0;
    long     timezone = 0;
    size_t   tzname0  = 0;
    char     tzbuf[64] = {};

    printf("_get_pgmptr=%d\n", _get_pgmptr(&pgmptr));
    printf("_get_wpgmptr=%d\n", _get_wpgmptr(&wpgmptr));
    printf("_get_daylight=%d value=%d\n", _get_daylight(&daylight), daylight);
    printf("_get_dstbias=%d value=%ld\n", _get_dstbias(&dstbias), dstbias);
    printf("_get_timezone=%d value=%ld\n", _get_timezone(&timezone), timezone);
    printf("_get_tzname=%d len=%lu\n", _get_tzname(&tzname0, tzbuf, sizeof(tzbuf), 0), (unsigned long)tzname0);
    printf("_configthreadlocale=%d\n", _configthreadlocale(_ENABLE_PER_THREAD_LOCALE));

    if (pgmptr != NULL)
        printf("pgmptr=%s\n", pgmptr);
    if (wpgmptr != NULL)
        printf("wpgmptr[0]=%lu\n", (unsigned long)wpgmptr[0]);
    printf("tzname=%s\n", tzbuf);
    puts("test_crt_env done");
    return 0;
}
