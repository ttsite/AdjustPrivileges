// test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <stdio.h>


void RunShell()
{
    TCHAR  szComspec[MAX_PATH]{};

    GetEnvironmentVariable(L"COMSPEC", szComspec, MAX_PATH);

    SHELLEXECUTEINFO sei;
    sei.cbSize = sizeof(sei);
    sei.hwnd = 0;
    sei.lpVerb = L"Open";
    sei.lpFile = szComspec;
    sei.lpParameters = nullptr;//可运行个whoami.L"/c whoami /all"
    sei.lpDirectory = 0;
    sei.nShow = SW_SHOW;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    ShellExecuteEx(&sei);
}


int main()
{
    HANDLE HDevice = CreateFileW(L"\\\\.\\AdjustPrivilege",
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);
    if (HDevice == INVALID_HANDLE_VALUE) {

        return GetLastError();
    }

    RunShell();

    (void)getchar();

    CloseHandle(HDevice);

    return GetLastError();
}
