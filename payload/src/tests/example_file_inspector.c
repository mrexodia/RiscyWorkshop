#include <windows.h>
#include <stdio.h>
#include <sys/stat.h>
#include <io.h>

int main()
{
    char          module_path[MAX_PATH] = {};
    struct stat   st                    = {};
    struct stat64 st64                  = {};
    struct stat   fdst                  = {};
    unsigned char header[16]            = {};
    FILE*         file                  = NULL;
    long          file_size             = 0;
    int           fd;

    if (GetModuleFileNameA(NULL, module_path, MAX_PATH) == 0)
    {
        puts("GetModuleFileNameA failed");
        return 1;
    }

    printf("stat=%d\n", stat(module_path, &st));
    printf("stat64=%d\n", stat64(module_path, &st64));

    file = fopen(module_path, "rb");
    if (file == NULL)
    {
        puts("fopen failed");
        return 2;
    }

    fread(header, 1, sizeof(header), file);
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fd = _fileno(file);
    printf("fstat=%d\n", fstat(fd, &fdst));
    fclose(file);

    printf("size(stat)=%ld size(stat64)=%lld size(fstat)=%ld\n",
        (long)st.st_size,
        (long long)st64.st_size,
        (long)fdst.st_size);
    printf("mz=%02X%02X file_size=%ld\n", header[0], header[1], file_size);
    return 0;
}
