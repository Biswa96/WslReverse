#include <Windows.h>
#include <stdio.h>

void Log(
    unsigned long Result,
    wchar_t* Function)
{
    wchar_t* MsgBuffer = NULL;
    unsigned long tChars = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        Result,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (wchar_t*)&MsgBuffer,
        0,
        NULL);

    if (tChars)
    {
        wprintf(
            L"[+] %ls Status: %ld\t %ls",
            Function, (Result & 0xFFFF), MsgBuffer);
    }
    else
    {
        wprintf(
            L"[+] %ls Status: %ld\n",
            Function, (Result & 0xFFFF));
    }


    LocalFree(MsgBuffer);
}

void Usage(
    void)
{
    wprintf(
        L"\nWslReverse -- (c) Copyright 2018 Biswapriyo Nath\n"
        L"Licensed under GNU Public License version 3 or higher\n\n"
        L"Use hidden COM interface of Windows Subsystem for Linux for fun\n"
        L"Usage: WslReverse.exe [-] [option] [argument]\n\n"
        L"Options:\n"
        L"  -d, --get-id       [distribution name]      Get distribution ID.\n"
        L"  -e, --export       [distribution name]      Exports the distribution to a tar file.\n"
        L"  -G, --get-default                           Get default distribution ID.\n"
        L"  -g, --get-config   [distribution name]      Get distribution configuration.\n"
        L"  -h, --help                                  Show list of options.\n"
        L"  -i, --install      [distribution name]      Install distribution (run as administrator).\n"
        L"  -l, --list                                  List all distributions with pending ones.\n"
        L"  -r, --run          [distribution name]      Run bash in provided distribution.\n"
        L"  -S, --set-default  [distribution name]      Set default distribution.\n"
        L"  -s, --set-config   [distribution name]      Set configuration for distribution.\n"
        L"  -t, --terminate    [distribution name]      Terminate running distribution.\n"
        L"  -u, --uninstall    [distribution name]      Uninstall distribution.\n"
        L"\n"
    );
}

#define GUID_STRING 40

void PrintGuid(
    GUID* id,
    wchar_t* string)
{
    _snwprintf_s(
        string,
        GUID_STRING,
        GUID_STRING,
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        id->Data1, id->Data2, id->Data3,
        id->Data4[0], id->Data4[1], id->Data4[2],
        id->Data4[3], id->Data4[4], id->Data4[5],
        id->Data4[6], id->Data4[7]);
}
