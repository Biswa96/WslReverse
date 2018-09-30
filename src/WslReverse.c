#define _CRT_SECURE_NO_WARNINGS
#include "Functions.h"
#include "WslSession.h"
#include "CreateLxProcess.h"
#include "wgetopt.h"
#include <stdio.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

int main() {

    int wargc;
    wchar_t** wargv;
    wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);

    if (wargc < 2) {
        wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
        exit(EXIT_FAILURE);
    }

    /*Declare variables*/
    int c;
    HRESULT result = 0;
    GUID DistroId = { 0 };
    pWslSession* wslSession;
    pWslInstance* wslInstance;

    /*Option table*/
    const struct option OptionTable[] = {
        { L"get-id",        required_argument,   0,  'd' },
        { L"get-default",   no_argument,         0,  'G' },
        { L"get-config",    required_argument,   0,  'g' },
        { L"help",          no_argument,         0,  'h' },
        { L"install",       required_argument,   0,  'i' },
        { L"run",           required_argument,   0,  'r' },
        { L"set-default",   required_argument,   0,  'S' },
        { L"set-config",    required_argument,   0,  's' },
        { L"terminate",     required_argument,   0,  't' },
        { L"uninstall",     required_argument,   0,  'u' },
        { 0,                no_argument,         0,   0  },
    };

    CoInitializeEx(0, COINIT_MULTITHREADED);
    CoInitializeSecurity(0, -1, 0, 0, RPC_C_AUTHN_LEVEL_DEFAULT, SecurityDelegation, 0, EOAC_STATIC_CLOAKING, 0);
    result = CoCreateInstance(&CLSID_LxssUserSession, 0, CLSCTX_LOCAL_SERVER, &IID_ILxssUserSession, (PVOID*)&wslSession);
    Log(result, L"CoCreateInstance");

    while ((c = wgetopt_long(wargc, wargv, L"d:Gg:hi:r:S:s:t:u:", OptionTable, 0)) != -1) {

        switch (c) {

        case 0: {
            wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
            Usage();
            break;
        }

        case 'd': {
            result = (*wslSession)->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            Log(result, L"GetDistributionId");
            PrintGuid(&DistroId);
            break;
        }

        case 'G': {
            result = (*wslSession)->GetDefaultDistribution(wslSession, &DistroId);
            Log(result, L"GetDefaultDistribution");
            PrintGuid(&DistroId);
            break;
        }

        case 'g': {
            ULONG Version, DefaultUid, Flags, EnvironmentCount;
            PWSTR DistributionName, BasePath;
            LPSTR KernelCommandLine;
            LPSTR* DefaultEnvironment = 0;

            result = (*wslSession)->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            Log(result, L"GetDistributionId");
            result = (*wslSession)->GetDistributionConfiguration(
                wslSession, &DistroId, &DistributionName, &Version, &BasePath,
                &KernelCommandLine, &DefaultUid, &EnvironmentCount, &DefaultEnvironment, &Flags
            );
            Log(result, L"GetDistributionConfiguration");
            printf(
                " Distribution Name: %ls\n Version: %lu\n BasePath: %ls\n KernelCommandLine: %s\n"
                " DefaultUID: %lu\n EnvironmentCount: %lu\n DefaultEnvironment: %s\n Flags: %lu\n"
                , DistributionName, Version, BasePath, KernelCommandLine,
                DefaultUid, EnvironmentCount, *DefaultEnvironment, Flags
            );
            break;
        }

        case 'h': {
            Usage();
            break;
        }

        case 'i': {
            wchar_t TarFile[MAX_PATH], BasePath[MAX_PATH];
            wprintf(L"Enter full path of .tar.gz file (without quote): ");
            wscanf(L"%ls", TarFile);

            HANDLE hTarFile = CreateFileW(
                TarFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            wprintf(L"Enter folder path where to install (without quote): ");
            wscanf(L"%ls", BasePath);

            CoCreateGuid(&DistroId);
            result = (*wslSession)->RegisterDistribution(
                wslSession, optarg, ToBeInstall, hTarFile, BasePath, &DistroId);
            Log(result, L"RegisterDistribution");
            wprintf(L"Installing...\n");
            break;
        }

        case 'r': {
            result = (*wslSession)->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            Log(result, L"GetDistributionId");
            result = (*wslSession)->CreateInstance(wslSession, &DistroId, TRUE, &IID_ILxssInstance, (PVOID*)&wslInstance);
            Log(result, L"CreateInstance");
            result = CreateLxProcess(wslInstance);
            Log(result, L"CreateLxProcess");
            break;
        }

        case 'S': {
            result = (*wslSession)->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            Log(result, L"GetDistributionId");
            result = (*wslSession)->SetDefaultDistribution(wslSession, &DistroId);
            Log(result, L"SetDefaultDistribution");
            break;
        }

        case 's': {
            result = (*wslSession)->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            Log(result, L"GetDistributionId");
            LPCSTR KernelCommandLine = "BOOT_IMAGE=/kernel init=/init ro";
            LPCSTR DefaultEnvironment[4] = { 
                "HOSTTYPE=x86_64"
                "LANG=en_US.UTF-8"
                "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
                "TERM=xterm-256color"
            };
            result = (*wslSession)->ConfigureDistribution(wslSession, &DistroId, KernelCommandLine,
                RootUser, ARRAY_SIZE(DefaultEnvironment), DefaultEnvironment, WSL_DISTRIBUTION_FLAGS_DEFAULT);
            Log(result, L"ConfigureDistribution");
            break;
        }

        case 't': {
            result = (*wslSession)->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            Log(result, L"GetDistributionId");
            result = (*wslSession)->TerminateDistribution(wslSession, &DistroId);
            Log(result, L"TerminateDistribution");
            break;
        }

        case 'u': {
            result = (*wslSession)->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            Log(result, L"GetDistributionId");
            result = (*wslSession)->UnregisterDistribution(wslSession, &DistroId);
            Log(result, L"UnregisterDistribution");
            break;
        }

        default:
            wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
        }

    }

    return 0;
}
