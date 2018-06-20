#pragma once
#include <Windows.h>

typedef struct _LXSS_STD_HANDLE {
    HANDLE Handle;
    BOOLEAN Pipe;
} LXSS_STD_HANDLE, *PLXSS_STD_HANDLE;

typedef struct _LXSS_STD_HANDLES {
    LXSS_STD_HANDLE StdIn;
    LXSS_STD_HANDLE StdOut;
    LXSS_STD_HANDLE StdErr;
} LXSS_STD_HANDLES, *PLXSS_STD_HANDLES;

typedef struct _LXSS_CONSOLE_DATA {
    ULONG ConsoleHandle;
} LXSS_CONSOLE_DATA, *PLXSS_CONSOLE_DATA;

typedef struct _WslInstance WslInstance, *pWslInstance;

/**
* PVOID ObjectStublessClient3;
* Register ADSS Bus Server
* 0x22002B == CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0A, METHOD_NEITHER, FILE_ANY_ACCESS)
**/
typedef HRESULT(__stdcall *pRegisterAdssBusServer) (
    _In_ pWslInstance *wslInstance,
    _In_ LPCSTR ServerName,
    _Out_ PULONG ServerHandle
    );

/**
* PVOID ObjectStublessClient4;
* InstanceID matches with tmpfs folder name
* But differs when CreateInstance is called
**/
typedef HRESULT (__stdcall *pGetId) (
    _In_ pWslInstance* wslInstance,
    _Out_ GUID* InstanceId
    );

/**
* PVOID ObjectStublessClient5;
* Get only current initiated Distribution ID
**/
typedef HRESULT(__stdcall *pGetDistroId) (
    _In_ pWslInstance* wslInstance,
    _Out_ GUID* DistroId
    );

//PVOID ObjectStublessClient6;
typedef HRESULT(__stdcall *pCreateLxProcess) ( /*unknown*/ );

/**
* PVOID ObjectStublessClient7;
* Connect with ADSS Bus Server 
* 0x22002F == CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0B, METHOD_NEITHER, FILE_ANY_ACCESS)
**/
typedef HRESULT(__stdcall *pConnectAdssBusServer) (
    _In_ pWslInstance *wslInstance,
    _In_ LPCSTR ServerName,
    _In_ PULONG ServerHandle
    );

struct _WslInstance {
    PVOID QueryInterface;
    PVOID AddRef;
    PVOID Release;
    pRegisterAdssBusServer RegisterAdssBusServer;//3
    pGetId GetId;//4
    pGetDistroId GetDistributionId;//5
    pCreateLxProcess CreateLxProcess;//6
    pConnectAdssBusServer ConnectAdssBusServer;//7
};
