#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

int main()
{
    char* base = (char*)malloc(16);
    char* recalloc_ptr;
    void* aligned_ptr;

    if (base == NULL)
    {
        puts("malloc failed");
        return 1;
    }

    memset(base, 'A', 16);
    recalloc_ptr = (char*)_recalloc(base, 4, 8);
    printf("_recalloc=%p first=%d\n", recalloc_ptr, recalloc_ptr != NULL ? recalloc_ptr[0] : -1);

    aligned_ptr = _aligned_malloc(32, 16);
    if (aligned_ptr == NULL)
    {
        puts("_aligned_malloc failed");
        free(recalloc_ptr);
        return 2;
    }

    memset(aligned_ptr, 'B', 32);
    aligned_ptr = _aligned_recalloc(aligned_ptr, 4, 16, 16);
    printf("_aligned_recalloc=%p\n", aligned_ptr);
    printf("_aligned_msize=%zu\n", _aligned_msize(aligned_ptr, 16, 0));

    _aligned_free(aligned_ptr);
    free(recalloc_ptr);
    puts("test_crt_alloc done");
    return 0;
}
