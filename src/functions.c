#include <Windows.h>
#include <stdio.h>

#define MsgSize 0x400

void Log(HRESULT Result, PWSTR Function)
{
    if (Result == 0)
    {
        wprintf(L"%ls Success\n", Function);
    }
    else
    {
        wchar_t MsgBuffer[MsgSize];
        memset(MsgBuffer, 0, MsgSize * sizeof(wchar_t));
        FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, Result, MsgSize, MsgBuffer, MsgSize, NULL);
        wprintf(L"%ls Error: %ld\n%ls\n", Function, (Result & 0xFFFF), MsgBuffer);
        exit(EXIT_FAILURE);
    }
}

void Usage() {
    wprintf(
        L"\nWslReverse -- (c) Copyright 2018 Biswapriyo Nath\n"
        L"Licensed under GNU Public License version 3 or higher\n\n"
        L"Use hidden COM interface of Windows Subsystem for Linux for fun\n"
        L"Usage: WslReverse.exe [-] [option] [argument]\n\n"
        L"Options:\n"
        L"  -d, --get-id       [distribution name]      Get distribution ID.\n"
        L"  -G, --get-default                           Get default distribution ID.\n"
        L"  -g, --get-config   [distribution name]      Get distribution configuration.\n"
        L"  -h, --help                                  Show list of options.\n"
        L"  -i, --install      [distribution name]      Install distribution (run as administrator).\n"
        L"  -r, --run          [distribution name]      Run a Linux binary (incomplete feature).\n"
        L"  -S, --set-default  [distribution name]      Set default distribution.\n"
        L"  -s, --set-config   [distribution name]      Set configuration for distribution.\n"
        L"  -t, --terminate    [distribution name]      Terminate running distribution.\n"
        L"  -u, --uninstall    [distribution name]      Uninstall distribution.\n"
        L"\n"
    );
    return;
}

#define GUID_STRING 40

void PrintGuid(GUID* guid) {

    wchar_t szGuid[GUID_STRING];

    swprintf(
        szGuid,
        GUID_STRING,
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2],
        guid->Data4[3], guid->Data4[4], guid->Data4[5],
        guid->Data4[6], guid->Data4[7]
    );

    wprintf(L"%ls\n", szGuid);
}
