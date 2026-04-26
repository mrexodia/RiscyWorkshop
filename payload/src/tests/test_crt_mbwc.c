#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

int main()
{
    const char*  src = "Hello";
    const char*  src_ptr;
    const wchar_t* wide_ptr;
    mbstate_t    state;
    wchar_t      wide_buffer[16] = {};
    char         roundtrip[16]   = {};
    wchar_t      wc              = 0;
    char         single[8]       = {};

    memset(&state, 0, sizeof(state));
    printf("mbrlen=%lu\n", (unsigned long)mbrlen(src, 1, &state));

    memset(&state, 0, sizeof(state));
    printf("mbrtowc=%lu\n", (unsigned long)mbrtowc(&wc, src, 1, &state));

    src_ptr = src;
    memset(&state, 0, sizeof(state));
    printf("mbsrtowcs=%lu\n", (unsigned long)mbsrtowcs(wide_buffer, &src_ptr, 16, &state));

    memset(&state, 0, sizeof(state));
    printf("wcrtomb=%lu\n", (unsigned long)wcrtomb(single, L'Z', &state));

    wide_ptr = wide_buffer;
    memset(&state, 0, sizeof(state));
    printf("wcsrtombs=%lu\n", (unsigned long)wcsrtombs(roundtrip, &wide_ptr, sizeof(roundtrip), &state));

    printf("wide[0]=%lu roundtrip='%s' single='%s'\n",
        (unsigned long)wide_buffer[0],
        roundtrip,
        single);

    puts("test_crt_mbwc done");
    return 0;
}
