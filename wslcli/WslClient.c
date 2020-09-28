/*
 * This file is part of WslReverse project.
 * Licensed under the terms of the GNU General Public License v3 or later.
 * WslClient.c: WslClient COM interface usage example.
 */

#include <Windows.h>
#include "WslClient.h"

int main(void)
{
    HRESULT hRes;
    DWORD dwRes;
    HKEY hKey;
    DWORD Type = REG_SZ, cbData = MAX_PATH;
    char Data[MAX_PATH];

    HMODULE hMod;
    typedef HRESULT(WINAPI* DLLGETCLASSOBJECT)(const GUID* const rclsid, const GUID* const riid, void** ppv);
    DLLGETCLASSOBJECT pfnDllGetClassObject;
    IClassFactory* classFactory;
    IWslClient* wslClient;

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

    pfnDllGetClassObject = (DLLGETCLASSOBJECT)GetProcAddress(hMod, "DllGetClassObject");

    hRes = pfnDllGetClassObject(&CLSID_WslClient, &IID_IClassFactory, &classFactory);

    hRes = classFactory->lpVtbl->CreateInstance(classFactory, NULL, &IID_IWslClient, &wslClient);

    hRes = wslClient->lpVtbl->Main(wslClient, WSL_CLENT_ENTRY_WSL, L"", &dwRes);

    FreeLibrary(hMod);
    CoUninitialize();
    return 0;
}
