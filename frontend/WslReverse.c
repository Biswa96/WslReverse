#include "WinInternal.h"
#include "LxssUserSession.h"
#include "Helpers.h"
#include "CreateLxProcess.h"
#include "LxBusServer.h"
#include "wgetopt.h"
#include <stdio.h>

int WINAPI main(void)
{
    int wargc;
    PWSTR* wargv = CommandLineToArgvW(RtlGetCommandLineW(), &wargc);

    if (wargc < 2)
    {
        LocalFree(wargv);
        wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
        return 1;
    }

    /* Declare variables */
    int c;
    HRESULT hRes = E_FAIL;
    PWSTR BasePath, DistributionName, TarFilePath;
    PSTR KernelCommandLine, *DefaultEnvironment;
    ULONG Version, DefaultUid, Flags, EnvironmentCount;
    GUID DistroId = { 0 };
    UNICODE_STRING GuidString = { 0 };

    HANDLE hTarFile = NULL;
    ILxssUserSession* wslSession = NULL;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

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
        return 1;

    /* Option table */
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
    const wchar_t* OptionString = L"b:d:e:Gg:hi:lr:S:s:t:u:x";

    /* Option parsing */
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
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
            LogResult(hRes, L"GetDistributionId");

            hRes = wslSession->lpVtbl->CreateInstance(wslSession, &DistroId, 0);
            LogResult(hRes, L"CreateInstance");
            if (hRes < 0)
                return 1;
            hRes = LxBusServer(wslSession, &DistroId);
            break;
        }
        case 'd':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
            RtlStringFromGUID(&DistroId, &GuidString);
            LogResult(hRes, L"GetDistributionId");
            wprintf(L"%ls: %ls\n", optarg, GuidString.Buffer);
            break;
        }
        case 'e':
        {
            TarFilePath = wargv[3];

            hTarFile = CreateFileW(TarFilePath,
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_DELETE,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);

            if (GetFileType(hTarFile) == FILE_TYPE_PIPE)
            {
                hTarFile = GetStdHandle(STD_OUTPUT_HANDLE);
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
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
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

        /* Install */
        case 'i':
        {
            if (wargc < 5)
            {
                wprintf(L"Try 'WslReverse.exe --help' for more information.\n");
                break;
            }

            BasePath = wargv[3], TarFilePath = wargv[4];


            if (!CreateDirectoryW(BasePath, NULL) && RtlGetLastWin32Error() != ERROR_ALREADY_EXISTS)
            {
                wprintf(L"Provided path is not valid.\n");
                break;
            }

            hTarFile = CreateFileW(TarFilePath,
                                   GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_DELETE,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

            if (hTarFile != INVALID_HANDLE_VALUE)
            {
                if (GetFileType(hTarFile) == FILE_TYPE_PIPE)
                {
                    hTarFile = GetStdHandle(STD_INPUT_HANDLE);
                    hRes = wslSession->lpVtbl->RegisterDistributionFromPipe(
                           wslSession, optarg, 0, hTarFile, BasePath, &DistroId);
                    LogResult(hRes, L"RegisterDistributionFromPipe");
                }
                else
                {
                    hRes = wslSession->lpVtbl->RegisterDistribution(
                           wslSession, optarg, 0, hTarFile, BasePath, &DistroId);
                    LogResult(hRes, L"RegisterDistribution");
                }

                if (SUCCEEDED(hRes))
                {
                    RtlStringFromGUID(&DistroId, &GuidString);
                    wprintf(L"Distribution ID: %ls\n", GuidString.Buffer);
                    wprintf(L"Install Finished.\n");
                }
            }
            else
            {
                Log(RtlGetLastWin32Error(), L"CreateFileW");
            }
            break;
        }

        /* List */
        case 'l':
        {
            ULONG DistroCount;
            PLXSS_ENUMERATE_INFO DistroInfo = NULL, tDistroInfo = NULL;
            hRes = wslSession->lpVtbl->EnumerateDistributions(wslSession, &DistroCount, &DistroInfo);

            if (DistroCount)
            {
                tDistroInfo = DistroInfo;
                wprintf(L"\nWSL Distributions:\n");
                for (ULONG i = 0; i < DistroCount; i++)
                {
                    hRes = wslSession->lpVtbl->GetDistributionConfiguration(
                           wslSession, &DistroInfo->DistributionID, &DistributionName, &Version, &BasePath,
                           &KernelCommandLine, &DefaultUid, &EnvironmentCount, &DefaultEnvironment, &Flags);

                    RtlStringFromGUID(&DistroInfo->DistributionID, &GuidString);

                    if (DistroInfo->Default)
                    {
                        wprintf(L"%ls : %ls (Default) (%ld)\n",
                                GuidString.Buffer, DistributionName, DistroInfo->Version);
                    }
                    else
                    {
                        wprintf(L"%ls : %ls (%ld)\n",
                                GuidString.Buffer, DistributionName, DistroInfo->Version);
                    }

                    RtlFreeUnicodeString(&GuidString);
                    DistroInfo = (PLXSS_ENUMERATE_INFO)((PBYTE)DistroInfo + sizeof (*DistroInfo));
                }
            }
            else
                wprintf(L"No Distribution Installed.\n");
            if (tDistroInfo)
                CoTaskMemFree(tDistroInfo);
            break;
        }
        case 'r':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
            LogResult(hRes, L"GetDistributionId");

            PSTR Arguments[] = { "-bash" };
            hRes = CreateLxProcess(wslSession, &DistroId, "/bin/bash",
                                   Arguments, ARRAY_SIZE(Arguments), L"root");
            break;
        }
        case 'S':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            hRes = wslSession->lpVtbl->SetDefaultDistribution(wslSession, &DistroId);
            LogResult(hRes, L"SetDefaultDistribution");
            break;
        }
        case 's':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            KernelCommandLine = "BOOT_IMAGE=/kernel init=/init";
            PCSTR Environment[4] = {
                "HOSTTYPE=x86_64",
                "LANG=en_US.UTF-8",
                "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games",
                "TERM=xterm-256color"
            };

            hRes = wslSession->lpVtbl->ConfigureDistribution(
                   wslSession, &DistroId, KernelCommandLine, 0,
                   ARRAY_SIZE(Environment), Environment, WSL_DISTRIBUTION_FLAGS_DEFAULT);
            LogResult(hRes, L"ConfigureDistribution");
            if (hRes == ERROR_SUCCESS)
                wprintf(L"%ls configured successfully.\n", optarg);
            break;
        }
        case 't':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            hRes = wslSession->lpVtbl->TerminateDistribution(wslSession, &DistroId);
            LogResult(hRes, L"TerminateDistribution");
            if (hRes == ERROR_SUCCESS)
                wprintf(L"%ls terminated successfully.\n", optarg);
            break;
        }
        case 'u':
        {
            hRes = wslSession->lpVtbl->GetDistributionId(wslSession, optarg, FALSE, &DistroId);
            LogResult(hRes, L"GetDistributionId");
            hRes = wslSession->lpVtbl->UnregisterDistribution(wslSession, &DistroId);
            LogResult(hRes, L"UnregisterDistribution");
            if (hRes == ERROR_SUCCESS)
                wprintf(L"%ls uninstalled successfully.\n", optarg);
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
    WSACleanup();
    LocalFree(wargv);
    return 0;
}
