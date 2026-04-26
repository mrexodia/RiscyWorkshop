# Targeted runtime example programs

These payloads are meant to look like small real programs instead of one-symbol micro-tests.

- `example_console_report.c`
  - heap allocation, string handling, `snprintf`/`vsnprintf`, `sscanf`, `memcpy`/`memmove`
- `example_file_inspector.c`
  - `fopen`/`fread`/`fseek`/`ftell`, `stat`/`stat64`/`fstat`, `GetModuleFileNameA`
- `example_unicode_inspector.c`
  - wide-char formatting, UTF-8/UTF-16 conversion, `wcslen`/`wcscmp`
- `example_mbwc_roundtrip.c`
  - multibyte/wide conversion helpers (`mbrtowc`, `mbsrtowcs`, `wcrtomb`, `wcsrtombs`)
- `example_cpp_lifecycle.cpp`
  - global constructors/destructors, `atexit`, function-local statics, `new[]`/`delete[]`

They complement the lower-level `test_crt_*` repro targets.
