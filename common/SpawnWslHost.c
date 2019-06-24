#include "WinInternal.h"
#include "Helpers.h"
#include <stdio.h>

BOOL
WINAPI
SpawnWslHost(HANDLE ServerHandle,
             GUID* InitiatedDistroID,
             GUID* LxInstanceID)
{
    BOOL bRes;
    NTSTATUS Status;
    HANDLE EventHandle, ProcHandle, hProc = NtCurrentProcess();
    HANDLE HeapHandle = RtlGetProcessHeap();

    bRes = SetHandleInformation(ServerHandle,
                                HANDLE_FLAG_INHERIT,
                                HANDLE_FLAG_INHERIT);

    /* Create an event to synchronize with wslhost.exe process */
    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    LogStatus(Status, L"NtCreateEvent");

    bRes = SetHandleInformation(EventHandle,
                                HANDLE_FLAG_INHERIT,
                                HANDLE_FLAG_INHERIT);

    Status = NtDuplicateObject(hProc, hProc,
                               hProc, &ProcHandle,
                               0, OBJ_INHERIT, DUPLICATE_SAME_ACCESS);

    /* Configure all three handles for wslhost.exe process */
    size_t AttrbSize;
    LPPROC_THREAD_ATTRIBUTE_LIST AttrbList = NULL;
    bRes = InitializeProcThreadAttributeList(NULL, 1, 0, &AttrbSize);
    AttrbList = RtlAllocateHeap(HeapHandle, 0, AttrbSize);
    bRes = InitializeProcThreadAttributeList(AttrbList, 1, 0, &AttrbSize);

    HANDLE Value[3];
    Value[0] = ServerHandle;
    Value[1] = EventHandle;
    Value[2] = ProcHandle;

    bRes = UpdateProcThreadAttribute(AttrbList, 0,
                                     PROC_THREAD_ATTRIBUTE_HANDLE_LIST, /* 0x20002u */
                                     Value, sizeof Value, NULL, NULL);

    /**
    * Create required commandline for WslHost process
    * Format: WslHost.exe [CurrentDistroID] [ServerHandle] [EventHandle] [ProcessHandle] [LxInstanceId]
    **/
    wchar_t ProgramName[MAX_PATH], CommandLine[MAX_PATH];
    ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\lxss\\wslhost.exe", ProgramName, MAX_PATH);

    UNICODE_STRING InitiatedDistroIDstring, LxInstanceIDstring;
    RtlStringFromGUID(InitiatedDistroID, &InitiatedDistroIDstring);

    /* Testing: Replace WslHost.exe with custom WslReverseHost.exe */
#ifdef _DEBUG
    RtlZeroMemory(ProgramName, sizeof ProgramName);
    wcscpy_s(ProgramName, MAX_PATH, L"WslReverseHost.exe");
#endif

    _snwprintf_s(CommandLine,
                 MAX_PATH,
                 MAX_PATH,
                 L"%ls %ls %ld %ld %ld ",
                 ProgramName,
                 InitiatedDistroIDstring.Buffer,
                 ToULong(ServerHandle),
                 ToULong(EventHandle),
                 ToULong(ProcHandle));

    RtlFreeUnicodeString(&InitiatedDistroIDstring);

    /* Check if this invoked from WSL1 or WSL2 */
    if (LxInstanceID)
    {
        RtlStringFromGUID(LxInstanceID, &LxInstanceIDstring);
        wcsncat_s(CommandLine, MAX_PATH, LxInstanceIDstring.Buffer, MAX_PATH);
        RtlFreeUnicodeString(&LxInstanceIDstring);
    }

    PROCESS_INFORMATION ProcInfo;
    STARTUPINFOEXW SInfoEx;
    RtlZeroMemory(&SInfoEx, sizeof SInfoEx);

    SInfoEx.StartupInfo.cb = sizeof SInfoEx;
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    SInfoEx.StartupInfo.wShowWindow = SW_SHOWMINNOACTIVE;
    SInfoEx.lpAttributeList = AttrbList;

    /* bInheritHandles must be TRUE for wslhost.exe */
    bRes = CreateProcessW(NULL,
                          CommandLine,
                          NULL,
                          NULL,
                          TRUE,
                          EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE,
                          NULL,
                          NULL,
                          &SInfoEx.StartupInfo,
                          &ProcInfo);

    if (bRes)
    {
        wprintf(L"[+] BackendHost: \n\t CommandLine: %ls\n\t"
                L" ProcessId: %lu \n\t ProcessHandle: 0x%p \n\t ThreadHandle: 0x%p\n",
                CommandLine,
                ProcInfo.dwProcessId, ProcInfo.hProcess, ProcInfo.hThread);

        HANDLE Handles[2];
        Handles[0] = ProcInfo.hProcess;
        Handles[1] = EventHandle;

        Status = NtWaitForMultipleObjects(ARRAY_SIZE(Handles), Handles,
                                          WaitAny, FALSE, NULL);
    }
    else
        Log(RtlGetLastWin32Error(), L"CreateProcessW");

    /* Cleanup */
    RtlFreeHeap(HeapHandle, 0, AttrbList);
    NtClose(ProcInfo.hProcess);
    NtClose(ProcInfo.hThread);
    NtClose(EventHandle);
    NtClose(ProcHandle);
    return bRes;
}
