/*
 * This file is part of WslReverse project.
 * Licensed under the terms of the GNU General Public License v3 or later.
 * WslClient.c: WslClient COM interface usage example.
 */

#include <Windows.h>
#include "WslClient.h"

int main(void)
{
    HRESULT hRes = 0;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hRes = CoInitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_DEFAULT,
                                SecurityDelegation, NULL,
                                EOAC_STATIC_CLOAKING, NULL);

    IWslClient* wslClient = NULL;
    hRes = CoCreateInstance(&CLSID_WslClient,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            &IID_IWslClient,
                            (PVOID*)&wslClient);
    if (hRes)
        return 1;

    DWORD Result;
    hRes = wslClient->lpVtbl->Main(wslClient,
                        WSL_CLENT_ENTRY_WSL, L"", &Result);

    return 0;
}
