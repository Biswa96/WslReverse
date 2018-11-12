#include "WslSession.h"
#include "Functions.h"
#include "WinInternal.h"
#include "ConsolePid.h"
#include <stdio.h>

#define STATUS_SUCCESS 0
#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#define DISABLED_INPUT_MODE (ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)
#define REQUIRED_INPUT_MODE (ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT)

// LxCore!LxpControlDeviceIoctlLxProcess
#define IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x34, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200D3u

// LxCore!LxpControlDeviceIoctlServerPort
#define IOCTL_ADSS_IPC_SERVER_WAIT_FOR_CONNECTION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0C, METHOD_NEITHER, FILE_ANY_ACCESS) //0x220033u

typedef union _LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG {
    ULONG Timeout;
    ULONG ClientHandle;
} LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG, *PLXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG;

// Global variables
HRESULT hRes;
NTSTATUS Status;
IO_STATUS_BLOCK Isb;

// Forward declarations
void CreateProcessAsync(
    PTP_CALLBACK_INSTANCE Instance,
    HANDLE ClientHandle,
    PTP_WORK Work);

void CreateProcessWorker(
    PTP_CALLBACK_INSTANCE Instance,
    HANDLE ServerHandle,
    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);
    PTP_WORK pwk = NULL;
    LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG ConnectionMsg = { 0 };

    // Infinite loop to wait for client message
    while (TRUE)
    {
        ConnectionMsg.Timeout = INFINITE;

        Status = NtDeviceIoControlFile(
            ServerHandle,
            NULL,
            NULL,
            NULL,
            &Isb,
            IOCTL_ADSS_IPC_SERVER_WAIT_FOR_CONNECTION,
            &ConnectionMsg, sizeof(ConnectionMsg),
            &ConnectionMsg, sizeof(ConnectionMsg));

        if (Status < STATUS_SUCCESS)
            break;

        wprintf(L"ClientHandle: %lu\n", ConnectionMsg.ClientHandle);

        pwk = CreateThreadpoolWork(
            (PTP_WORK_CALLBACK)CreateProcessAsync,
            ULongToHandle(ConnectionMsg.ClientHandle),
            NULL);

        ConnectionMsg.Timeout = 0;
        SubmitThreadpoolWork(pwk);
        CloseThreadpoolWork(pwk);
    }

    NtClose(ServerHandle);
}

BOOL InitializeInterop(
    HANDLE ServerHandle,
    GUID* CurrentDistroID)
{
    BOOL bRes;
    wchar_t WslHost[MAX_PATH], CommandLine[MAX_PATH];
    HANDLE ProcHandle, hServer, hEvent, hProc = NtCurrentProcess();
    HANDLE Value[3] = { NULL };
    size_t size;
    PROCESS_INFORMATION ProcInfo = { 0 };
    STARTUPINFOEXW SInfoEx = { 0 };

    // Create an event to synchronize with wslhost.exe process
    hEvent = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
    bRes = SetHandleInformation(hEvent, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
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
    InitializeProcThreadAttributeList(NULL, 1, 0, &size);
    LPPROC_THREAD_ATTRIBUTE_LIST AttrList = malloc(size);
    bRes = InitializeProcThreadAttributeList(AttrList, 1, 0, &size);

    Value[0] = ServerHandle;
    Value[1] = hEvent;
    Value[2] = ProcHandle;
    bRes = UpdateProcThreadAttribute(AttrList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, //0x20002u
        Value, sizeof(Value), NULL, NULL);

    // Create required string for CreateProcessW
    ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\lxss\\wslhost.exe", WslHost, MAX_PATH);

    _snwprintf_s(
        CommandLine,
        MAX_PATH * sizeof(wchar_t),
        MAX_PATH * sizeof(wchar_t),
        L"%ls {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} %ld %ld %ld",
        WslHost,
        CurrentDistroID->Data1, CurrentDistroID->Data2, CurrentDistroID->Data3,
        CurrentDistroID->Data4[0], CurrentDistroID->Data4[1], CurrentDistroID->Data4[2],
        CurrentDistroID->Data4[3], CurrentDistroID->Data4[4], CurrentDistroID->Data4[5],
        CurrentDistroID->Data4[6], CurrentDistroID->Data4[7],
        HandleToULong(ServerHandle),
        HandleToULong(hEvent),
        HandleToULong(ProcHandle));

    SInfoEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    SInfoEx.lpAttributeList = AttrList;

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

    free(AttrList);

    if (bRes)
    {
        HANDLE lpHandles[2] = { NULL };
        lpHandles[0] = ProcInfo.hProcess;
        lpHandles[1] = hEvent;
        WaitForMultipleObjectsEx(ARRAY_SIZE(lpHandles), lpHandles, FALSE, 0, FALSE);
    }
    else
    {
        Log(GetLastError(), L"CreteProcess");
    }

    // Cleanup handles
    NtClose(hEvent);
    NtClose(ProcHandle);
    NtClose(ProcInfo.hProcess);
    NtClose(ProcInfo.hThread);
    return bRes;
}

HRESULT CreateLxProcess(
    PWslSession* WslSession,
    GUID* DistroID)
{
    ULONG InputMode = 0, OutputMode = 0, NewMode = 0;
    HANDLE ProcessHandle, ServerHandle;
    GUID CurrentDistroID, LxInstanceID;
    COORD WindowSize;
    CONSOLE_SCREEN_BUFFER_INFOEX ConBuffer = { 0 };
    PVOID socket = NULL;

    // LxssManager sets Standard Handles automatically
    LXSS_STD_HANDLES StdHandles = { 0 };

    // Console Window handle of current process (if any)
    X_PRTL_USER_PROCESS_PARAMETERS ProcParam = UserProcessParameter();
    HANDLE ConsoleHandle = ProcParam->ConsoleHandle;
    ConsolePid(ConsoleHandle);

    // Preapare environments and argument
    PSTR Args[] = { "-bash" };
    ULONG nSize = ExpandEnvironmentStringsW(L"%PATH%", NULL, 0);
    PWSTR PathVariable = malloc(nSize * sizeof(wchar_t));
    ExpandEnvironmentStringsW(L"%PATH%", PathVariable, nSize);

    // Configure Console Handles
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (GetFileType(hIn) == FILE_TYPE_CHAR
        && GetConsoleMode(hIn, &InputMode))
    {
        // Switch to VT-100 Input Console
        NewMode = (InputMode & (~DISABLED_INPUT_MODE)) | REQUIRED_INPUT_MODE; // & 0xFFFFFFD8 | 0x208
        SetConsoleMode(hIn, NewMode);

        // Switch input to UTF-8
        SetConsoleCP(CP_UTF8);
    }

    if (GetConsoleMode(hOut, &OutputMode))
    {
        // Switch to VT-100 Output Console
        NewMode = (OutputMode
            | DISABLE_NEWLINE_AUTO_RETURN
            | ENABLE_VIRTUAL_TERMINAL_PROCESSING
            | ENABLE_PROCESSED_OUTPUT); // 0xD
        SetConsoleMode(hOut, NewMode);

        // Switch output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }

    // Set console size from screen buffer co-ordinates
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

    hRes = (*WslSession)->CreateLxProcess(
        WslSession,
        DistroID,
        "/bin/bash",
        ARRAY_SIZE(Args),
        Args,
        ProcParam->CurrentDirectory.DosPath.Buffer,    // GetCurrentDircetoryW();
        PathVariable,                                  // Paths shared b/w Windows and WSL
        ProcParam->Environment,                        // GetEnvironmentStringsW();
        ProcParam->EnvironmentSize,
        L"root",
        WindowSize.X,
        WindowSize.Y,
        HandleToULong(ConsoleHandle),
        &StdHandles,
        &CurrentDistroID,
        &LxInstanceID,
        &ProcessHandle,
        &ServerHandle,
        &socket,
        &socket,
        &socket,
        &socket);

    free(PathVariable);

    if (hRes >= ERROR_SUCCESS)
    {
        if (SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            ULONG ExitStatus = INFINITE;

            InitializeInterop(ServerHandle, &CurrentDistroID);

            // Use the IOCTL to wait on the process to terminate
            Status = NtDeviceIoControlFile(
                ProcessHandle,
                NULL,
                NULL,
                NULL,
                &Isb,
                IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL,
                &ExitStatus, sizeof(ExitStatus),
                &ExitStatus, sizeof(ExitStatus));

            if(Status >= STATUS_SUCCESS)
                GetExitCodeProcess(ProcessHandle, &ExitStatus);
        }
    }

    // Restore Console mode to previous state
    SetConsoleMode(hIn, InputMode);
    SetConsoleMode(hOut, OutputMode);

    // Cleanup handles
    NtClose(ProcessHandle);
    NtClose(ServerHandle);

    return hRes;
}
