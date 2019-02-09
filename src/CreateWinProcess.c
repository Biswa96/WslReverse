#include "WinInternal.h"
#include "LxBus.h"
#include "Log.h"
#include <stdio.h>

typedef struct _X_HPCON {
    HANDLE hWritePipe;
    HANDLE hConDrvReference;
    HANDLE hConHostProcess;
} X_HPCON, *PX_HPCON;

BOOL
CreateWinProcess(PLXSS_MESSAGE_PORT_RECEIVE_OBJECT LxReceiveMsg,
                 PLX_CREATE_PROCESS_RESULT ProcResult)
{
    BOOL bRes;
    NTSTATUS Status;
    SIZE_T AttrSize = 0;
    STARTUPINFOEXW SInfoEx = { 0 };
    PROCESS_BASIC_INFORMATION BasicInfo = { 0 };
    HANDLE HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;
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

        // Cast hpCon to a internal structure for ConHost PID
        Status = ZwQueryInformationProcess(((PX_HPCON)hpCon)->hConHostProcess,
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
            LogStatus(Status, L"ZwQueryInformationProcess");

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

    WCHAR w_ApplicationName[MAX_PATH], w_CurrentDirectory[MAX_PATH];
    PSTR ApplicationName = LxReceiveMsg->Unknown + LxReceiveMsg->WinApplicationPathOffset;
    PSTR CurrentDirectory = LxReceiveMsg->Unknown + LxReceiveMsg->WinCurrentPathOffset;

    mbstowcs_s(NULL, w_ApplicationName, MAX_PATH, ApplicationName, MAX_PATH);
    mbstowcs_s(NULL, w_CurrentDirectory, MAX_PATH, CurrentDirectory, MAX_PATH);

    bRes = CreateProcessW(w_ApplicationName,
                          NULL,
                          NULL,
                          NULL,
                          TRUE,
                          EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
                          NULL,
                          w_CurrentDirectory,
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
        LogResult(GetLastError(), L"CreateWinProcess");

    Status = ZwQueryInformationProcess(ProcResult->ProcInfo.hProcess,
                                       ProcessBasicInformation,
                                       &BasicInfo,
                                       sizeof BasicInfo,
                                       NULL);
    LogStatus(Status, L"ZwQueryInformationProcess");

    if (NT_SUCCESS(Status))
    {
        PEB Peb;
        SIZE_T NumberOfBytesRead = 0;

        RtlZeroMemory(&Peb, sizeof Peb);
        Status = ZwReadVirtualMemory(ProcResult->ProcInfo.hProcess,
                                     BasicInfo.PebBaseAddress,
                                     &Peb,
                                     sizeof Peb,
                                     &NumberOfBytesRead);

        if (NT_SUCCESS(Status))
        {
            wprintf(L"[+] ZwReadVirtualMemory \n\t"
                    L" NumberOfBytesRead: %zu \n\t OSMajorVersion: %lu\n",
                    NumberOfBytesRead, Peb.OSMajorVersion);
        }
        else
            LogStatus(Status, L"ZwReadVirtualMemory");

        // From IMAGE_OPTIONAL_HEADER structure
        if (Peb.ImageSubsystemMinorVersion == IMAGE_SUBSYSTEM_WINDOWS_GUI)
            ProcResult->IsSubsystemGUI |= INTEROP_RESTORE_CONSOLE_STATE_MODE;
    }

    // Set lasterror always success intentionally ;)
    ProcResult->LastError = ERROR_SUCCESS;

    RtlFreeHeap(HeapHandle, 0, AttrList);
    return bRes;
}
