#include "WinInternal.h"
#include "LxBus.h"
#include "Helpers.h"
#include <stdio.h>

typedef struct _X_HPCON {
    HANDLE hWritePipe;
    HANDLE hConDrvReference;
    HANDLE hConHostProcess;
} X_HPCON, *PX_HPCON;

BOOL
WINAPI
CreateWinProcess(PLXSS_MESSAGE_PORT_RECEIVE_OBJECT LxReceiveMsg,
                 PLX_CREATE_PROCESS_RESULT ProcResult)
{
    BOOL bRes;
    NTSTATUS Status;
    SIZE_T AttrSize = 0;
    STARTUPINFOEXW SInfoEx;
    RtlZeroMemory(&SInfoEx, sizeof SInfoEx);

    PROCESS_BASIC_INFORMATION BasicInfo;
    HANDLE HeapHandle = RtlGetProcessHeap();
    LPPROC_THREAD_ATTRIBUTE_LIST AttrList = NULL;

    bRes = InitializeProcThreadAttributeList(NULL, 1, 0, &AttrSize);
    AttrList = RtlAllocateHeap(HeapHandle, HEAP_ZERO_MEMORY, AttrSize);
    bRes = InitializeProcThreadAttributeList(AttrList, 1, 0, &AttrSize);

    if (LxReceiveMsg->IsWithoutPipe)
    {
        HRESULT hRes;
        COORD ConsoleSize;
        HPCON hpCon = NULL;

        ConsoleSize.X = LxReceiveMsg->WindowWidth;
        ConsoleSize.Y = LxReceiveMsg->WindowHeight;

        hRes = CreatePseudoConsole(ConsoleSize,
                                   ToHandle(LxReceiveMsg->VfsMsg[0].Handle),
                                   ToHandle(LxReceiveMsg->VfsMsg[1].Handle),
                                   PSEUDOCONSOLE_INHERIT_CURSOR,
                                   &hpCon);

        // Return hpCon to caller
        ProcResult->hpCon = hpCon;
        LogResult(hRes, L"CreatePseudoConsole");

        // Cast hpCon to an internal structure for ConHost PID
        RtlZeroMemory(&BasicInfo, sizeof BasicInfo);
        Status = NtQueryInformationProcess(((PX_HPCON)hpCon)->hConHostProcess,
                                           ProcessBasicInformation,
                                           &BasicInfo,
                                           sizeof BasicInfo,
                                           NULL);

        if (NT_SUCCESS(Status))
        {
            wprintf(L"[+] CreatePseudoConsole: \n\t"
                    L" ConHostProcessId: %zu \n\t ConHostProcessHandle: 0x%p\n",
                    BasicInfo.UniqueProcessId, ((PX_HPCON)hpCon)->hConHostProcess);
        }
        else
            LogStatus(Status, L"NtQueryInformationProcess");

        if(AttrList)
            bRes = UpdateProcThreadAttribute(AttrList,
                                             0,
                                             PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, //0x20016u
                                             hpCon,
                                             sizeof hpCon,
                                             NULL,
                                             NULL);
    }
    else
    {
        SInfoEx.StartupInfo.hStdInput = ToHandle(LxReceiveMsg->VfsMsg[0].Handle);
        SInfoEx.StartupInfo.hStdOutput = ToHandle(LxReceiveMsg->VfsMsg[1].Handle);
        SInfoEx.StartupInfo.hStdError = ToHandle(LxReceiveMsg->VfsMsg[2].Handle);

        HANDLE Value[3] = { NULL };
        Value[0] = ToHandle(LxReceiveMsg->VfsMsg[0].Handle);
        Value[1] = ToHandle(LxReceiveMsg->VfsMsg[1].Handle);
        Value[2] = ToHandle(LxReceiveMsg->VfsMsg[2].Handle);

        if(AttrList)
            bRes = UpdateProcThreadAttribute(AttrList,
                                             0,
                                             PROC_THREAD_ATTRIBUTE_HANDLE_LIST, //0x20002u
                                             Value,
                                             sizeof Value,
                                             NULL,
                                             NULL);
    }

    SInfoEx.StartupInfo.cb = sizeof SInfoEx;
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    SInfoEx.lpAttributeList = AttrList;

    UNICODE_STRING u_ApplicationName, u_CurrentDirectory;
    PSTR ApplicationName = LxReceiveMsg->Unknown + LxReceiveMsg->WinApplicationPathOffset;
    PSTR CurrentDirectory = LxReceiveMsg->Unknown + LxReceiveMsg->WinCurrentPathOffset;

    Status = MbsToWcs(ApplicationName, &u_ApplicationName);

    // Check if current path exist in that offset
    if (CurrentDirectory[0])
        Status = MbsToWcs(CurrentDirectory, &u_CurrentDirectory);
    else
        u_CurrentDirectory.Buffer = NULL;

    bRes = CreateProcessW(u_ApplicationName.Buffer,
                          NULL,
                          NULL,
                          NULL,
                          TRUE,
                          EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
                          NULL,
                          u_CurrentDirectory.Buffer,
                          &SInfoEx.StartupInfo,
                          (LPPROCESS_INFORMATION)&ProcResult->ProcInfo);

    if (bRes)
    {
        wprintf(L"[+] CreateWinProcess:\n\t ProcessId: %u \n\t"
                L" ProcessHandle: 0x%p \n\t ThreadHandle: 0x%p\n",
                ProcResult->ProcInfo.dwProcessId,
                ProcResult->ProcInfo.hProcess, ProcResult->ProcInfo.hThread);
    }
    else
        LogResult(RtlGetLastWin32Error(), L"CreateWinProcess");

    Status = NtQueryInformationProcess(ProcResult->ProcInfo.hProcess,
                                       ProcessBasicInformation,
                                       &BasicInfo,
                                       sizeof BasicInfo,
                                       NULL);
    LogStatus(Status, L"NtQueryInformationProcess");

    if (NT_SUCCESS(Status))
    {
        PEB Peb;
        SIZE_T NumberOfBytesRead = 0;

        RtlZeroMemory(&Peb, sizeof Peb);
        Status = NtReadVirtualMemory(ProcResult->ProcInfo.hProcess,
                                     BasicInfo.PebBaseAddress,
                                     &Peb,
                                     sizeof Peb,
                                     &NumberOfBytesRead);

        if (NT_SUCCESS(Status))
        {
            wprintf(L"[+] NtReadVirtualMemory \n\t"
                    L" NumberOfBytesRead: %zu \n\t OSMajorVersion: %lu\n",
                    NumberOfBytesRead, Peb.OSMajorVersion);
        }
        else
            LogStatus(Status, L"NtReadVirtualMemory");

        // From IMAGE_OPTIONAL_HEADER structure
        if (Peb.ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI)
            ProcResult->IsSubsystemGUI |= INTEROP_RESTORE_CONSOLE_STATE_MODE;
    }

    // Set lasterror always success intentionally ;)
    ProcResult->LastError = ERROR_SUCCESS;

    // Cleanup
    if (u_ApplicationName.Buffer)
        RtlFreeUnicodeString(&u_ApplicationName);
    if (u_CurrentDirectory.Buffer)
        RtlFreeUnicodeString(&u_CurrentDirectory);
    if (AttrList)
        RtlFreeHeap(HeapHandle, 0, AttrList);
    return bRes;
}
