#include "GetConhostServerId.h"
#include "CreateProcessAsync.h"
#include "Functions.h"
#include "WslSession.h"
#include "WinInternal.h" // PEB and some defined expression
#include "LxBus.h" // For IOCTLs values
#include <stdio.h>

void CreateProcessWorker(
    PTP_CALLBACK_INSTANCE Instance,
    HANDLE ServerHandle,
    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PTP_WORK WorkReturn = NULL;
    LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG ConnectionMsg = { 0 };

    // Infinite loop to wait for client message
    while (TRUE)
    {
        ConnectionMsg.Timeout = INFINITE;
        wprintf(
            L"[*] CreateProcessWorker ServerHandle: %lu\n",
            ToULong(ServerHandle));

        Status = ZwDeviceIoControlFile(
            ServerHandle,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            IOCTL_LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION,
            &ConnectionMsg, sizeof(ConnectionMsg),
            &ConnectionMsg, sizeof(ConnectionMsg));

        if (!NT_SUCCESS(Status))
            break;

        wprintf(
            L"[*] CreateProcessWorker ClientHandle: %ld\n",
            ConnectionMsg.ClientHandle);

        Status = TpAllocWork(
            &WorkReturn,
            (PTP_WORK_CALLBACK)CreateProcessAsync,
            ToHandle(ConnectionMsg.ClientHandle),
            NULL);

        if (NT_SUCCESS(Status))
            Log(Status, L"CreateProcessAsync TpAllocWork");

        ConnectionMsg.ClientHandle = 0;
        TpPostWork(WorkReturn);
        TpReleaseWork(WorkReturn);
    }

    ZwClose(ServerHandle);
}

BOOL InitializeInterop(
    HANDLE ServerHandle,
    GUID* CurrentDistroID,
    GUID* LxInstanceID)
{
    UNREFERENCED_PARAMETER(LxInstanceID); // For future use in Hyper-V VM Mode

    BOOL bRes;
    NTSTATUS Status;
    wchar_t WslHost[MAX_PATH], CommandLine[MAX_PATH];
    HANDLE EventHandle, ProcHandle, ServerHandleDup, HeapHandle = GetProcessHeap();

    // Create an event to synchronize with wslhost.exe process
    Status = ZwCreateEvent(
        &EventHandle,
        EVENT_ALL_ACCESS,
        NULL,
        SynchronizationEvent,
        FALSE);

    bRes = SetHandleInformation(EventHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    bRes = SetHandleInformation(ServerHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    Status = ZwDuplicateObject(
        ZwCurrentProcess(),
        ZwCurrentProcess(),
        ZwCurrentProcess(),
        &ProcHandle,
        0,
        OBJ_INHERIT,
        DUPLICATE_SAME_ACCESS);
    Status = ZwDuplicateObject(
        ZwCurrentProcess(),
        ServerHandle,
        ZwCurrentProcess(),
        &ServerHandleDup,
        0,
        0,
        DUPLICATE_SAME_ACCESS);

    // Connect to LxssMessagePort for Windows interopt
    PTP_WORK WorkReturn = NULL;
    Status = TpAllocWork(
        &WorkReturn,
        (PTP_WORK_CALLBACK)CreateProcessWorker,
        ServerHandleDup,
        NULL);

    if (NT_SUCCESS(Status))
        Log(Status, L"CreateProcessWorker TpAllocWork");
    TpPostWork(WorkReturn);

    // Configure all the required handles for wslhost.exe process
    size_t AttrbSize = 0;
    LPPROC_THREAD_ATTRIBUTE_LIST AttrbList = NULL;
    InitializeProcThreadAttributeList(NULL, 1, 0, &AttrbSize);
    AttrbList = RtlAllocateHeap(HeapHandle, HEAP_ZERO_MEMORY, AttrbSize);
    bRes = InitializeProcThreadAttributeList(AttrbList, 1, 0, &AttrbSize);

    HANDLE Value[3] = { ServerHandle, EventHandle, ProcHandle };
    if(AttrbList)
        bRes = UpdateProcThreadAttribute(
            AttrbList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, //0x20002u
            Value, sizeof(Value), NULL, NULL);

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
    SInfoEx.StartupInfo.cb = sizeof(SInfoEx);
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    // SInfoEx.StartupInfo.wShowWindow = SW_SHOWDEFAULT;
    SInfoEx.lpAttributeList = AttrbList;

    // bInheritHandles must be TRUE for wslhost.exe
    bRes = CreateProcessW(
        NULL,
        CommandLine,
        NULL,
        NULL,
        TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &SInfoEx.StartupInfo,
        &ProcInfo);

    wprintf(
        L"[*] WslHost CommandLine: %ls\n"
        L"[*] WslHost hProcess: %lu hThread: %lu dwProcessId: %lu\n",
        CommandLine,
        ToULong(ProcInfo.hProcess),
        ToULong(ProcInfo.hThread),
        ProcInfo.dwProcessId);

    if (bRes)
    {
        HANDLE Handles[2] = { ProcInfo.hProcess, EventHandle };
        Status = ZwWaitForMultipleObjects(
            ARRAY_SIZE(Handles),
            Handles,
            WaitAny,
            FALSE,
            NULL);
    }
    else
    {
        Log(GetLastError(), L"CreteProcess");
    }

    // Cleanup
    RtlFreeHeap(HeapHandle, 0, AttrbList);
    ZwClose(EventHandle);
    // ZwClose(hServer) causes STATUS_INVALID_HANDLE in CreateProcessWorker
    ZwClose(ProcHandle);
    ZwClose(ProcInfo.hProcess);
    ZwClose(ProcInfo.hThread);
    return bRes;
}

#define DISABLED_INPUT_MODE (ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)
#define ENABLE_INPUT_MODE (ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT)
#define ENABLE_OUTPUT_MODE (DISABLE_NEWLINE_AUTO_RETURN | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT)

HRESULT CreateLxProcess(
    PWslSession* wslSession,
    GUID* DistroID)
{
    ULONG InputMode = 0, OutputMode = 0;
    HANDLE ProcessHandle = NULL, ServerHandle = NULL;
    GUID InitiatedDistroID, LxInstanceID;
    PVOID socket = NULL;
    HANDLE HeapHandle = GetProcessHeap();

    // LxssManager sets Standard Handles automatically
    LXSS_STD_HANDLES StdHandles = { 0 };

    // Console Window handle of current process (if any)
    PRTL_USER_PROCESS_PARAMETERS ProcParam = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters;
    HANDLE ConsoleHandle = ProcParam->ConsoleHandle;
    wprintf(
        L"[*] CreateLxProcess ConHost PID: %lld Handle: %ld\n",
        GetConhostServerId(ConsoleHandle),
        ToULong(ConsoleHandle));

    // Preapare environments and argument
    PSTR Arguments[] = { "-bash" };
    ULONG nSize = ExpandEnvironmentStringsW(L"%PATH%", NULL, 0);
    PWSTR PathVariable = RtlAllocateHeap(HeapHandle, HEAP_ZERO_MEMORY, nSize * sizeof(wchar_t));
    ExpandEnvironmentStringsW(L"%PATH%", PathVariable, nSize);

    // Configure Console Handles
    HANDLE hIn = ProcParam->StandardInput;
    HANDLE hOut = ProcParam->StandardOutput;

    if (GetFileType(hIn) == FILE_TYPE_CHAR && GetConsoleMode(hIn, &InputMode))
    {
        // Switch to VT-100 Input Console
        ULONG NewMode = (InputMode & (~DISABLED_INPUT_MODE)) | ENABLE_INPUT_MODE; // & 0xFFFFFFD8 | 0x208
        SetConsoleMode(hIn, NewMode);

        // Switch input to UTF-8
        SetConsoleCP(CP_UTF8);
    }

    if (GetConsoleMode(hOut, &OutputMode))
    {
        // Switch to VT-100 Output Console
        ULONG NewMode = (OutputMode | ENABLE_OUTPUT_MODE); // 0xD
        SetConsoleMode(hOut, NewMode);

        // Switch output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }

    // Set console size from screen buffer co-ordinates
    COORD WindowSize;
    CONSOLE_SCREEN_BUFFER_INFOEX ConBuffer = { 0 };
    ConBuffer.cbSize = sizeof(ConBuffer);
    GetConsoleScreenBufferInfoEx(hOut, &ConBuffer);
    WindowSize.X = ConBuffer.srWindow.Right - ConBuffer.srWindow.Left + 1;
    WindowSize.Y = ConBuffer.srWindow.Bottom - ConBuffer.srWindow.Top + 1;

#if 0
    // Fun with Lxss handles
    StdHandles.StdIn.Handle = HandleToULong(hIn);
    StdHandles.StdIn.Pipe = 1;
    StdHandles.StdOut.Handle = HandleToULong(hOut);
    StdHandles.StdOut.Pipe = 2;
    StdHandles.StdErr.Handle = HandleToULong(hOut);
    StdHandles.StdErr.Pipe = 2;
#endif

    HRESULT hRes = (*wslSession)->CreateLxProcess(
        wslSession,
        DistroID,
        "/bin/bash",
        ARRAY_SIZE(Arguments),
        Arguments,
        ProcParam->CurrentDirectory.DosPath.Buffer,    // GetCurrentDircetoryW();
        PathVariable,                                  // Paths shared b/w Windows and WSL
        ProcParam->Environment,                        // GetEnvironmentStringsW();
        ProcParam->EnvironmentSize,
        L"root",
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
    Log(hRes, L"CreateLxProcess");

    if (hRes >= ERROR_SUCCESS)
    {
        wchar_t GuidString[GUID_STRING];

        PrintGuid(&LxInstanceID, GuidString);
        wprintf(L"[*] LxInstanceID: %ls\n", GuidString);

        if (SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            NTSTATUS Status;
            IO_STATUS_BLOCK IoStatusBlock;
            LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG WaitForSignalMsg;
            WaitForSignalMsg.TimeOut = INFINITE;

            InitializeInterop(ServerHandle, &InitiatedDistroID, &LxInstanceID);

            // Use the IOCTL to wait on the process to terminate
            Status = ZwDeviceIoControlFile(
                ProcessHandle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL,
                &WaitForSignalMsg, sizeof(WaitForSignalMsg),
                &WaitForSignalMsg, sizeof(WaitForSignalMsg));

            if (NT_SUCCESS(Status))
                Log(WaitForSignalMsg.ExitStatus, L"WaitForSignalMsg");
        }
    }

    // Restore Console mode to previous state
    SetConsoleMode(hIn, InputMode);
    SetConsoleMode(hOut, OutputMode);

    // Cleanup
    RtlFreeHeap(HeapHandle, 0, PathVariable);
    ZwClose(ProcessHandle);
    ZwClose(ServerHandle);

    return hRes;
}
