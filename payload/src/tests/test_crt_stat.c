#include <windows.h>
#include <stdio.h>
#include <sys/stat.h>
#include <io.h>
#include <wchar.h>

int main()
{
    char           module_path[MAX_PATH]   = {};
    wchar_t        module_path_w[MAX_PATH] = {};
    struct _stat64 direct_a                = {};
    struct _stat64 direct_w                = {};
    struct _stat64 direct_fd               = {};
    struct stat64  wrap_a                  = {};
    struct stat64  wrap_w                  = {};
    struct stat64  wrap_fd                 = {};
    struct stat    compat_a                = {};
    struct stat    compat_fd               = {};
    FILE*          file                    = NULL;
    int            fd;

    if (GetModuleFileNameA(NULL, module_path, MAX_PATH) == 0)
    {
        puts("GetModuleFileNameA failed");
        return 1;
    }

    MultiByteToWideChar(CP_UTF8, 0, module_path, -1, module_path_w, MAX_PATH);

    printf("direct _stat64=%d\n", _stat64(module_path, &direct_a));
    printf("direct _wstat64=%d\n", _wstat64(module_path_w, &direct_w));

    file = fopen(module_path, "rb");
    if (file == NULL)
    {
        puts("fopen failed");
        return 2;
    }

    fd = _fileno(file);
    printf("direct _fstat64=%d\n", _fstat64(fd, &direct_fd));

    printf("wrapper stat64=%d\n", stat64(module_path, &wrap_a));
    printf("wrapper wstat64=%d\n", wstat64(module_path_w, &wrap_w));
    printf("wrapper fstat64=%d\n", fstat64(fd, &wrap_fd));

    printf("compat stat=%d\n", stat(module_path, &compat_a));
    printf("compat fstat=%d\n", fstat(fd, &compat_fd));
    fclose(file);

    printf("direct size=%I64d wrapper size=%I64d compat size=%ld mode=%u\n",
        (long long)direct_a.st_size,
        (long long)wrap_a.st_size,
        (long)compat_a.st_size,
        (unsigned)direct_a.st_mode);

    puts("test_crt_stat done");
    return 0;
}
