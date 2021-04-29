/*
 * This file is part of WslReverse project.
 * Licensed under the terms of the GNU General Public License v3 or later.
 * WslClient.c: WslClient COM interface usage example.
 */

#include <stdio.h>
#include <Windows.h>
#include "WslClient.h"
#include "WslSupport.h"

int main(void)
{
    HRESULT hRes;
    DWORD dwRes;
    HKEY hKey;
    DWORD Type = REG_SZ, cbData = MAX_PATH;
    char Data[MAX_PATH];

    HMODULE hMod = NULL;
    typedef HRESULT(WINAPI* DLLGETCLASSOBJECT)(const GUID* const rclsid, const GUID* const riid, void** ppv);
    DLLGETCLASSOBJECT pfnDllGetClassObject;
    IClassFactory* classFactory = NULL;
    IWslClient* wslClient = NULL;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hRes = CoInitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_DEFAULT,
                                SecurityDelegation, NULL,
                                EOAC_STATIC_CLOAKING, NULL);
#if 0
    hRes = CoCreateInstance(&CLSID_WslClient,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            &IID_IWslClient,
                            (PVOID*)&wslClient);
#endif

    dwRes = RegOpenKeyExA(
        HKEY_CLASSES_ROOT,
        "CLSID\\{615A13BE-241D-48B1-89B0-8E1D40FFD287}\\InprocServer32",
        0, KEY_READ, &hKey);

    dwRes = RegQueryValueExA(hKey, NULL, NULL, &Type, (PBYTE)Data, &cbData);
    dwRes = RegCloseKey(hKey);

    hMod = LoadLibraryExA(Data, NULL, 0);
    if (!hMod)
        goto Cleanup;

    pfnDllGetClassObject = (DLLGETCLASSOBJECT)GetProcAddress(hMod, "DllGetClassObject");

    hRes = pfnDllGetClassObject(&CLSID_WslClient, &IID_IClassFactory, &classFactory);

    hRes = classFactory->lpVtbl->CreateInstance(classFactory, NULL, &IID_IWslClient, &wslClient);

    hRes = wslClient->lpVtbl->Main(wslClient, WSL_CLENT_ENTRY_WSL, L"", &dwRes);

    if (wslClient)
        wslClient->lpVtbl->Release(wslClient);

    if (classFactory)
        classFactory->lpVtbl->Release(classFactory);

Cleanup:
    if (hMod)
        FreeLibrary(hMod);

    CoUninitialize();
    return 0;
}

int VerboseDistroList(void)
{
    HRESULT hRes = 0;
    IWslSupport* wslSupport = NULL;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hRes = CoInitializeSecurity(NULL, -1, NULL, NULL,
                                RPC_C_AUTHN_LEVEL_DEFAULT,
                                SecurityDelegation, NULL,
                                EOAC_STATIC_CLOAKING, NULL);

    hRes = CoCreateInstance(&CLSID_LxssUserSession,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            &IID_IWslSupport,
                            (PVOID*)&wslSupport);
    if (hRes)
        goto Cleanup;

    ULONG DistroCount, Version, DefaultUid, Flags, EnvCount;
    PSTR *Environ;
    PWSTR *DistroName;

    hRes = wslSupport->lpVtbl->ListDistributions(wslSupport, &DistroCount, &DistroName);
    if (hRes)
        goto Cleanup;

    for (ULONG i = 0; i < DistroCount; i++)
    {
        hRes = wslSupport->lpVtbl->GetDistributionConfiguration(
            wslSupport, DistroName[i], &Version, &DefaultUid, &EnvCount, &Environ, &Flags);
        if (hRes)
            goto Cleanup;

        wprintf(L"[%lu] %ls %lu %lu %lu", (i + 1), DistroName[i], Version, DefaultUid, Flags);
        for (ULONG j = 0; j < EnvCount; j++)
        {
            printf(" %s", Environ[j]);
        }
        printf("\n");
    }

Cleanup:
    if (wslSupport)
        wslSupport->lpVtbl->Release(wslSupport);
    CoUninitialize();
    return 0;
}
