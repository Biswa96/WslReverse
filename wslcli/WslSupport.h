//
// This file is part of WslReverse project.
// Licensed under the terms of the GNU General Public License v3 or later.
// WslSupport.h: WslSupport COM interface declaration and related CLSID and IID.
// Wrapper upon LxssUserSession without managing DistroId GUID.
//

#ifndef WSLSUPPORT_H
#define WSLSUPPORT_H

// {4F476546-B412-4579-B64C-123DF331E3D6}
static const GUID CLSID_LxssUserSession = {
    0x4F476546,
    0xB412,
    0x4579,
    { 0xB6, 0x4C, 0x12, 0x3D, 0xF3, 0x31, 0xE3, 0xD6 } };

// {46F3C96D-FFA3-42F0-B052-52F5E7ECBB08}
static const GUID IID_IWslSupport = {
    0x46F3C96D,
    0xFFA3,
    0x42F0,
    { 0xB0, 0x52, 0x52, 0xF5, 0xE7, 0xEC, 0xBB, 0x08 } };

typedef struct _IWslSupport IWslSupport;

struct _IWslSupportVtbl
{
    // IUnknown_QueryInterface_Proxy
    HRESULT (*QueryInterface)(
        _In_ IWslSupport *This,
        _In_ GUID *Riid,
        _COM_Outptr_ PVOID *ppvObject);

    // IUnknown_AddRef_Proxy
    ULONG (*AddRef)(
        _In_ IWslSupport *This);

    // IUnknown_Release_Proxy
    ULONG (*Release)(
        _In_ IWslSupport *This);

    // ObjectStublessClient3
    HRESULT (*RegisterDistribution)(
        _In_ IWslSupport *This,
        _In_z_ PCWSTR DistroName,
        _In_ ULONG Version,
        _In_ HANDLE TarGzFileHandle,
        _In_ HANDLE Unknown,
        _In_z_ PCWSTR BasePath);

    // ObjectStublessClient4
    HRESULT (*UnregisterDistribution)(
        _In_ IWslSupport *This,
        _In_z_ PCWSTR DistroName);

    // ObjectStublessClient5
    HRESULT (*GetDistributionConfiguration)(
        _In_ IWslSupport *This,
        _In_ PCWSTR DistroName,
        _Out_ PULONG Version,
        _Out_ PULONG DefaultUid,
        _Out_ PULONG EnvironmentCount,
        _Outptr_ PSTR **DefaultEnvironment,
        _Out_ PULONG DistroFlags);

    // ObjectStublessClient6
    HRESULT (*SetDistributionConfiguration)(
        _In_ IWslSupport *This,
        _In_z_ PCWSTR DistroName,
        _In_opt_ ULONG DefaultUid,
        _In_opt_ ULONG DistroFlags);

    // ObjectStublessClient7
    HRESULT (*ListDistributions)(
        _In_ IWslSupport *This,
        _Out_ PULONG DistroCount,
        _Outptr_ PWSTR **DistroName);

    // ObjectStublessClient8
    HRESULT (*CreateInstance)(
        _In_ IWslSupport *This,
        _In_z_ PWSTR DistroName,
        _In_opt_ ULONG InstanceFlags);

    // ObjectStublessClient9
    HRESULT (*Shutdown)(
        _In_ IWslSupport *This);
};

struct _IWslSupport
{
    struct _IWslSupportVtbl *lpVtbl;
};

#endif // WSLSUPPORT_H
