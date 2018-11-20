#include "ConsolePid.h"
#include "CreateProcessAsync.h"
#include "Functions.h"
#include "WslSession.h"
#include "WinInternal.h" // some defined expression
#include "LxBus.h" // For IOCTLs values
#include <stdio.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#define DISABLED_INPUT_MODE (ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)
#define REQUIRED_INPUT_MODE (ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT)

void CreateProcessWorker(
    PTP_CALLBACK_INSTANCE Instance,
    HANDLE ServerHandle,
    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    NTSTATUS Status;
    IO_STATUS_BLOCK Isb;
    PTP_WORK pwk = NULL;
    LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG ConnectionMsg = { 0 };

    // Infinite loop to wait for client message
    while (TRUE)
    {
        ConnectionMsg.Timeout = INFINITE;
        wprintf(
            L"[*] CreateProcessWorker ServerHandle: %lu\n",
            ToULong(ServerHandle));

        // IoCreateFile creates \Device\lxss\{Instance-GUID}\MessagePort
        Status = NtDeviceIoControlFile(
            ServerHandle,
            NULL,
            NULL,
            NULL,
            &Isb,
            IOCTL_ADSS_IPC_SERVER_WAIT_FOR_CONNECTION,
            &ConnectionMsg, sizeof(ConnectionMsg),
            &ConnectionMsg, sizeof(ConnectionMsg));

        if (!NT_SUCCESS(Status))
            break;

        wprintf(
            L"[*] CreateProcessWorker ClientHandle: %lu\n",
            ConnectionMsg.ClientHandle);

        pwk = CreateThreadpoolWork(
            (PTP_WORK_CALLBACK)CreateProcessAsync,
            ToHandle(ConnectionMsg.ClientHandle),
            NULL);

        ConnectionMsg.ClientHandle = 0;
        SubmitThreadpoolWork(pwk);
        CloseThreadpoolWork(pwk);
    }

    NtClose(ServerHandle);
}

BOOL InitializeInterop(
    HANDLE ServerHandle,
    GUID* CurrentDistroID)
{
    wchar_t WslHost[MAX_PATH], CommandLine[MAX_PATH];
    HANDLE ProcHandle, hServer, hProc = NtCurrentProcess();

    // Create an event to synchronize with wslhost.exe process
    HANDLE EventHandle = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
    BOOL bRes = SetHandleInformation(EventHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    bRes = SetHandleInformation(ServerHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    bRes = DuplicateHandle(hProc, hProc, hProc, &ProcHandle, 0, TRUE, DUPLICATE_SAME_ACCESS);
    bRes = DuplicateHandle(hProc, ServerHandle, hProc, &hServer, 0, FALSE, DUPLICATE_SAME_ACCESS);

    // Connect to LxssMessagePort for Windows interopt
    PTP_WORK pwk = CreateThreadpoolWork(
        (PTP_WORK_CALLBACK)CreateProcessWorker,
        hServer,
        NULL);
    SubmitThreadpoolWork(pwk);

    // Configure all the required handles for wslhost.exe process
    size_t AttrbSize;
    InitializeProcThreadAttributeList(NULL, 1, 0, &AttrbSize);
    LPPROC_THREAD_ATTRIBUTE_LIST AttrbList = malloc(AttrbSize);
    bRes = InitializeProcThreadAttributeList(AttrbList, 1, 0, &AttrbSize);

    HANDLE Value[3] = { NULL };
    Value[0] = ServerHandle;
    Value[1] = EventHandle;
    Value[2] = ProcHandle;
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
    STARTUPINFOEXW SInfoEx = { 0 }; // Must set all members to Zero
    SInfoEx.StartupInfo.cb = sizeof(SInfoEx);
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
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
        L"[*] CreateLxProcess hProcess: %lu hThread: %lu dwProcessId: %lu\n",
        CommandLine,
        ToULong(ProcInfo.hProcess),
        ToULong(ProcInfo.hThread),
        ProcInfo.dwProcessId);

    if (bRes)
    {
        HANDLE Handles[2] = { NULL };
        Handles[0] = ProcInfo.hProcess;
        Handles[1] = EventHandle;
        WaitForMultipleObjectsEx(ARRAY_SIZE(Handles), Handles, FALSE, 0, FALSE);
    }
    else
    {
        Log(GetLastError(), L"CreteProcess");
    }

    // Cleanup
    free(AttrbList);
    NtClose(EventHandle);
    // NtClose(hServer) causes STATUS_INVALID_HANDLE in CreateProcessWorker
    NtClose(ProcHandle);
    NtClose(ProcInfo.hProcess);
    NtClose(ProcInfo.hThread);
    return bRes;
}

// Cast the pre-defined structure with X_ prefixed one
#define UserProcessParameter() \
    (X_PRTL_USER_PROCESS_PARAMETERS)NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters

HRESULT CreateLxProcess(
    PWslSession* WslSession,
    GUID* DistroID)
{
    ULONG InputMode = 0, OutputMode = 0;
    HANDLE ProcessHandle, ServerHandle;
    GUID InitiatedDistroId, LxInstanceID;
    COORD WindowSize;
    PVOID socket = NULL;

    // LxssManager sets Standard Handles automatically
    LXSS_STD_HANDLES StdHandles = { 0 };

    // Console Window handle of current process (if any)
    X_PRTL_USER_PROCESS_PARAMETERS ProcParam = UserProcessParameter();
    HANDLE ConsoleHandle = ProcParam->ConsoleHandle;
    ConsolePid(ConsoleHandle, L"CreateLxProcess");

    // Preapare environments and argument
    char* Arguments[] = { "-bash" };
    ULONG nSize = ExpandEnvironmentStringsW(L"%PATH%", NULL, 0);
    wchar_t* PathVariable = malloc(nSize * sizeof(wchar_t));
    ExpandEnvironmentStringsW(L"%PATH%", PathVariable, nSize);

    // Configure Console Handles
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (GetFileType(hIn) == FILE_TYPE_CHAR
        && GetConsoleMode(hIn, &InputMode))
    {
        // Switch to VT-100 Input Console
        ULONG NewMode = (InputMode & (~DISABLED_INPUT_MODE)) | REQUIRED_INPUT_MODE; // & 0xFFFFFFD8 | 0x208
        SetConsoleMode(hIn, NewMode);

        // Switch input to UTF-8
        SetConsoleCP(CP_UTF8);
    }

    if (GetConsoleMode(hOut, &OutputMode))
    {
        // Switch to VT-100 Output Console
        ULONG NewMode = (OutputMode
            | DISABLE_NEWLINE_AUTO_RETURN
            | ENABLE_VIRTUAL_TERMINAL_PROCESSING
            | ENABLE_PROCESSED_OUTPUT); // 0xD
        SetConsoleMode(hOut, NewMode);

        // Switch output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }

    // Set console size from screen buffer co-ordinates
    CONSOLE_SCREEN_BUFFER_INFOEX ConBuffer = { 0 };
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

    HRESULT hRes = (*WslSession)->CreateLxProcess(
        WslSession,
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
        &InitiatedDistroId,
        &LxInstanceID,
        &ProcessHandle,
        &ServerHandle,
        &socket,
        &socket,
        &socket,
        &socket);

    if (hRes >= ERROR_SUCCESS)
    {
        wchar_t GuidString[GUID_STRING];

        PrintGuid(&LxInstanceID, GuidString);
        wprintf(L"[*] LxInstanceID: %ls\n", GuidString);

        if (SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            IO_STATUS_BLOCK Isb;
            LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG WaitForSignalMsg;
            WaitForSignalMsg.ExitStatus = INFINITE;

            InitializeInterop(ServerHandle, &InitiatedDistroId);

            // Use the IOCTL to wait on the process to terminate
            NTSTATUS Status = NtDeviceIoControlFile(
                ProcessHandle,
                NULL,
                NULL,
                NULL,
                &Isb,
                IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL,
                &WaitForSignalMsg, sizeof(WaitForSignalMsg),
                &WaitForSignalMsg, sizeof(WaitForSignalMsg));

            if (!NT_SUCCESS(Status))
            {
                GetExitCodeProcess(ProcessHandle, &WaitForSignalMsg.ExitStatus);
                Log(WaitForSignalMsg.ExitStatus, L"WaitForSignalMsg");
            }
        }
    }

    // Restore Console mode to previous state
    SetConsoleMode(hIn, InputMode);
    SetConsoleMode(hOut, OutputMode);

    // Cleanup
    free(PathVariable);
    NtClose(ProcessHandle);
    NtClose(ServerHandle);

    return hRes;
}
