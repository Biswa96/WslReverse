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

struct _WslInstance {
    PVOID QueryInterface;
    PVOID AddRef;
    PVOID Release;

    /**
    * PVOID ObjectStublessClient3;
    * Register ADSS Bus Server
    * 0x22002B == CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0A, METHOD_NEITHER, FILE_ANY_ACCESS)
    **/
    HRESULT(__stdcall *RegisterAdssBusServer) (
        _In_ pWslInstance *wslInstance,
        _In_ LPCSTR ServerName,
        _Out_ PULONG ServerHandle
        );

    /**
    * PVOID ObjectStublessClient4;
    * InstanceID matches with tmpfs folder name
    * But differs when CreateInstance is called
    **/
    HRESULT(__stdcall *GetInstanceId) (
        _In_ pWslInstance* wslInstance,
        _Out_ GUID* InstanceId
        );

    /**
    * PVOID ObjectStublessClient5;
    * Get only current initiated Distribution ID
    **/
    HRESULT(__stdcall *GetDistributionId) (
        _In_ pWslInstance* wslInstance,
        _Out_ GUID* DistroId
        );

    //PVOID ObjectStublessClient6;
    HRESULT(__stdcall *CreateLxProcess) ( /*unknown*/);

    /**
    * PVOID ObjectStublessClient7;
    * Connect with ADSS Bus Server
    * 0x22002F == CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0B, METHOD_NEITHER, FILE_ANY_ACCESS)
    **/
    HRESULT(__stdcall *ConnectAdssBusServer) (
        _In_ pWslInstance *wslInstance,
        _In_ LPCSTR ServerName,
        _In_ PULONG ServerHandle
        );

};
