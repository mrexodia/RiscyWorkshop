#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

struct Foo
{
    Foo()
    {
        puts("Foo initialized");
    }
};

Foo f;

int main()
{
    int  fd = open("secret.txt", O_RDONLY);
    char buffer[256];
    read(fd, buffer, sizeof(buffer));
    puts(buffer);
    return 0;
}
