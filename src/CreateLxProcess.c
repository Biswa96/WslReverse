#include "WslInstance.h"
#include "Functions.h"
#include "WinInternal.h"
#include "ConsolePid.h"
#include <stdio.h>

#define STATUS_SUCCESS 0
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#ifndef CTL_CODE
#define FILE_ANY_ACCESS 0
#define FILE_DEVICE_UNKNOWN 0x00000022
#define METHOD_NEITHER 3
#define CTL_CODE( DeviceType, Function, Method, Access ) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

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

    // Infinite loop to wait from client message
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

void InitializeInterop(PWslInstance* wslInstance, HANDLE ServerHandle)
{
    wchar_t WslHost[MAX_PATH], CommandLine[MAX_PATH];
    HANDLE CurrentProcHandle = NULL, serverHandle = NULL;
    GUID Guid;
    size_t size;
    PROCESS_INFORMATION ProcInfo;
    STARTUPINFOEXW SInfoEx;

    // Make handles inheritable by child process aka. wslhost.exe
    SetHandleInformation(ServerHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    HANDLE hProc = GetCurrentProcess();

    // Connect to LxssServerPort for Windows interopt
    if (DuplicateHandle(hProc, ServerHandle, hProc, &serverHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        PTP_WORK pwk = CreateThreadpoolWork(
            (PTP_WORK_CALLBACK)CreateProcessWorker,
            serverHandle,
            NULL);
        SubmitThreadpoolWork(pwk);
    }

    ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\lxss\\wslhost.exe", WslHost, MAX_PATH);
    hRes = (*wslInstance)->GetDistributionId(wslInstance, &Guid);
    Log(hRes, L"GetDistributionId");

    // Create an event to synchronize with wslhost.exe process
    HANDLE hEvent = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
    SetHandleInformation(hEvent, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    DuplicateHandle(hProc, hProc, hProc, &CurrentProcHandle, 0, TRUE, DUPLICATE_SAME_ACCESS);

    InitializeProcThreadAttributeList(NULL, 1, 0, &size);
    LPPROC_THREAD_ATTRIBUTE_LIST AttrList = malloc(size);
    InitializeProcThreadAttributeList(AttrList, 1, 0, &size);

    HANDLE Value[3] = { NULL };
    Value[0] = ServerHandle;
    Value[1] = hEvent;
    Value[2] = CurrentProcHandle;
    UpdateProcThreadAttribute(AttrList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        Value, sizeof(Value), NULL, NULL);

    // Create required string for CreateProcessW
    swprintf_s(CommandLine, MAX_PATH,
        L"%ls {%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} %ld %ld %ld",
        WslHost,
        Guid.Data1, Guid.Data2, Guid.Data3,
        Guid.Data4[0], Guid.Data4[1], Guid.Data4[2],
        Guid.Data4[3], Guid.Data4[4], Guid.Data4[5],
        Guid.Data4[6], Guid.Data4[7],
        PtrToUlong(ServerHandle),
        PtrToUlong(hEvent),
        PtrToUlong(CurrentProcHandle));

    memset(&SInfoEx, 0, sizeof(STARTUPINFOEXW));
    SInfoEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    SInfoEx.lpAttributeList = AttrList;

    // bInheritHandles must be TRUE for wslhost.exe
    BOOL bRes = CreateProcessW(
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
        WaitForMultipleObjects(ARRAY_SIZE(lpHandles), lpHandles, FALSE, 0);
    }
    else
    {
        Log(GetLastError(), L"CreteProcess");
    }

    // Cleanup handles
    NtClose(hEvent);
    NtClose(CurrentProcHandle);
    NtClose(ProcInfo.hProcess);
    NtClose(ProcInfo.hThread);
}

#define DISABLED_INPUT_MODE (ENABLE_INSERT_MODE | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT)
#define REQUIRED_INPUT_MODE (ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT)

void ConfigStdHandles(HANDLE hStdInput, PULONG InputMode, HANDLE hStdOutput, PULONG OutputMode)
{
    if (GetFileType(hStdInput) == FILE_TYPE_CHAR
        && GetConsoleMode(hStdInput, InputMode))
    {
        // Switch to VT-100 Input Console
        ULONG NewMode = (*InputMode & (~DISABLED_INPUT_MODE)) | REQUIRED_INPUT_MODE; // & 0xFFFFFFD8 | 0x208
        SetConsoleMode(hStdInput, NewMode);

        // Switch input to UTF-8
        SetConsoleCP(CP_UTF8);
    }

    if (GetConsoleMode(hStdOutput, OutputMode))
    {
        // Switch to VT-100 Output Console
        ULONG NewMode = (*OutputMode
            | DISABLE_NEWLINE_AUTO_RETURN
            | ENABLE_VIRTUAL_TERMINAL_PROCESSING
            | ENABLE_PROCESSED_OUTPUT); // 0xD
        SetConsoleMode(hStdOutput, NewMode);

        // Switch output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }
}

// Type casting used to know placement of standard handles
// Added in CreateLxProcess.c file

static X_PRTL_USER_PROCESS_PARAMETERS UserProcessParameter(void)
{
    return (X_PRTL_USER_PROCESS_PARAMETERS)NtCurrentTeb()->
        ProcessEnvironmentBlock->
        ProcessParameters;
}

HRESULT CreateLxProcess(PWslInstance* wslInstance)
{
    ULONG InputMode, OutputMode;
    HANDLE ProcessHandle = NULL, ServerHandle = NULL;

    // Preapare environments and argument
    PSTR Args[] = { "-bash" };
    ULONG nSize = ExpandEnvironmentStringsW(L"%PATH%", NULL, 0);
    PWSTR PathVariable = malloc(sizeof(wchar_t) * nSize);
    ExpandEnvironmentStringsW(L"%PATH%", PathVariable, nSize);

    // Configure Console Handles
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    ConfigStdHandles(hIn, &InputMode, hOut, &OutputMode);

    // LxssManager sets Standard Handles automatically
    LXSS_STD_HANDLES StdHandles;
    memset(&StdHandles, 0, sizeof(LXSS_STD_HANDLES));

#if 0
    // Fun with Lxss handles
    HANDLE FileHandle = CreateFileW(L"Alohomora.txt",
        GENERIC_ALL, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    StdHandles.StdIn.Handle = HandleToULong(FileHandle);
    StdHandles.StdIn.Pipe = TRUE;
    StdHandles.StdOut.Handle = HandleToULong(FileHandle);
    StdHandles.StdOut.Pipe = TRUE;
    StdHandles.StdErr.Handle = HandleToULong(FileHandle);
    StdHandles.StdErr.Pipe = TRUE;
#endif

    // Console Window handle of current process (if any)
    HANDLE ConsoleHandle = UserProcessParameter()->ConsoleHandle;
    ConsolePid(ConsoleHandle);

    hRes = (*wslInstance)->CreateLxProcess(
        wslInstance,
        "/bin/bash",
        ARRAY_SIZE(Args),
        Args,
        UserProcessParameter()->CurrentDirectory.DosPath.Buffer,    // GetCurrentDircetoryW();
        PathVariable,                                               // Paths shared b/w Windows and WSL
        UserProcessParameter()->Environment,                        // GetEnvironmentStringsW();
        UserProcessParameter()->EnvironmentSize,
        &StdHandles,
        HandleToULong(ConsoleHandle),
        L"root",
        &ProcessHandle,
        &ServerHandle);

    free(PathVariable);

    if (hRes >= ERROR_SUCCESS)
    {
        if (SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            ULONG ExitStatus = INFINITE;

            InitializeInterop(wslInstance, ServerHandle);

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
    CloseHandle(ProcessHandle);
    CloseHandle(ServerHandle);

    return hRes;
}
