#include <stdio.h>
#include <stdlib.h>

static int g_state = 0;

static void on_exit_callback()
{
    puts("atexit callback");
    g_state |= 4;
}

struct GlobalObject
{
    GlobalObject()
    {
        puts("global ctor");
        g_state |= 1;
    }

    ~GlobalObject()
    {
        puts("global dtor");
        g_state |= 2;
    }
};

static GlobalObject g_object;

int main()
{
    printf("main state=%d before atexit\n", g_state);
    if (atexit(on_exit_callback) != 0)
    {
        puts("atexit registration failed");
        return 1;
    }

    puts("returning from main");
    return 0;
}
