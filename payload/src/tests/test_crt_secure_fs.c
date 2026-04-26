#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <errno.h>
#include <sys/stat.h>

int main()
{
    char template_name[] = "gapXXXXXX.tmp";
    int  fd = -1;
    errno_t err;

    err = _mktemp_s(template_name, sizeof(template_name));
    printf("_mktemp_s=%d name=%s\n", (int)err, template_name);

    err = _sopen_s(&fd, template_name, _O_CREAT | _O_RDWR | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    printf("_sopen_s=%d fd=%d\n", (int)err, fd);

    if (fd >= 0)
    {
        printf("_access_s=%d\n", (int)_access_s(template_name, 0));
        _close(fd);
        remove(template_name);
    }

    puts("test_crt_secure_fs done");
    return 0;
}
