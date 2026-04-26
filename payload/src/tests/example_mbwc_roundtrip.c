#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>

int main()
{
    const char*   input = "Alpha Beta";
    const char*   input_ptr;
    const wchar_t* wide_ptr;
    mbstate_t     state;
    wchar_t       wide[32] = {};
    wchar_t       first    = 0;
    char          single[8] = {};
    char          roundtrip[32] = {};

    memset(&state, 0, sizeof(state));
    mbrtowc(&first, input, 1, &state);

    input_ptr = input;
    memset(&state, 0, sizeof(state));
    mbsrtowcs(wide, &input_ptr, 32, &state);

    memset(&state, 0, sizeof(state));
    wcrtomb(single, L'Z', &state);

    wide_ptr = wide;
    memset(&state, 0, sizeof(state));
    wcsrtombs(roundtrip, &wide_ptr, sizeof(roundtrip), &state);

    printf("first=%lu single=%s roundtrip=%s\n", (unsigned long)first, single, roundtrip);
    return 0;
}
