#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <stdio.h>

static DWORD setDebugPrivilege(HANDLE hProcess, bool bEnablePrivilege)
{
    DWORD  dwLastError;
    HANDLE hToken = 0;
    if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        dwLastError = GetLastError();
        if (hToken)
            CloseHandle(hToken);
        return dwLastError;
    }
    TOKEN_PRIVILEGES tokenPrivileges;
    memset(&tokenPrivileges, 0, sizeof(TOKEN_PRIVILEGES));
    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
    {
        dwLastError = GetLastError();
        CloseHandle(hToken);
        return dwLastError;
    }
    tokenPrivileges.PrivilegeCount     = 1;
    tokenPrivileges.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tokenPrivileges.Privileges[0].Attributes = 0;
    AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    dwLastError = GetLastError();
    CloseHandle(hToken);
    return dwLastError;
}

DWORD findLsass()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!hSnapshot)
    {
        return 0;
    }

    PROCESSENTRY32 pe32 = {sizeof(PROCESSENTRY32)};
    if (!Process32First(hSnapshot, &pe32))
    {
        return 0;
    }
    do
    {
        puts(pe32.szExeFile);
    } while (Process32Next(hSnapshot, &pe32));

    return 1;
}

int main()
{
    if (setDebugPrivilege(GetCurrentProcess(), true) != ERROR_SUCCESS)
    {
        puts("Failed to get debug privilege");
    }

    auto lsassPid = findLsass();
    printf("lsass.exe: %lu", lsassPid);
}
