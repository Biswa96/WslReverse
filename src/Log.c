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
        L"  -b, --bus          [distribution name]      Create own LxBus server (as administrator).\n"
        L"  -d, --get-id       [distribution name]      Get distribution ID.\n"
        L"  -e, --export       [distribution name]      Exports selected distribution to a tar file.\n"
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
        L"\n");
}
