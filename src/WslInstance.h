#pragma once
#include <Windows.h>

typedef struct _LXSS_STD_HANDLES {
    ULONG unknown[6];
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
    HANDLE hConout;
} LXSS_STD_HANDLES, *PLXSS_STD_HANDLES;

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
    HRESULT(__stdcall *CreateLxProcess) (
        _In_ pWslInstance* This,
        _In_opt_ PSTR CommandLine,
        _In_opt_ ULONG ArgumentCount,
        _In_opt_ PSTR* Arguments,
        _In_opt_ PWSTR CurrentDirectory,
        _In_opt_ PWSTR Environment,
        _In_opt_ PWSTR EnvSeparators,
        _In_opt_ ULONG EnvLength,
        _In_ PLXSS_STD_HANDLES StdHandles,
        _In_ ULONG ConsoleHandle,
        _In_ PWSTR LinuxUserName,
        _Out_ PHANDLE ProcessHandle,
        _Out_ PHANDLE ServerHandle
        );

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
