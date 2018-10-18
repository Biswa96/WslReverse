#ifndef WSLINSTANCE_H
#define WSLINSTANCE_H

#include <Windows.h>

typedef struct _LXSS_STD_HANDLE {
    ULONG Handle;
    ULONG Pipe;
} LXSS_STD_HANDLE, *PLXSS_STD_HANDLE;

typedef struct _LXSS_STD_HANDLES {
    LXSS_STD_HANDLE StdIn;
    LXSS_STD_HANDLE StdOut;
    LXSS_STD_HANDLE StdErr;
} LXSS_STD_HANDLES, *PLXSS_STD_HANDLES;

typedef struct _WslInstance WslInstance, *PWslInstance;

struct _WslInstance {
    PVOID QueryInterface;
    PVOID AddRef;
    PVOID Release;

    /**
    * PVOID ObjectStublessClient3;
    * lxssmanager is registered by LxssManager service
    * Choose LxBusServerName other than lxssmanager
    * 0x22002B == CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0A, METHOD_NEITHER, FILE_ANY_ACCESS)
    **/
    HRESULT(STDMETHODCALLTYPE *RegisterLxBusServer)(
        _In_ PWslInstance *wslInstance,
        _In_ PSTR LxBusServerName,
        _Out_ PHANDLE ServerHandle
        );

    /**
    * PVOID ObjectStublessClient4;
    * InstanceID matches with tmpfs folder name
    * But differs when CreateInstance is called
    **/
    HRESULT(STDMETHODCALLTYPE *GetInstanceId)(
        _In_ PWslInstance* wslInstance,
        _Out_ GUID* InstanceId
        );

    /**
    * PVOID ObjectStublessClient5;
    * Get only current initiated Distribution ID
    **/
    HRESULT(STDMETHODCALLTYPE *GetDistributionId)(
        _In_ PWslInstance* wslInstance,
        _Out_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient6;
    * Connected with TEN IOCTLs in LxCore driver
    * If succeed creates a inherited ConHost process
    **/
    HRESULT(STDMETHODCALLTYPE *CreateLxProcess)(
        _In_ PWslInstance* This,
        _In_opt_ PSTR CommandLine,
        _In_opt_ ULONG ArgumentCount,
        _In_opt_ PSTR* Arguments,
        _In_opt_ PWSTR CurrentDirectory,
        _In_opt_ PWSTR SharedEnvironment,
        _In_opt_ PWSTR ProcessEnvironment,
        _In_opt_ SIZE_T EnvironmentLength,
        _In_ PLXSS_STD_HANDLES StdHandles,
        _In_ ULONG ConsoleHandle,
        _In_opt_ PWSTR LinuxUserName,
        _Out_ PHANDLE ProcessHandle,
        _Out_ PHANDLE ServerHandle
        );

    // Removed from 19H1 builds remains in /init file
#ifdef RS_FIVE
    /**
    * PVOID ObjectStublessClient7;
    * Connect with LxBus Server
    * 0x22002F == CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0B, METHOD_NEITHER, FILE_ANY_ACCESS)
    **/
    HRESULT(STDMETHODCALLTYPE *ConnectLxBusServer)(
        _In_ PWslInstance *wslInstance,
        _In_ PSTR LxBusServerName,
        _Out_ PHANDLE ServerHandle
        );
#else
#endif // RS_FIVE

};

#endif //WSLINSTANCE_H
