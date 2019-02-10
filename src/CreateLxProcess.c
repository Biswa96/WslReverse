#include "GetConhostServerId.h"
#include "CreateProcessAsync.h"
#include "Log.h"
#include "WslSession.h"
#include "WinInternal.h" // PEB and some defined expression
#include "LxBus.h" // For IOCTLs values
#include <stdio.h>

#define DISABLED_INPUT_MODE (ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)
#define ENABLE_INPUT_MODE (ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT)
#define ENABLE_OUTPUT_MODE (DISABLE_NEWLINE_AUTO_RETURN | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT)

typedef struct _SvcCommIo {
    HANDLE hIn;
    HANDLE hOut;
    HANDLE hErr;
    ULONG InMode;
    ULONG OutMode;
    ULONG ErrMode;
} SvcCommIo, *PSvcCommIo;

void
ConfigureStdHandles(PLXSS_STD_HANDLES StdHandles,
                    PSvcCommIo CommIo)
{
    // Set pipe handles if standard I/O handles are redirected
    StdHandles->StdIn.Handle = ToULong(CommIo->hIn);
    StdHandles->StdIn.Pipe = LxInputPipeType;
    StdHandles->StdOut.Handle = ToULong(CommIo->hOut);
    StdHandles->StdOut.Pipe = LxOutputPipeType;
    StdHandles->StdErr.Handle = ToULong(CommIo->hOut);
    StdHandles->StdErr.Pipe = LxOutputPipeType;

    if (GetFileType(CommIo->hIn) == FILE_TYPE_CHAR &&
        GetConsoleMode(CommIo->hIn, &CommIo->InMode))
    {
        // Switch to VT-100 Input Console
        ULONG NewMode = (CommIo->InMode & (~DISABLED_INPUT_MODE)) | ENABLE_INPUT_MODE; // & 0xFFFFFFD8 | 0x208
        SetConsoleMode(CommIo->hIn, NewMode);

        // Switch input to UTF-8
        SetConsoleCP(CP_UTF8);

        StdHandles->StdIn.Handle = 0;
        StdHandles->StdIn.Pipe = 0;
    }

    if (GetFileType(CommIo->hOut) == FILE_TYPE_CHAR &&
        GetConsoleMode(CommIo->hOut, &CommIo->OutMode))
    {
        // Switch to VT-100 Output Console
        ULONG NewMode = (CommIo->OutMode | ENABLE_OUTPUT_MODE); // 0xD
        SetConsoleMode(CommIo->hOut, NewMode);

        // Switch output to UTF-8
        SetConsoleOutputCP(CP_UTF8);

        StdHandles->StdOut.Handle = 0;
        StdHandles->StdOut.Pipe = 0;
    }

    if (GetFileType(CommIo->hErr) == FILE_TYPE_CHAR &&
        GetConsoleMode(CommIo->hErr, &CommIo->ErrMode))
    {
        // Switch to VT-100 Output Console
        ULONG NewMode = (CommIo->ErrMode | ENABLE_OUTPUT_MODE); // 0xD
        SetConsoleMode(CommIo->hErr, NewMode);

        StdHandles->StdErr.Handle = 0;
        StdHandles->StdErr.Pipe = 0;
    }
}

void
CreateProcessWorker(PTP_CALLBACK_INSTANCE Instance,
                    HANDLE ServerHandle,
                    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PTP_WORK WorkReturn = NULL;
    LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG WaitMsg = { 0 };

    // Infinite loop to wait for client message
    while (TRUE)
    {
        WaitMsg.Timeout = INFINITE;

        Status = ZwDeviceIoControlFile(ServerHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION,
                                       &WaitMsg, sizeof WaitMsg,
                                       &WaitMsg, sizeof WaitMsg);

        if (NT_SUCCESS(Status))
            wprintf(L"[+] ClientHandle: %u\n", WaitMsg.ClientHandle);
        else
            break;

        Status = TpAllocWork(&WorkReturn,
                             (PTP_WORK_CALLBACK)CreateProcessAsync,
                             ToHandle(WaitMsg.ClientHandle),
                             NULL);
        LogStatus(Status, L"CreateProcessAsync TpAllocWork");

        WaitMsg.ClientHandle = 0;
        TpPostWork(WorkReturn);
        TpReleaseWork(WorkReturn);
    }

    ZwClose(ServerHandle);
}

BOOL
InitializeInterop(HANDLE ServerHandle,
                  GUID* CurrentDistroID,
                  GUID* LxInstanceID)
{
    UNREFERENCED_PARAMETER(LxInstanceID); // For future use in Hyper-V VM Mode

    BOOL bRes;
    NTSTATUS Status;
    wchar_t WslHost[MAX_PATH], CommandLine[MAX_PATH];
    HANDLE EventHandle, ProcHandle, ServerHandleDup;
    HANDLE HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    // Create an event to synchronize with wslhost.exe process
    Status = ZwCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    LogStatus(Status, L"ZwCreateEvent");

    bRes = SetHandleInformation(EventHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    bRes = SetHandleInformation(ServerHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    Status = ZwDuplicateObject(ZwCurrentProcess(),
                               ZwCurrentProcess(),
                               ZwCurrentProcess(),
                               &ProcHandle,
                               0,
                               OBJ_INHERIT,
                               DUPLICATE_SAME_ACCESS);
    Status = ZwDuplicateObject(ZwCurrentProcess(),
                               ServerHandle,
                               ZwCurrentProcess(),
                               &ServerHandleDup,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);

    // Connect to LxssMessagePort for Windows interopt
    PTP_WORK WorkReturn = NULL;
    Status = TpAllocWork(&WorkReturn,
                         (PTP_WORK_CALLBACK)CreateProcessWorker,
                         ServerHandleDup,
                         NULL);

    LogStatus(Status, L"CreateProcessWorker TpAllocWork");
    TpPostWork(WorkReturn);

    // Configure all the required handles for wslhost.exe process
    size_t AttrbSize = 0;
    LPPROC_THREAD_ATTRIBUTE_LIST AttrbList = NULL;
    InitializeProcThreadAttributeList(NULL, 1, 0, &AttrbSize);
    AttrbList = RtlAllocateHeap(HeapHandle, HEAP_ZERO_MEMORY, AttrbSize);
    bRes = InitializeProcThreadAttributeList(AttrbList, 1, 0, &AttrbSize);

    if (AttrbList)
    {
        HANDLE Value[3] = { NULL };
        Value[0] = ServerHandle;
        Value[1] = EventHandle;
        Value[2] = ProcHandle;

        bRes = UpdateProcThreadAttribute(AttrbList,
                                         0,
                                         PROC_THREAD_ATTRIBUTE_HANDLE_LIST, //0x20002u
                                         Value,
                                         sizeof Value,
                                         NULL,
                                         NULL);
    }

    // Create required string for CreateProcessW
    ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\lxss\\wslhost.exe", WslHost, MAX_PATH);

    _snwprintf_s(
        CommandLine,
        MAX_PATH,
        MAX_PATH,
        L"%ls {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} %ld %ld %ld",
        WslHost,
        CurrentDistroID->Data1, CurrentDistroID->Data2, CurrentDistroID->Data3,
        CurrentDistroID->Data4[0], CurrentDistroID->Data4[1], CurrentDistroID->Data4[2],
        CurrentDistroID->Data4[3], CurrentDistroID->Data4[4], CurrentDistroID->Data4[5],
        CurrentDistroID->Data4[6], CurrentDistroID->Data4[7],
        ToULong(ServerHandle),
        ToULong(EventHandle),
        ToULong(ProcHandle));

    PROCESS_INFORMATION ProcInfo;
    STARTUPINFOEXW SInfoEx = { 0 };
    SInfoEx.StartupInfo.cb = sizeof SInfoEx;
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    // SInfoEx.StartupInfo.wShowWindow = SW_SHOWDEFAULT;
    SInfoEx.lpAttributeList = AttrbList;

    // bInheritHandles must be TRUE for wslhost.exe
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
        wprintf(L"[+] WslHost: \n\t CommandLine: %ls\n\t"
                L" ProcessId: %lu \n\t ProcessHandle: 0x%p \n\t ThreadHandle: 0x%p\n",
                CommandLine,
                ProcInfo.dwProcessId, ProcInfo.hProcess, ProcInfo.hThread);

        HANDLE Handles[2] = { NULL };
        Handles[0] = ProcInfo.hProcess;
        Handles[1] = EventHandle;

        Status = ZwWaitForMultipleObjects(ARRAY_SIZE(Handles),
                                          Handles,
                                          WaitAny,
                                          FALSE,
                                          NULL);
    }
    else
        LogResult(GetLastError(), L"CreteProcess");

    // Cleanup
    RtlFreeHeap(HeapHandle, 0, AttrbList);
    ZwClose(EventHandle);
    // ZwClose(hServer) causes STATUS_INVALID_HANDLE in CreateProcessWorker
    ZwClose(ProcHandle);
    ZwClose(ProcInfo.hProcess);
    ZwClose(ProcInfo.hThread);
    return bRes;
}

HRESULT
CreateLxProcess(PWslSession* wslSession,
                GUID* DistroID,
                char* CommandLine,
                char** Arguments,
                int ArgumentCount,
                wchar_t* LxssUserName)
{
    HRESULT hRes;
    HANDLE ProcessHandle = NULL, ServerHandle = NULL;
    GUID InitiatedDistroID, LxInstanceID;
    PVOID socket = NULL;
    HANDLE HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;

    // Console Window handle of current process (if any)
    PRTL_USER_PROCESS_PARAMETERS ProcParam = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters;
    HANDLE ConsoleHandle = ProcParam->ConsoleHandle;
    wprintf(L"[+] ConHost: \n\t"
            L" ConhostServerId: %lld \n\t ConsoleHandle: 0x%p\n",
            GetConhostServerId(ConsoleHandle), ConsoleHandle);

    // Configure standard handles
    LXSS_STD_HANDLES StdHandles;
    RtlZeroMemory(&StdHandles, sizeof StdHandles);

    SvcCommIo CommIo;
    RtlZeroMemory(&CommIo, sizeof CommIo);

    CommIo.hIn = ProcParam->StandardInput;
    CommIo.hOut = ProcParam->StandardOutput;
    CommIo.hErr = ProcParam->StandardError;
    ConfigureStdHandles(&StdHandles, &CommIo);

    // Prepare environments
    ULONG nSize = ExpandEnvironmentStringsW(L"%PATH%", NULL, 0);
    PWSTR PathVariable = RtlAllocateHeap(HeapHandle, HEAP_ZERO_MEMORY, nSize * sizeof(wchar_t));
    ExpandEnvironmentStringsW(L"%PATH%", PathVariable, nSize);

    // Set console size from screen buffer co-ordinates
    COORD WindowSize;
    CONSOLE_SCREEN_BUFFER_INFOEX ConBuffer = { 0 };
    ConBuffer.cbSize = sizeof ConBuffer;

    GetConsoleScreenBufferInfoEx(CommIo.hOut, &ConBuffer);
    WindowSize.X = ConBuffer.srWindow.Right - ConBuffer.srWindow.Left + 1;
    WindowSize.Y = ConBuffer.srWindow.Bottom - ConBuffer.srWindow.Top + 1;

    hRes = (*wslSession)->CreateLxProcess(
        wslSession,
        DistroID,
        CommandLine,
        ArgumentCount,
        Arguments,
        ProcParam->CurrentDirectory.DosPath.Buffer,    // GetCurrentDircetoryW();
        PathVariable,                                  // Paths shared b/w Windows and WSL
        ProcParam->Environment,                        // GetEnvironmentStringsW();
        ProcParam->EnvironmentSize,
        LxssUserName,
        WindowSize.X,
        WindowSize.Y,
        ToULong(ConsoleHandle),
        &StdHandles,
        &InitiatedDistroID,
        &LxInstanceID,
        &ProcessHandle,
        &ServerHandle,
        &socket,
        &socket,
        &socket,
        &socket);

    if (SUCCEEDED(hRes))
    {
        wchar_t u_LxInstanceID[GUID_STRING];
        wchar_t u_InitiatedDistroID[GUID_STRING];

        PrintGuid(&LxInstanceID, u_LxInstanceID);
        PrintGuid(&InitiatedDistroID, u_InitiatedDistroID);

        wprintf(L"[+] CreateLxProcess: \n\t ProcessHandle: 0x%p \n\t ServerHandle: 0x%p \n\t"
                L" LxInstanceID: %ls \n\t InitiatedDistroID: %ls\n",
                ProcessHandle, ServerHandle,
                u_LxInstanceID, u_InitiatedDistroID);

        if (SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            NTSTATUS Status;
            IO_STATUS_BLOCK IoStatusBlock;
            LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG WaitForSignalMsg;
            WaitForSignalMsg.TimeOut = INFINITE;

            InitializeInterop(ServerHandle, &InitiatedDistroID, &LxInstanceID);

            // Use the IOCTL to wait on the process to terminate
            Status = ZwDeviceIoControlFile(ProcessHandle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           IOCTL_LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL,
                                           &WaitForSignalMsg, sizeof WaitForSignalMsg,
                                           &WaitForSignalMsg, sizeof WaitForSignalMsg);

            if (NT_SUCCESS(Status))
            {
                wprintf(L"[+] WaitForSignalMsg.ExitStatus: %u\n",
                        WaitForSignalMsg.ExitStatus);
            }
            else
                LogStatus(Status, L"ZwDeviceIoControlFile");
        }
    }
    else
        LogResult(hRes, L"CreateLxProcess");

    // Restore Console mode to previous state
    SetConsoleMode(CommIo.hIn, CommIo.InMode);
    SetConsoleMode(CommIo.hOut, CommIo.OutMode);
    SetConsoleMode(CommIo.hErr, CommIo.ErrMode);

    // Cleanup
    RtlFreeHeap(HeapHandle, 0, PathVariable);
    ZwClose(ProcessHandle);
    ZwClose(ServerHandle);

    return hRes;
}
