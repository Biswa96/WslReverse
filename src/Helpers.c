#include "WinInternal.h"
#include <stdio.h>

void
WINAPI
LogResult(HRESULT hResult, PWSTR Function)
{
    if(hResult < 0)
        wprintf(L"[-] ERROR %ld %ls\n", (hResult & 0xFFFF), Function);
}

void
WINAPI
LogStatus(NTSTATUS Status, PWSTR Function)
{
    if(Status < 0)
        wprintf(L"[-] NTSTATUS 0x%08lX %ls\n", Status, Function);
}

#define MsgSize 0x400

void
WINAPI
Log(ULONG Result, PWSTR Function)
{
    if (Result != 0)
    {
        wchar_t MsgBuffer[MsgSize];
        RtlZeroMemory(MsgBuffer, sizeof MsgBuffer);
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, Result, MsgSize, MsgBuffer, MsgSize, NULL);
        wprintf(L"%ls Error: %ld\n%ls\n", Function, Result, MsgBuffer);
    }
}

NTSTATUS
WINAPI
MbsToWcs(PSTR src, PUNICODE_STRING dst)
{
    NTSTATUS Status = 0;
    ANSI_STRING AnsiString;
    RtlZeroMemory(&AnsiString, sizeof AnsiString);

    Status = RtlInitAnsiStringEx(&AnsiString, src);
    if(Status == 0)
        Status = RtlAnsiStringToUnicodeString(dst, &AnsiString, TRUE);
    return Status;
}

void
WINAPI
Usage(void)
{
    wprintf(
        L"\nWslReverse -- (c) Copyright 2018-19 Biswapriyo Nath\n"
        L"Licensed under GNU Public License version 3 or higher\n\n"
        L"Use hidden COM interface of Windows Subsystem for Linux for fun\n"
        L"Usage: WslReverse.exe [-] [option] [argument]\n\n"
        L"Options:\n"
        L"  -b, --bus          [Distro]    Create own LxBus server (as administrator).\n"
        L"  -d, --get-id       [Distro]    Get distribution ID.\n"
        L"  -e, --export       [Distro]  [File Name] \n"
        L"                                 Exports selected distribution to a tar file.\n"
        L"  -G, --get-default              Get default distribution ID.\n"
        L"  -g, --get-config   [Distro]    Get distribution configuration.\n"
        L"  -h, --help                     Show this help information.\n"
        L"  -i, --install      [Distro]  [Install Folder]  [File Name] \n"
        L"                                 Install tar file as a new distribution.\n"
        L"  -l, --list                     List all distributions with pending ones.\n"
        L"  -r, --run          [Distro]    Run bash in provided distribution.\n"
        L"  -S, --set-default  [Distro]    Set default distribution.\n"
        L"  -s, --set-config   [Distro]    Set configuration for distribution.\n"
        L"  -t, --terminate    [Distro]    Terminate running distribution.\n"
        L"  -u, --uninstall    [Distro]    Uninstall distribution.\n"
        L"\n");
}
