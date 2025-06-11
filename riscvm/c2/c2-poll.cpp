#include "httplib.h"
#include <windows.h>

int main()
{
    httplib::Client cli("http://127.0.0.1");

    while (true)
    {
        puts("Getting payload...");
        auto res = cli.Get("/payload.bin");
        if (!res)
        {
            puts("Failed!");
        }
        else
        {
            printf("status: %d\n", res->status);
            printf("body: %s\n", res->body.c_str());
        }
        Sleep(2000);
    }

    return 0;
}
