#include "../src/WinInternal.h"
#include "../src/WslSession.h"
#include "../src/CreateLxProcess.h"
#include "../src/GetConhostServerId.h"
#include <stdio.h>

void
WINAPI
BackendUsage(void)
{
    wprintf(L"\n Required Command Line Format:\n"
            L"WslReverseHost.exe [CurrentDistroID] [ServerHandle] [EventHandle] [ProcessHandle] [LxInstanceId]\n");
}

int
WINAPI
main(void)
{
    int wargc;
    PWSTR* wargv = CommandLineToArgvW(RtlGetCommandLineW(), &wargc);

    if (wargc < 5)
    {
        BackendUsage();
        return 0;
    }

    HRESULT hRes;
    PWslSession* wslSession = NULL;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hRes = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                SecurityDelegation, NULL, EOAC_STATIC_CLOAKING, NULL);
    hRes = CoCreateInstance(&CLSID_LxssUserSession,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            &IID_ILxssUserSession,
                            (PVOID*)&wslSession);
    if (!wslSession)
        return 0;

    NTSTATUS Status;
    UNICODE_STRING DistroIdString;
    GUID DistroId;

    // Get LxssInstance only if it exist i.e. distribution is running
    Status = RtlInitUnicodeStringEx(&DistroIdString, wargv[1]);
    Status = RtlGUIDFromString(&DistroIdString, &DistroId);
    hRes = (*wslSession)->CreateInstance(wslSession, &DistroId, 3);
    if (hRes != 0)
        return 0;

    HANDLE ServerHandle = NULL, EventHandle = NULL, ProcessHandle = NULL;

    ServerHandle = ToHandle(wcstoul(wargv[2], NULL, 0));
    EventHandle = ToHandle(wcstoul(wargv[3], NULL, 0));
    ProcessHandle = ToHandle(wcstoul(wargv[4], NULL, 0));

    // Log info
    wprintf(L"[+] WslReverseHost: \n\t DistroId: %ls\n\t"
            L" ServerHandle: 0x%p \n\t EventHandle: 0x%p \n\t ProcessHandle: 0x%p\n",
            DistroIdString.Buffer,
            ServerHandle, EventHandle, ProcessHandle);

    HANDLE ConsoleHandle = GetUserProcessParameter()->ConsoleHandle;
    wprintf(L"[+] ConHost: \n\t"
            L" ConhostServerId: %llu \n\t ConsoleHandle: 0x%p\n",
            GetConhostServerId(ConsoleHandle), ConsoleHandle);

    Status = NtSetEvent(EventHandle, NULL);
    Status = NtWaitForSingleObject(ProcessHandle, FALSE, NULL);
    CreateProcessWorker(NULL, ServerHandle, NULL);

    // Cleanup
    if (ProcessHandle)
        NtClose(ProcessHandle);
    if (EventHandle)
        NtClose(EventHandle);
    if (ServerHandle)
        NtClose(ServerHandle);
    hRes = (*wslSession)->Release(wslSession);
    CoUninitialize();
}
