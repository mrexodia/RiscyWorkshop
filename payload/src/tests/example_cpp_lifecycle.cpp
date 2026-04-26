#include <stdio.h>
#include <stdlib.h>

struct Logger
{
    Logger()
    {
        puts("logger ctor");
    }

    ~Logger()
    {
        puts("logger dtor");
    }
};

static Logger g_logger;

static void shutdown_notice()
{
    puts("atexit callback");
}

struct Record
{
    int  id;
    char tag[16];
};

static Record* get_records()
{
    static Record records[2] = {{1, "alpha"}, {2, "beta"}};
    return records;
}

int main()
{
    Record* heap_records = new Record[2];
    Record* static_records = get_records();

    heap_records[0] = static_records[0];
    heap_records[1] = static_records[1];

    printf("records: %d/%s %d/%s\n",
        heap_records[0].id,
        heap_records[0].tag,
        heap_records[1].id,
        heap_records[1].tag);

    if (atexit(shutdown_notice) != 0)
    {
        puts("atexit failed");
    }

    delete[] heap_records;
    puts("example_cpp_lifecycle done");
    return 0;
}
