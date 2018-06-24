#include <Windows.h>
#include <stdio.h>

void Log(HRESULT result, PCWSTR function) {

    if (result == 0) {
        wprintf(L"%ls success\n", function);
    }
    else {
        wprintf(L"%ls failed error: %d\n", function, (result & 0xFFFF));
        exit(1);
    }
}

void Usage() {
    printf(
        "\nWslReverse -- (c) Copyright 2018 Biswapriyo Nath\n"
        "Licensed under GNU Public License version 3 or higher\n\n"
        "Use hidden COM interface of Windows Subsystem for Linux for fun\n"
        "Usage: WslReverse.exe [-] [option] [argument]\n\n"
        "Options:\n"
        "  -d, --get-id       [distribution name]      Get distribution ID.\n"
        "  -G, --get-default                           Get default distribution ID.\n"
        "  -g, --get-config   [distribution name]      Get distribution configuration.\n"
        "  -h, --help                                  Show list of options.\n"
        "  -i, --install      [distribution name]      Install distribution (run as administrator).\n"
        "  -r, --run          [distribution name]      Run a Linux binary (incomplete feature).\n"
        "  -S, --set-default  [distribution name]      Set default distribution.\n"
        "  -s, --set-config   [distribution name]      Set configuration for distribution.\n"
        "  -u, --uninstall    [distribution name]      Uninstall distribution.\n"
        "\n"
    );
    exit(1);
}

void PrintGuid(GUID* guid) {

    wchar_t szGuid[40];

    swprintf_s(
        szGuid,
        40,
        L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2],
        guid->Data4[3], guid->Data4[4], guid->Data4[5],
        guid->Data4[6], guid->Data4[7]
    );

    wprintf(L"%s\n", szGuid);
}
