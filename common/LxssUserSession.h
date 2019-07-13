#ifndef LXSSUSERSESSION_H
#define LXSSUSERSESSION_H

static const GUID CLSID_LxssUserSession = {
    0x4F476546,
    0xB412,
    0x4579,
    { 0xB6, 0x4C, 0x12, 0x3D, 0xF3, 0x31, 0xE3, 0xD6 } };

static const GUID IID_ILxssUserSession = {
    0x536A6BCF,
    0xFE04,
    0x41D9,
    { 0xB9, 0x78, 0xDC, 0xAC, 0xA9, 0xA9, 0xB5, 0xB9 } };

typedef enum _LXSS_DISTRIBUTION_STATES {
    Stopped = 1,
    Running,
    Installing,
    Uninstalling,
    Converting
} LXSS_DISTRIBUTION_STATES;

typedef struct _LXSS_ENUMERATE_INFO {
    GUID DistributionID;
    LXSS_DISTRIBUTION_STATES State;
    ULONG Version;
    ULONG Default;
} LXSS_ENUMERATE_INFO, *PLXSS_ENUMERATE_INFO;

typedef enum _WSL_DISTRIBUTION_FLAGS {
    WSL_DISTRIBUTION_FLAGS_NONE = 0,
    WSL_DISTRIBUTION_FLAGS_ENABLE_INTEROP = 1,
    WSL_DISTRIBUTION_FLAGS_APPEND_NT_PATH = 2,
    WSL_DISTRIBUTION_FLAGS_ENABLE_DRIVE_MOUNTING = 4,
    WSL_DISTRIBUTION_FLAGS_DEFAULT = 7
} WSL_DISTRIBUTION_FLAGS;

typedef struct _LXSS_STD_HANDLE {
    ULONG Handle;
    ULONG Pipe;
} LXSS_STD_HANDLE, *PLXSS_STD_HANDLE;

typedef struct _LXSS_STD_HANDLES {
    LXSS_STD_HANDLE StdIn;
    LXSS_STD_HANDLE StdOut;
    LXSS_STD_HANDLE StdErr;
} LXSS_STD_HANDLES, *PLXSS_STD_HANDLES;

typedef struct _ILxssUserSession ILxssUserSession;

struct _ILxssUserSessionVtbl
{
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* riid,
        _Out_ PVOID* ppv);

    ULONG(STDMETHODCALLTYPE *AddRef)(
        _In_ ILxssUserSession* This);

    ULONG(STDMETHODCALLTYPE *Release)(
        _In_ ILxssUserSession* This);

    /**
    * PVOID ObjectStublessClient3;
    * Create new LxssInstance or get the running one
    * If no DistroId is provided get the default distribuiton GUID
    **/
    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
        _In_ ILxssUserSession* wslSession,
        _In_opt_ GUID* DistroId,
        _In_ ULONG InitializeCount);

    /**
    * PVOID ObjectStublessClient4;
    * If Version is not provided DefaultVersion (REG_DWORD) is used
    * Version greater than 2 (Two) results invalid parameter
    * Command: "System32/lxss/tools/bsdtar -C /rootfs -x -p --xattrs --no-acls -f -"
    **/
    HRESULT(STDMETHODCALLTYPE *RegisterDistribution)(
        _In_ ILxssUserSession* wslSession,
        _In_ PWSTR DistributionName,
        _In_opt_ ULONG Version,
        _In_ HANDLE TarGzFileHandle,
        _In_ PWSTR BasePath,
        _Out_ GUID* DistroId);

    /* PVOID ObjectStublessClient5; */
    HRESULT(STDMETHODCALLTYPE *RegisterDistributionFromPipe)(
        _In_ ILxssUserSession* wslSession,
        _In_ PWSTR DistributionName,
        _In_opt_ ULONG Version,
        _In_ HANDLE TarGzFileHandle,
        _In_ PWSTR BasePath,
        _Out_ GUID* DistroId);

    /**
    * PVOID ObjectStublessClient6;
    * Query State registry value in Lxss registry key
    * Allows only if that value is ONE means installed
    **/
    HRESULT(STDMETHODCALLTYPE *GetDistributionId)(
        _In_ ILxssUserSession* wslSession,
        _In_ PWSTR DistroName,
        _In_ ULONG EnableEnumerate,
        _Out_ GUID* DistroId);

    /**
    * PVOID ObjectStublessClient7;
    * If DistroId is NULL it will be default distribution
    **/
    HRESULT(STDMETHODCALLTYPE *TerminateDistribution)(
        _In_ ILxssUserSession* wslSession,
        _In_opt_ GUID* DistroId);

    /**
    * PVOID ObjectStublessClient8;
    * Query GUID with already registered in Lxss key
    * Write State registry value as FOUR means uninstalling
    **/
    HRESULT(STDMETHODCALLTYPE *UnregisterDistribution)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* DistroId);

    /**
    * PVOID ObjectStublessClient9;
    * Flags should be less than or equal to SEVEN
    * Writes Environment variables iff EnvironmentCount present
    **/
    HRESULT(STDMETHODCALLTYPE *ConfigureDistribution)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* DistroId,
        _In_opt_ PCSTR KernelCommandLine,
        _In_opt_ ULONG DefaultUid,
        _In_opt_ ULONG EnvironmentCount,
        _In_opt_ PCSTR* DefaultEnvironment,
        _In_ ULONG Flags);

    /* PVOID ObjectStublessClient10; */
    HRESULT(STDMETHODCALLTYPE *GetDistributionConfiguration)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* DistroId,
        _Out_ PWSTR* DistributionName,
        _Out_ PULONG Version,
        _Out_ PWSTR* BasePath,
        _Out_ PSTR* KernelCommandLine,
        _Out_ PULONG DefaultUid,
        _Out_ PULONG EnvironmentCount,
        _Out_ PSTR** DefaultEnvironment,
        _Out_ PULONG Flags);

    /**
    * PVOID ObjectStublessClient11;
    * Query DefaultDistribution registry string
    * Delete that if BasePath doesn't exist
    **/
    HRESULT(STDMETHODCALLTYPE *GetDefaultDistribution)(
        _In_ ILxssUserSession* wslSession,
        _Out_ GUID* DistroId);

    /**
    * PVOID ObjectStublessClient12;
    * Query State registry value should be ONE
    * Write DefaultDistribution registry with provided GUID
    **/
    HRESULT(STDMETHODCALLTYPE *SetDefaultDistribution)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* DistroId);

    /**
    * PVOID ObjectStublessClient13;
    * Free allocated memory with CoTaskMemFree
    **/
    HRESULT(STDMETHODCALLTYPE *EnumerateDistributions)(
        _In_ ILxssUserSession* wslSession,
        _Out_ PULONG DistroCount,
        _Out_ PLXSS_ENUMERATE_INFO* DistroInfo);

    /**
    * PVOID ObjectStublessClient14;
    * LxInstanceId is same with HyperV VM ID
    * LxProcessHandle and ServerHandle are NULL when SOCKETs are not aka. WSL2
    **/
    HRESULT(STDMETHODCALLTYPE *CreateLxProcess)(
        _In_ ILxssUserSession* wslSession,
        _In_opt_ GUID* DistroId,
        _In_opt_ PSTR CommandLine,
        _In_opt_ ULONG ArgumentCount,
        _In_opt_ PSTR* Arguments,
        _In_opt_ PWSTR CurrentDirectory,
        _In_opt_ PWSTR SharedEnvironment,
        _In_opt_ PWSTR ProcessEnvironment,
        _In_opt_ SIZE_T EnvironmentLength,
        _In_opt_ PWSTR LinuxUserName,
        _In_opt_ USHORT WindowWidthX,
        _In_opt_ USHORT WindowHeightY,
        _In_ ULONG ConsoleHandle,
        _In_ PLXSS_STD_HANDLES StdHandles,
        _Out_ GUID* InitiatedDistroId,
        _Out_ GUID* LxInstanceId,
        _Out_ PHANDLE LxProcessHandle,
        _Out_ PHANDLE ServerHandle,
        _Out_ SOCKET* VmModeSocketA,
        _Out_ SOCKET* VmModeSocketB,
        _Out_ SOCKET* VmModeSocketC,
        _Out_ SOCKET* IpcServerSocket);

    /**
    * PVOID ObjectStublessClient15;
    * E_NOINTERFACE No such interface supported
    **/
    HRESULT(STDMETHODCALLTYPE *SetVersion)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* DistroId,
        _In_ ULONG Version);

    /**
    * PVOID ObjectStublessClient16;
    * Allow only when RootLxBusAccess REG_DWORD enabled
    * Not for WSL2 i.e. Hyper-V VM
    **/
    HRESULT(STDMETHODCALLTYPE *RegisterLxBusServer)(
        _In_ ILxssUserSession* wslSession,
        _In_opt_ GUID* DistroId,
        _In_ PSTR LxBusServerName,
        _Out_ PHANDLE ServerHandle);

    /**
    * PVOID ObjectStublessClient17;
    * Command: "System32/lxss/tools/bsdtar -C /rootfs -c --one-file-system --xattrs -f - ."
    **/
    HRESULT(STDMETHODCALLTYPE *ExportDistribution)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* DistroId,
        _In_ HANDLE FileHandle);

    /**
    * PVOID ObjectStublessClient18;
    * The filename from standard output.
    **/
    HRESULT(STDMETHODCALLTYPE *ExportDistributionFromPipe)(
        _In_ ILxssUserSession* wslSession,
        _In_ GUID* DistroId,
        _In_ HANDLE FileHandle);

    /* PVOID ObjectStublessClient19; */
    HRESULT(STDMETHODCALLTYPE *Shutdown)(
        _In_ ILxssUserSession *wslSession);
};

struct _ILxssUserSession
{
    struct _ILxssUserSessionVtbl* lpVtbl;
};

#endif // LXSSUSERSESSION_H
