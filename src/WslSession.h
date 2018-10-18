#ifndef WSLSESSION_H
#define WSLSESSION_H

#include <Windows.h>

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

static const GUID IID_ILxssInstance = {
    0x8F9E8123,
    0x58D4,
    0x484A,
    { 0xAC, 0x25, 0x7E, 0xF7, 0xD5, 0xF7, 0x44, 0x8F } };

typedef enum _WslDefaultUID {
    RootUser = 0,
    NormalUser = 1000
} WslDefaultUID;

typedef enum _LxssDistributionState {
    Installed = 1,
    ToBeInstall = 2,
    Installing = 3,
    Uninstalling = 4,
    Upgrading = 5
} LxssDistributionState;

typedef enum _WSL_DISTRIBUTION_FLAGS {
    WSL_DISTRIBUTION_FLAGS_NONE = 0,
    WSL_DISTRIBUTION_FLAGS_ENABLE_INTEROP = 1,
    WSL_DISTRIBUTION_FLAGS_APPEND_NT_PATH = 2,
    WSL_DISTRIBUTION_FLAGS_ENABLE_DRIVE_MOUNTING = 4,
    WSL_DISTRIBUTION_FLAGS_DEFAULT = 7
} WSL_DISTRIBUTION_FLAGS;

typedef struct _WslSession WslSession, *PWslSession;

struct _WslSession {
    PVOID QueryInterface;
    PVOID AddRef;
    PVOID Release;

    /**
    * PVOID ObjectStublessClient3;
    * Create new LxssInstance or get the running one
    * If no DistroId is present get default one
    **/
    HRESULT(STDMETHODCALLTYPE *CreateInstance)(
        _In_ PWslSession* wslSession,
        _In_opt_ GUID* DistroId,
        _In_ ULONG CreateWithThrow,
        _In_ const GUID* ILxssInstance,
        _Out_ PVOID* wslInstance
        );

    /**
    * PVOID ObjectStublessClient4;
    * Write State to THREE means installing
    * Run as administrator otherwise E_ACCESSDENIED error
    * Version Must be TWO for newer installation
    **/
#ifdef RS_FIVE
    HRESULT(STDMETHODCALLTYPE *RegisterDistribution)(
        _In_ PWslSession* wslSession,
        _In_ PWSTR DistributionName,
        _In_ ULONG State,
        _In_ PWSTR TarGzFilePath,
        _In_ PWSTR BasePath,
        _In_ GUID* DistroId
        );
#else // 19H1 builds
    HRESULT(STDMETHODCALLTYPE *RegisterDistribution)(
        _In_ PWslSession* wslSession,
        _In_ PWSTR DistributionName,
        _In_ ULONG State,
        _In_ HANDLE TarGzFileHandle,
        _In_ PWSTR BasePath,
        _In_ GUID* DistroId
        );
#endif // RS_FIVE

    /**
    * PVOID ObjectStublessClient5;
    * Query State registry value in Lxss registry key
    * Allows only if that value is ONE means installed
    **/
    HRESULT(STDMETHODCALLTYPE *GetDistributionId)(
        _In_ PWslSession* wslSession,
        _In_ PWSTR DistroName,
        _In_ ULONG State,
        _Out_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient6;
    * If DistroId is NULL it will be default distribution
    * Use IServerSecurity interface and terminates any process
    **/
    HRESULT(STDMETHODCALLTYPE *TerminateDistribution)(
        _In_ PWslSession* wslSession,
        _In_opt_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient7;
    * Query GUID with already registered in Lxss key
    * Write State registry value as FOUR means uninstalling
    **/
    HRESULT(STDMETHODCALLTYPE *UnregisterDistribution)(
        _In_ PWslSession* wslSession,
        _In_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient8;
    * Flags should be less than or equal to SEVEN
    * Writes Environment variables iff EnvironmentCount present
    **/
    HRESULT(STDMETHODCALLTYPE *ConfigureDistribution)(
        _In_ PWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_opt_ PCSTR KernelCommandLine,
        _In_opt_ ULONG DefaultUid,
        _In_opt_ ULONG EnvironmentCount,
        _In_opt_ PCSTR* DefaultEnvironment,
        _In_ ULONG Flags
        );

    /**
    * PVOID ObjectStublessClient9;
    * Enumerate all value in Lxss\DistroId registry key
    * Query CurrentControlSet\services\LxssManager\DistributionFlags
    **/
    HRESULT(STDMETHODCALLTYPE *GetDistributionConfiguration)(
        _In_ PWslSession* wslSession,
        _In_ GUID* DistroId,
        _Out_ PWSTR* DistributionName,
        _Out_ PULONG Version,
        _Out_ PWSTR* BasePath,
        _Out_ PSTR* KernelCommandLine,
        _Out_ PULONG DefaultUid,
        _Out_ PULONG EnvironmentCount,
        _Out_ PSTR** DefaultEnvironment,
        _Out_ PULONG Flags
        );

    /**
    * PVOID ObjectStublessClient10;
    * Query DefaultDistribution registry string
    * Delete that if BasePath doesn't exist
    **/
    HRESULT(STDMETHODCALLTYPE *GetDefaultDistribution)(
        _In_ PWslSession* wslSession,
        _Out_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient11;
    * Query State registry value should be ONE
    * Write DefaultDistribution registry with provided GUID
    **/
    HRESULT(STDMETHODCALLTYPE *SetDefaultDistribution)(
        _In_ PWslSession* wslSession,
        _In_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient12;
    * Query State registry value should be ONE
    * Returns GUID list in 16 bytes offsets
    **/
    HRESULT(STDMETHODCALLTYPE *EnumerateDistributions)(
        _In_ PWslSession* wslSession,
        _In_ ULONG State,
        _Out_ PULONG DistroCount,
        _Out_ GUID** DistroIdList
        );

    /**
    * PVOID ObjectStublessClient13;
    * Related with hidden WSL_VM_Mode feature
    **/
    HRESULT(STDMETHODCALLTYPE *CreateLxProcess)(
        _In_ PWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_ PCSTR unknown
        );

    /**
    * PVOID ObjectStublessClient14;
    * Query Distribution Configuration and Version is TWO for newer installation
    * Write State registry to FIVE means upgrading
    **/
    HRESULT(STDMETHODCALLTYPE *BeginUpgradeDistribution)(
        _In_ PWslSession* wslSession,
        _In_ GUID* DistroId,
        _Out_ PULONG Version,
        _Out_ PWSTR* BasePath
        );

    /**
    * PVOID ObjectStublessClient15;
    * Query Distribution Configuration and Write State registry to ONE means installed
    * Write Version registry to TWO for newer installation
    **/
    HRESULT(STDMETHODCALLTYPE *FinishUpgradeDistribution)(
        _In_ PWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_ ULONG Version
        );
};

#endif //WSLSESSION_H
