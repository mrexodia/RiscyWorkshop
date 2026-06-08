#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

extern "C" uintptr_t riscvm_resolve_dll(uint32_t module_hash);
extern "C" uintptr_t riscvm_resolve_import(uintptr_t image, uint32_t export_hash);
extern "C" uintptr_t riscvm_host_call(uintptr_t address, uintptr_t args[13]);

static uint32_t hash_x65599(const char* s, bool case_sensitive)
{
    uint32_t hash = 0;
    for (; *s; ++s)
    {
        char ch = *s;
        if (!case_sensitive && ch >= 'a' && ch <= 'z')
            ch -= 'a' - 'A';
        hash = (uint8_t)ch + 65599u * hash;
    }
    return hash;
}

static uintptr_t resolve(const char* dll, const char* name)
{
    uintptr_t base = riscvm_resolve_dll(hash_x65599(dll, false));
    if (!base)
        return 0;
    return riscvm_resolve_import(base, hash_x65599(name, true));
}

static uintptr_t host_call(uintptr_t fn,
                           uintptr_t a0 = 0,
                           uintptr_t a1 = 0,
                           uintptr_t a2 = 0,
                           uintptr_t a3 = 0,
                           uintptr_t a4 = 0,
                           uintptr_t a5 = 0)
{
    uintptr_t args[13] = {a0, a1, a2, a3, a4, a5};
    return fn ? riscvm_host_call(fn, args) : 0;
}

static uintptr_t msvcrt(const char* name)
{
    return resolve("msvcrt.dll", name);
}

static size_t cstr_len(const char* s)
{
    size_t n = 0;
    if (s)
        while (s[n])
            ++n;
    return n;
}

static size_t wstr_len(const wchar_t* s)
{
    size_t n = 0;
    if (s)
        while (s[n])
            ++n;
    return n;
}

static void* mem_copy(void* dst, const void* src, size_t n)
{
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];
    return dst;
}

static void* mem_set(void* dst, int ch, size_t n)
{
    uint8_t* d = (uint8_t*)dst;
    for (size_t i = 0; i < n; ++i)
        d[i] = (uint8_t)ch;
    return dst;
}

static void append_char(char*& out, size_t& left, int& total, char ch)
{
    if (left > 1)
    {
        *out++ = ch;
        --left;
    }
    ++total;
}

static void append_str(char*& out, size_t& left, int& total, const char* s)
{
    if (!s)
        s = "(null)";
    while (*s)
        append_char(out, left, total, *s++);
}

static void append_wide_as_ascii(char*& out, size_t& left, int& total, const wchar_t* s)
{
    if (!s)
    {
        append_str(out, left, total, "(null)");
        return;
    }
    while (*s)
    {
        wchar_t wc = *s++;
        append_char(out, left, total, wc < 0x80 ? (char)wc : '?');
    }
}

static void append_uint(char*& out, size_t& left, int& total, unsigned long long value, unsigned base, bool upper)
{
    char tmp[32];
    size_t pos = 0;
    do
    {
        unsigned digit = (unsigned)(value % base);
        tmp[pos++] = (char)(digit < 10 ? '0' + digit : (upper ? 'A' : 'a') + digit - 10);
        value /= base;
    } while (value && pos < sizeof(tmp));
    while (pos)
        append_char(out, left, total, tmp[--pos]);
}

static void append_int(char*& out, size_t& left, int& total, long long value)
{
    if (value < 0)
    {
        append_char(out, left, total, '-');
        append_uint(out, left, total, (unsigned long long)-value, 10, false);
    }
    else
    {
        append_uint(out, left, total, (unsigned long long)value, 10, false);
    }
}

extern "C" int vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
    char* out = buffer;
    size_t left = size;
    int total = 0;

    for (const char* p = format; p && *p; ++p)
    {
        if (*p != '%')
        {
            append_char(out, left, total, *p);
            continue;
        }

        ++p;
        if (*p == '%')
        {
            append_char(out, left, total, '%');
            continue;
        }

        while (*p >= '0' && *p <= '9')
            ++p;

        bool long_mod = false;
        bool longlong_mod = false;
        bool size_mod = false;
        if (*p == 'I' && p[1] == '6' && p[2] == '4')
        {
            longlong_mod = true;
            p += 3;
        }
        else if (*p == 'l')
        {
            long_mod = true;
            ++p;
            if (*p == 'l')
            {
                longlong_mod = true;
                ++p;
            }
        }
        else if (*p == 'z')
        {
            size_mod = true;
            ++p;
        }

        switch (*p)
        {
        case 'd':
        case 'i':
            if (longlong_mod)
                append_int(out, left, total, va_arg(args, long long));
            else if (long_mod)
                append_int(out, left, total, va_arg(args, long));
            else
                append_int(out, left, total, va_arg(args, int));
            break;
        case 'u':
            if (size_mod)
                append_uint(out, left, total, va_arg(args, size_t), 10, false);
            else if (longlong_mod)
                append_uint(out, left, total, va_arg(args, unsigned long long), 10, false);
            else if (long_mod)
                append_uint(out, left, total, va_arg(args, unsigned long), 10, false);
            else
                append_uint(out, left, total, va_arg(args, unsigned), 10, false);
            break;
        case 'x':
        case 'X':
            append_uint(out, left, total, va_arg(args, unsigned), 16, *p == 'X');
            break;
        case 'p':
            append_str(out, left, total, "0x");
            append_uint(out, left, total, (uintptr_t)va_arg(args, void*), 16, false);
            break;
        case 's':
            if (long_mod)
                append_wide_as_ascii(out, left, total, va_arg(args, const wchar_t*));
            else
                append_str(out, left, total, va_arg(args, const char*));
            break;
        case 'c':
            append_char(out, left, total, (char)va_arg(args, int));
            break;
        default:
            append_char(out, left, total, '%');
            append_char(out, left, total, *p);
            break;
        }
    }

    if (size)
        *out = '\0';
    return total;
}

extern "C" int vsprintf(char* buffer, const char* format, va_list args)
{
    return vsnprintf(buffer, (size_t)-1, format, args);
}

extern "C" int __ms_vsnprintf(char* buffer, size_t size, const char* format, va_list args)
{
    return vsnprintf(buffer, size, format, args);
}

extern "C" int snprintf(char* buffer, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}

extern "C" int sprintf(char* buffer, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, (size_t)-1, format, args);
    va_end(args);
    return ret;
}

static void append_wchar(wchar_t*& out, size_t& left, int& total, wchar_t ch)
{
    if (left > 1)
    {
        *out++ = ch;
        --left;
    }
    ++total;
}

static void append_wstr(wchar_t*& out, size_t& left, int& total, const wchar_t* s)
{
    if (!s)
        s = L"(null)";
    while (*s)
        append_wchar(out, left, total, *s++);
}

static void append_ascii_as_wide(wchar_t*& out, size_t& left, int& total, const char* s)
{
    if (!s)
        s = "(null)";
    while (*s)
        append_wchar(out, left, total, (unsigned char)*s++);
}

static void append_uint_w(wchar_t*& out, size_t& left, int& total, unsigned long long value)
{
    char tmp[32];
    char* p = tmp;
    size_t l = sizeof(tmp);
    int ignored = 0;
    append_uint(p, l, ignored, value, 10, false);
    *p = 0;
    append_ascii_as_wide(out, left, total, tmp);
}

static void append_int_w(wchar_t*& out, size_t& left, int& total, long long value)
{
    if (value < 0)
    {
        append_wchar(out, left, total, L'-');
        append_uint_w(out, left, total, (unsigned long long)-value);
    }
    else
        append_uint_w(out, left, total, (unsigned long long)value);
}

extern "C" int vswprintf(wchar_t* buffer, size_t size, const wchar_t* format, va_list args)
{
    wchar_t* out = buffer;
    size_t left = size;
    int total = 0;
    for (const wchar_t* p = format; p && *p; ++p)
    {
        if (*p != L'%')
        {
            append_wchar(out, left, total, *p);
            continue;
        }
        ++p;
        if (*p == L'%')
        {
            append_wchar(out, left, total, L'%');
            continue;
        }
        while (*p >= L'0' && *p <= L'9')
            ++p;

        bool long_mod = false;
        if (*p == L'l')
        {
            long_mod = true;
            ++p;
        }
        switch (*p)
        {
        case L'd':
        case L'i':
            append_int_w(out, left, total, va_arg(args, int));
            break;
        case L'u':
            append_uint_w(out, left, total, va_arg(args, unsigned));
            break;
        case L's':
            if (long_mod)
                append_wstr(out, left, total, va_arg(args, const wchar_t*));
            else
                append_ascii_as_wide(out, left, total, va_arg(args, const char*));
            break;
        default:
            append_wchar(out, left, total, L'%');
            append_wchar(out, left, total, *p);
            break;
        }
    }
    if (size)
        *out = 0;
    return total;
}

extern "C" int swprintf(wchar_t* buffer, size_t size, const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vswprintf(buffer, size, format, args);
    va_end(args);
    return ret;
}

extern "C" int __ms_vswprintf(wchar_t* buffer, size_t size, const wchar_t* format, va_list args)
{
    return vswprintf(buffer, size, format, args);
}

static const char* skip_spaces(const char* s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
        ++s;
    return s;
}

extern "C" int vsscanf(const char* input, const char* format, va_list args)
{
    int assigned = 0;
    const char* in = input;
    for (const char* f = format; *f; ++f)
    {
        if (*f != '%')
        {
            if (*f == ' ')
                in = skip_spaces(in);
            else if (*in == *f)
                ++in;
            continue;
        }
        ++f;
        if (*f == 'd')
        {
            in = skip_spaces(in);
            int sign = 1;
            if (*in == '-')
            {
                sign = -1;
                ++in;
            }
            int val = 0;
            while (*in >= '0' && *in <= '9')
                val = val * 10 + (*in++ - '0');
            *va_arg(args, int*) = val * sign;
            ++assigned;
        }
    }
    return assigned;
}

extern "C" int vswscanf(const wchar_t* input, const wchar_t* format, va_list args)
{
    char in[128];
    char fmt[64];
    size_t i = 0;
    for (; input[i] && i + 1 < sizeof(in); ++i)
        in[i] = input[i] < 0x80 ? (char)input[i] : '?';
    in[i] = 0;
    i = 0;
    for (; format[i] && i + 1 < sizeof(fmt); ++i)
        fmt[i] = format[i] < 0x80 ? (char)format[i] : '?';
    fmt[i] = 0;
    return vsscanf(in, fmt, args);
}

extern "C" size_t mbrlen(const char* s, size_t n, void*)
{
    if (!s)
        return 0;
    if (n == 0)
        return (size_t)-2;
    return *s ? 1 : 0;
}

extern "C" size_t mbrtowc(wchar_t* pwc, const char* s, size_t n, void*)
{
    if (!s)
        return 0;
    if (n == 0)
        return (size_t)-2;
    if (pwc)
        *pwc = (unsigned char)*s;
    return *s ? 1 : 0;
}

extern "C" size_t mbsrtowcs(wchar_t* dst, const char** src, size_t len, void*)
{
    if (!src || !*src)
        return 0;
    const char* s = *src;
    size_t n = 0;
    while (s[n] && (!dst || n < len))
    {
        if (dst)
            dst[n] = (unsigned char)s[n];
        ++n;
    }
    if (dst && n < len)
        dst[n] = 0;
    if (dst)
        *src = s[n] ? s + n : nullptr;
    return n;
}

extern "C" size_t wcrtomb(char* s, wchar_t wc, void*)
{
    if (!s)
        return 1;
    s[0] = wc < 0x80 ? (char)wc : '?';
    s[1] = 0;
    return 1;
}

extern "C" size_t wcsrtombs(char* dst, const wchar_t** src, size_t len, void*)
{
    if (!src || !*src)
        return 0;
    const wchar_t* s = *src;
    size_t n = 0;
    while (s[n] && (!dst || n < len))
    {
        if (dst)
            dst[n] = s[n] < 0x80 ? (char)s[n] : '?';
        ++n;
    }
    if (dst && n < len)
        dst[n] = 0;
    if (dst)
        *src = s[n] ? s + n : nullptr;
    return n;
}

extern "C" void* _recalloc(void* ptr, size_t count, size_t size)
{
    size_t bytes = count * size;
    void* out = (void*)host_call(msvcrt("realloc"), (uintptr_t)ptr, bytes);
    return out;
}

extern "C" void* _aligned_recalloc(void* ptr, size_t count, size_t size, size_t alignment)
{
    return (void*)host_call(msvcrt("_aligned_realloc"), (uintptr_t)ptr, count * size, alignment);
}

extern "C" size_t _aligned_msize(void*, size_t, size_t)
{
    return 0;
}

extern "C" int _mktemp_s(char* tmpl, size_t size)
{
    if (!tmpl || !size)
        return 22;
    for (size_t i = 0; i + 5 < size && tmpl[i]; ++i)
    {
        if (tmpl[i] == 'X' && tmpl[i + 1] == 'X' && tmpl[i + 2] == 'X' &&
            tmpl[i + 3] == 'X' && tmpl[i + 4] == 'X' && tmpl[i + 5] == 'X')
        {
            tmpl[i + 0] = '0';
            tmpl[i + 1] = '0';
            tmpl[i + 2] = '0';
            tmpl[i + 3] = '0';
            tmpl[i + 4] = '0';
            tmpl[i + 5] = '1';
            return 0;
        }
    }
    return 22;
}

extern "C" int _sopen_s(int* fd, const char* name, int oflag, int shflag, int pmode)
{
    int result = (int)host_call(msvcrt("_sopen"), (uintptr_t)name, (uintptr_t)oflag, (uintptr_t)shflag, (uintptr_t)pmode);
    if (fd)
        *fd = result;
    return result < 0 ? 13 : 0;
}

extern "C" int _access_s(const char* name, int mode)
{
    return (int)host_call(msvcrt("_access"), (uintptr_t)name, (uintptr_t)mode) == 0 ? 0 : 13;
}

static bool is_leap(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static void seconds_to_tm(int64_t t, int* tm)
{
    static const int mdays_common[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int64_t days = t / 86400;
    int64_t rem = t % 86400;
    if (rem < 0)
    {
        rem += 86400;
        --days;
    }
    tm[0] = (int)(rem % 60);
    tm[1] = (int)((rem / 60) % 60);
    tm[2] = (int)(rem / 3600);
    tm[6] = (int)((days + 4) % 7);
    if (tm[6] < 0) tm[6] += 7;

    int year = 1970;
    while (true)
    {
        int yd = is_leap(year) ? 366 : 365;
        if (days < yd)
            break;
        days -= yd;
        ++year;
    }
    tm[5] = year - 1900;
    tm[7] = (int)days;
    int mon = 0;
    for (; mon < 12; ++mon)
    {
        int md = mdays_common[mon] + (mon == 1 && is_leap(year));
        if (days < md)
            break;
        days -= md;
    }
    tm[4] = mon;
    tm[3] = (int)days + 1;
    tm[8] = 0;
}

extern "C" int _gmtime64_s(void* out_tm, const int64_t* timep)
{
    if (!out_tm || !timep)
        return 22;
    seconds_to_tm(*timep, (int*)out_tm);
    return 0;
}

extern "C" int _localtime64_s(void* out_tm, const int64_t* timep)
{
    return _gmtime64_s(out_tm, timep);
}

extern "C" int _ctime64_s(char* buffer, size_t size, const int64_t* timep)
{
    static const char* wdays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (!buffer || !size || !timep)
        return 22;
    int tm[9];
    seconds_to_tm(*timep, tm);
    snprintf(buffer, size, "%s %s %02d %02d:%02d:%02d %04d\n",
        wdays[tm[6]], months[tm[4]], tm[3], tm[2], tm[1], tm[0], tm[5] + 1900);
    return 0;
}

extern "C" int _strdate_s(char* buffer, size_t size)
{
    if (!buffer || size < 9)
        return 22;
    host_call(msvcrt("_strdate"), (uintptr_t)buffer);
    buffer[size - 1] = 0;
    return 0;
}

extern "C" int _strtime_s(char* buffer, size_t size)
{
    if (!buffer || size < 9)
        return 22;
    host_call(msvcrt("_strtime"), (uintptr_t)buffer);
    buffer[size - 1] = 0;
    return 0;
}

extern "C" int _get_daylight(int* out) { if (out) *out = 0; return 0; }
extern "C" int _get_dstbias(long* out) { if (out) *out = 0; return 0; }
extern "C" int _get_timezone(long* out) { if (out) *out = 0; return 0; }
extern "C" int _get_tzname(size_t* len, char* buf, size_t size, int)
{
    const char* tz = "UTC";
    if (len) *len = 3;
    if (buf && size)
    {
        size_t n = size > 3 ? 3 : size - 1;
        mem_copy(buf, tz, n);
        buf[n] = 0;
    }
    return 0;
}
extern "C" int _configthreadlocale(int) { return 0; }

struct __attribute__((packed)) Stat64
{
    uint32_t dev;
    uint16_t ino;
    uint16_t mode;
    int16_t nlink;
    int16_t uid;
    int16_t gid;
    uint16_t pad0;
    uint32_t rdev;
    uint32_t pad1;
    int64_t size;
    int64_t atime;
    int64_t mtime;
    int64_t ctime;
};

struct __attribute__((packed)) Stat64i32
{
    uint32_t dev;
    uint16_t ino;
    uint16_t mode;
    int16_t nlink;
    int16_t uid;
    int16_t gid;
    uint16_t pad0;
    uint32_t rdev;
    int32_t size;
    int64_t atime;
    int64_t mtime;
    int64_t ctime;
};

extern "C" int stat64(const char* path, void* st)
{
    return (int)host_call(msvcrt("_stat64"), (uintptr_t)path, (uintptr_t)st);
}

extern "C" int wstat64(const wchar_t* path, void* st)
{
    return (int)host_call(msvcrt("_wstat64"), (uintptr_t)path, (uintptr_t)st);
}

extern "C" int fstat64(int fd, void* st)
{
    return (int)host_call(msvcrt("_fstat64"), (uintptr_t)fd, (uintptr_t)st);
}

static int stat64_to_i32(int ret, const Stat64& in, Stat64i32* out)
{
    if (ret != 0 || !out)
        return ret;
    out->dev = in.dev;
    out->ino = in.ino;
    out->mode = in.mode;
    out->nlink = in.nlink;
    out->uid = in.uid;
    out->gid = in.gid;
    out->rdev = in.rdev;
    out->size = (int32_t)in.size;
    out->atime = in.atime;
    out->mtime = in.mtime;
    out->ctime = in.ctime;
    return 0;
}

extern "C" int stat64i32(const char* path, Stat64i32* st)
{
    Stat64 tmp = {};
    return stat64_to_i32(stat64(path, &tmp), tmp, st);
}

extern "C" int wstat64i32(const wchar_t* path, Stat64i32* st)
{
    Stat64 tmp = {};
    return stat64_to_i32(wstat64(path, &tmp), tmp, st);
}

extern "C" int _fstat64i32(int fd, Stat64i32* st)
{
    Stat64 tmp = {};
    return stat64_to_i32(fstat64(fd, &tmp), tmp, st);
}

static void (*g_atexit[32])();
static int g_atexit_count;

extern "C" int atexit(void (*fn)())
{
    if (g_atexit_count >= (int)(sizeof(g_atexit) / sizeof(g_atexit[0])))
        return -1;
    g_atexit[g_atexit_count++] = fn;
    return 0;
}

extern "C" void riscvm_fini()
{
    while (g_atexit_count > 0)
        g_atexit[--g_atexit_count]();
}

static void* rv_malloc(size_t size)
{
    return (void*)host_call(msvcrt("malloc"), size ? size : 1);
}

static void rv_free(void* ptr)
{
    host_call(msvcrt("free"), (uintptr_t)ptr);
}

void* operator new(size_t size) { return rv_malloc(size); }
void* operator new[](size_t size) { return rv_malloc(size); }
void operator delete(void* p) noexcept { rv_free(p); }
void operator delete[](void* p) noexcept { rv_free(p); }
void operator delete(void* p, size_t) noexcept { rv_free(p); }
void operator delete[](void* p, size_t) noexcept { rv_free(p); }

// The input bitcode comes from x86_64-w64-mingw32 where size_t is mangled as
// unsigned long long; the RISC-V support object is built with an LP64 target
// where size_t mangles as unsigned long. Provide both spellings.
extern "C" void* _Znwy(unsigned long long size) { return rv_malloc((size_t)size); }
extern "C" void* _Znay(unsigned long long size) { return rv_malloc((size_t)size); }
extern "C" void _ZdlPvy(void* p, unsigned long long) { rv_free(p); }
extern "C" void _ZdaPvy(void* p, unsigned long long) { rv_free(p); }
