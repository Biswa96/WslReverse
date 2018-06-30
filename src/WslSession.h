#pragma once
#include <Windows.h>

const GUID CLSID_LxssUserSession = { 0x4F476546, 0xB412, 0x4579,{ 0xB6, 0x4C, 0x12, 0x3D, 0xF3, 0x31, 0xE3, 0xD6 } };
const GUID IID_ILxssUserSession = { 0x536A6BCF, 0xFE04, 0x41D9,{ 0xB9, 0x78, 0xDC, 0xAC, 0xA9, 0xA9, 0xB5, 0xB9 } };
const GUID IID_ILxssInstance = { 0x8F9E8123, 0x58D4, 0x484A,{ 0xAC, 0x25, 0x7E, 0xF7, 0xD5, 0xF7, 0x44, 0x8F } };

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

typedef struct _WslSession WslSession, *pWslSession;

struct _WslSession {
    PVOID QueryInterface;
    PVOID AddRef;
    PVOID Release;

    //PVOID ObjectStublessClient3;
    HRESULT(__stdcall *CreateInstance) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_ ULONG Create,
        _In_ const GUID* ILxssInstance,
        _Out_ PVOID wslInstance
        );

    /**
    * PVOID ObjectStublessClient4;
    * Write State to THREE means installing
    * Run as administrator otherwise E_ACCESSDENIED error
    * Version Must be TWO for newer installation
    **/
    HRESULT(__stdcall *RegisterDistribution) (
        _In_ pWslSession* wslSession,
        _In_ LPWSTR DistributionName,
        _In_ ULONG State,
        _In_ LPWSTR TarGzFilePath,
        _In_ LPWSTR BasePath,
        _In_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient5;
    * Query State registry value in Lxss registry key
    * Allows only if that value is ONE means installed
    **/
    HRESULT(__stdcall *GetDistributionId) (
        _In_ pWslSession* wslSession,
        _In_ LPWSTR DistroName,
        _In_ ULONG State,
        _Out_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient6;
    * Set always be TRUE othewise E_INVALIDARG error
    * State always be ONE means installed
    **/
    HRESULT(__stdcall *SetDistributionState) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_ BOOL Set,
        _In_ PULONG State
        );

    /**
    * PVOID ObjectStublessClient7;
    * Query State registry value
    * Fails if State is not present
    **/
    HRESULT(__stdcall *QueryDistributionState) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _Out_ LPDWORD State
        );

    /**
    * PVOID ObjectStublessClient8;
    * Query GUID with already registered in Lxss key
    * Write State registry value as FOUR means uninstalling
    **/
    HRESULT(__stdcall *UnregisterDistribution) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient9;
    * Flags should be less than or equal to SEVEN
    * Writes Environment variables iff EnvironmentCount present
    **/
    HRESULT(__stdcall *ConfigureDistribution) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_opt_ LPCSTR KernelCommandLine,
        _In_opt_ ULONG DefaultUid,
        _In_opt_ ULONG EnvironmentCount,
        _In_opt_ LPCSTR* DefaultEnvironment,
        _In_ ULONG Flags
        );

    /**
    * PVOID ObjectStublessClient10;
    * Enumerate all value in Lxss\DistroId registry key
    * Query CurrentControlSet\services\LxssManager\DistributionFlags
    **/
    HRESULT(__stdcall *GetDistributionConfiguration) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _Out_ LPWSTR* DistributionName,
        _Out_ PULONG Version,
        _Out_ LPWSTR* BasePath,
        _Out_ LPSTR* KernelCommandLine,
        _Out_ PULONG DefaultUid,
        _Out_ PULONG EnvironmentCount,
        _Out_ LPSTR** DefaultEnvironment,
        _Out_ PULONG Flags
        );

    /**
    * PVOID ObjectStublessClient11;
    * Query DefaultDistribution registry string
    * Delete that if BasePath doesn't exist
    **/
    HRESULT(__stdcall *GetDefaultDistribution) (
        _In_ pWslSession* wslSession,
        _Out_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient12;
    * Query State registry value should be ONE
    * Write DefaultDistribution registry with provided GUID
    **/
    HRESULT(__stdcall *SetDefaultDistribution) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId
        );

    /**
    * PVOID ObjectStublessClient13;
    * Query State registry value should be ONE
    * Returns GUID list in 16 bytes offsets
    **/
    HRESULT(__stdcall *EnumerateDistributions) (
        _In_ pWslSession* wslSession,
        _In_ ULONG State,
        _Out_ PULONG DistroCount,
        _Out_ GUID** DistroIdList
        );

    //PVOID ObjectStublessClient14;
    HRESULT(__stdcall *CreateLxProcess_s) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_ LPCSTR unknown
        );

    /**
    * PVOID ObjectStublessClient15;
    * Query Distribution Configuration and Version is TWO for newer installation
    * Write State registry to FIVE means upgrading
    **/
    HRESULT(__stdcall *BeginUpgradeDistribution) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _Out_ PULONG Version,
        _Out_ LPWSTR* BasePath
        );

    /**
    * PVOID ObjectStublessClient16;
    * Query Distribution Configuration and Write State registry to ONE means installed
    * Write Version registry to TWO for newer installation
    **/
    HRESULT(__stdcall *FinishUpgradeDistribution) (
        _In_ pWslSession* wslSession,
        _In_ GUID* DistroId,
        _In_ ULONG Version
        );

};
