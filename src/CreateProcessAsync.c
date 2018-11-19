#include "WinInternal.h" // some defined expression
#include "LxBus.h" // For IOCTLs values
#include <stdio.h>

void CreateProcessAsync(
    PTP_CALLBACK_INSTANCE Instance,
    HANDLE ClientHandle,
    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(ClientHandle);
    UNREFERENCED_PARAMETER(Work);

    wprintf(L"[*] CreateProcessAsync ClientHandle: %ld\n", ToULong(ClientHandle));

    // Coming Soon...
}
