#include "WinInternal.h"
#include "LxssDevice.h"
#include "LxssUserSession.h"
#include "Helpers.h"
#include "CreateLxProcess.h"
#include "LxBusServer.h"
#include "wgetopt.h"
#include <stdio.h>

int
WINAPI
main(void)
{
    int wargc;
    PWSTR* wargv = CommandLineToArgvW(RtlGetCommandLineW(), &wargc);

    if (wargc < 2)
    {
        wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
        return 0;
    }

    // Declare variables
    int c;
    HRESULT hRes = 0;
    ULONG Version, DefaultUid, Flags, EnvironmentCount;
    GUID DistroId = { 0 }, DefaultDistroId = { 0 };
    UNICODE_STRING GuidString;
    RtlZeroMemory(&GuidString, sizeof GuidString);

    HANDLE hTarFile = NULL;
    ILxssUserSession* wslSession = NULL;

    // Option table
    const wchar_t* OptionString = L"b:d:e:Gg:hi:lr:S:s:t:u:x";

    const struct option OptionTable[] = {
        { L"bus",           required_argument,   0,  'b' },
        { L"get-id",        required_argument,   0,  'd' },
        { L"export",        required_argument,   0,  'e' },
        { L"get-default",   no_argument,         0,  'G' },
        { L"get-config",    required_argument,   0,  'g' },
        { L"help",          no_argument,         0,  'h' },
        { L"install",       required_argument,   0,  'i' },
        { L"list",          no_argument,         0,  'l' },
        { L"run",           required_argument,   0,  'r' },
        { L"set-default",   required_argument,   0,  'S' },
        { L"set-config",    required_argument,   0,  's' },
        { L"terminate",     required_argument,   0,  't' },
        { L"uninstall",     required_argument,   0,  'u' },
        { L"xpert",         no_argument,         0,  'x' },
        { 0,                no_argument,         0,   0  },
    };

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hRes = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                SecurityDelegation, NULL, EOAC_STATIC_CLOAKING, NULL);
    hRes = CoCreateInstance(&CLSID_LxssUserSession,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            &IID_ILxssUserSession,
                            (PVOID*)&wslSession);

    LogResult(hRes, L"CoCreateInstance");
    if (FAILED(hRes))
        return 0;

    // Option parsing
    while ((c = wgetopt_long(wargc, wargv, OptionString, OptionTable, 0)) != -1)
    {
        switch (c)
        {
        case 0:
        {
            wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
            Usage();
            break;
        }
        case 'b':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, DistroStateAll, &DistroId);
            LogResult(hRes, L"GetDistributionId");

            hRes = wslSession->lpVtbl->CreateInstance(wslSession, &DistroId, 0);
            LogResult(hRes, L"CreateInstance");
            if (hRes < 0)
                return hRes;
            hRes = LxBusServer(wslSession, &DistroId);
            break;
        }
        case 'd':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, DistroStateAll, &DefaultDistroId);
            RtlStringFromGUID(&DefaultDistroId, &GuidString);
            LogResult(hRes, L"GetDistributionId");
            wprintf(L"%ls: %ls\n", optarg, GuidString.Buffer);
            break;
        }
        case 'e':
        {
            WCHAR TarFilePath[MAX_PATH];
            wprintf(L"Enter filename or full path of exported tar file (without quote): ");
            wscanf_s(L"%ls", TarFilePath, MAX_PATH);

            hTarFile = CreateFileW(TarFilePath,
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_DELETE,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, Installed, &DistroId);

            if (GetFileType(hTarFile) == FILE_TYPE_PIPE)
            {
                // hTarFile = GetStdHandle(STD_OUTPUT_HANDLE)
                hRes = wslSession->lpVtbl->ExportDistributionFromPipe(wslSession, &DistroId, hTarFile);
                LogResult(hRes, L"ExportDistributionFromPipe");
            }
            else
            {
                hRes = wslSession->lpVtbl->ExportDistribution(wslSession, &DistroId, hTarFile);
                LogResult(hRes, L"ExportDistribution");
            }

            break;
        }
        case 'G':
        {
            hRes = wslSession->lpVtbl->GetDefaultDistribution(wslSession, &DistroId);
            LogResult(hRes, L"GetDefaultDistribution");
            RtlStringFromGUID(&DistroId, &GuidString);
            wprintf(L"%ls\n", GuidString.Buffer);
            break;
        }
        case 'g':
        {
            PWSTR DistributionName, BasePath;
            PSTR KernelCommandLine;
            PSTR* DefaultEnvironment = NULL;

            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            LogResult(hRes, L"GetDistributionId");

            hRes = wslSession->lpVtbl->GetDistributionConfiguration(
                wslSession, &DistroId, &DistributionName, &Version, &BasePath,
                &KernelCommandLine, &DefaultUid, &EnvironmentCount, &DefaultEnvironment, &Flags);
            LogResult(hRes, L"GetDistributionConfiguration");

            wprintf(L"\n Distribution Name: %ls\n Version: %lu\n BasePath: %ls\n KernelCommandLine: %hs\n"
                    L" DefaultUID: %lu\n EnvironmentCount: %lu\n DefaultEnvironment: %hs\n Flags: %lu\n",
                    DistributionName, Version, BasePath, KernelCommandLine,
                    DefaultUid, EnvironmentCount, *DefaultEnvironment, Flags);
            break;
        }
        case 'h':
        {
            Usage();
            break;
        }
        case 'i':
        {
            wchar_t TarFilePath[MAX_PATH], BasePath[MAX_PATH];
            wprintf(L"Enter full path of .tar.gz file (without quote): ");
            wscanf_s(L"%ls", TarFilePath, MAX_PATH);

            wprintf(L"Enter folder path where to install (without quote): ");
            wscanf_s(L"%ls", BasePath, MAX_PATH);

            hRes = CoCreateGuid(&DistroId);

            hTarFile = CreateFileW(TarFilePath,
                                   GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_DELETE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

            if (GetFileType(hTarFile) == FILE_TYPE_PIPE)
            {
                // hTarFile = GetStdHandle(STD_INPUT_HANDLE)
                hRes = wslSession->lpVtbl->RegisterDistributionFromPipe(
                    wslSession, optarg, ToBeInstall, hTarFile, BasePath, &DistroId);
                LogResult(hRes, L"RegisterDistributionFromPipe");
            }
            else
            {
                hRes = wslSession->lpVtbl->RegisterDistribution(
                    wslSession, optarg, ToBeInstall, hTarFile, BasePath, &DistroId);
                LogResult(hRes, L"RegisterDistribution");
            }

            if(hRes == S_OK)
                wprintf(L"Install Finished.\n");
            break;
        }
        case 'l':
        {
            GUID* DistroIdList = NULL;
            ULONG DistroCount = 0;
            hRes = wslSession->lpVtbl->GetDefaultDistribution(wslSession, &DefaultDistroId);
            hRes = wslSession->lpVtbl->EnumerateDistributions(wslSession, DistroStateAll, &DistroCount, &DistroIdList);

            if (DistroCount)
            {
                ULONG i = 0;
                PWSTR DistributionName, BasePath;
                PSTR KernelCommandLine;
                PSTR* DefaultEnvironment = NULL;

                wprintf(L"\nWSL Distributions:\n");
                do
                {
                    DistroId = DistroIdList[i];
                    if (DistroId.Data1 == DefaultDistroId.Data1 &&
                        (DistroId.Data4 - DefaultDistroId.Data4))
                    {
                        hRes = wslSession->lpVtbl->GetDistributionConfiguration(
                            wslSession, &DistroId, &DistributionName, &Version, &BasePath,
                            &KernelCommandLine, &DefaultUid, &EnvironmentCount, &DefaultEnvironment, &Flags);

                        RtlStringFromGUID(&DistroId, &GuidString);
                        wprintf(L"%ls : %ls (Default)\n", GuidString.Buffer, DistributionName);
                    }
                    else
                    {
                        hRes = wslSession->lpVtbl->GetDistributionConfiguration(
                            wslSession, &DistroId, &DistributionName, &Version, &BasePath,
                            &KernelCommandLine, &DefaultUid, &EnvironmentCount, &DefaultEnvironment, &Flags);

                        RtlStringFromGUID(&DistroId, &GuidString);
                        wprintf(L"%ls : %ls\n", GuidString.Buffer, DistributionName);
                    }
                    ++i;
                } while (i < DistroCount);
            }
            else
            {
                wprintf(L"No Distribution installed.\n");
            }
            break;
        }
        case 'r':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            LogResult(hRes, L"GetDistributionId");

            PSTR Arguments[] = { "-bash" };
            hRes = CreateLxProcess(wslSession, &DistroId, "/bin/bash",
                                   Arguments, ARRAY_SIZE(Arguments), L"root");
            break;
        }
        case 'S':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            hRes = wslSession->lpVtbl->SetDefaultDistribution(wslSession, &DistroId);
            LogResult(hRes, L"SetDefaultDistribution");
            break;
        }
        case 's':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            PCSTR KernelCommandLine = "BOOT_IMAGE=/kernel init=/init ro";
            PCSTR DefaultEnvironment[] = {
                "HOSTTYPE=x86_64",
                "LANG=en_US.UTF-8",
                "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games",
                "TERM=xterm-256color"
            };

            hRes = wslSession->lpVtbl->ConfigureDistribution(
                wslSession, &DistroId, KernelCommandLine, 0,
                ARRAY_SIZE(DefaultEnvironment), DefaultEnvironment, WSL_DISTRIBUTION_FLAGS_DEFAULT);
            LogResult(hRes, L"ConfigureDistribution");
            break;
        }
        case 't':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            hRes = wslSession->lpVtbl->TerminateDistribution(wslSession, &DistroId);
            LogResult(hRes, L"TerminateDistribution");
            if (hRes == ERROR_SUCCESS)
                wprintf(L"Distribution terminated successfully.\n");
            break;
        }
        case 'u':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, Installed, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            hRes = wslSession->lpVtbl->UnregisterDistribution(wslSession, &DistroId);
            LogResult(hRes, L"UnregisterDistribution");
            break;
        }
        case 'x':
        {
            LxssDevice();
            break;
        }
        default:
            wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
        }
    }

    // Cleanup
    if(GuidString.Buffer)
        RtlFreeUnicodeString(&GuidString);
    if (hTarFile)
        NtClose(hTarFile);
    hRes = wslSession->lpVtbl->Release(wslSession);
    CoUninitialize();
    return 0;
}
