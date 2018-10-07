#include "CreateLxProcess.h"
#include "Functions.h"
#include <stdio.h>
#include <winternl.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#ifndef CTL_CODE
#define FILE_ANY_ACCESS 0
#define FILE_DEVICE_UNKNOWN 0x00000022
#define METHOD_NEITHER 3
#define CTL_CODE( DeviceType, Function, Method, Access ) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

//LxpControlDeviceIoctlLxProcess
#define IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x34, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200D3u

//LxpControlDeviceIoctlServerPort
#define IOCTL_ADSS_IPC_SERVER_WAIT_FOR_CONNECTION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0C, METHOD_NEITHER, FILE_ANY_ACCESS) //0x220033u

// Forward Declarations
void ConsolePid(HANDLE ConsoleHandle);

// Disable "nonstandard extension used" warning
#pragma warning(push)
#pragma warning(disable: 4204)

void InitializeInterop(pWslInstance* wslInstance, HANDLE ServerHandle)
{
    wchar_t WslHost[MAX_PATH], CommandLine[MAX_PATH];
    HANDLE hTarget = NULL;
    GUID Guid;
    size_t size;
    PROCESS_INFORMATION ProcInfo;
    STARTUPINFOEXW SInfoEx;

    ExpandEnvironmentStringsW(L"%WINDIR%\\System32\\lxss\\wslhost.exe", WslHost, MAX_PATH);
    HRESULT hr = (*wslInstance)->GetDistributionId(wslInstance, &Guid);
    Log(hr, L"GetDistributionId");

    // CreateThreadpoolWork((PTP_WORK_CALLBACK)CreateProcessWorker, TargetHandle, NULL);

    // Configure all the required handles
    HANDLE hEvent = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
    HANDLE hProc = GetCurrentProcess();
    DuplicateHandle(hProc, hProc, hProc, &hTarget, 0, TRUE, DUPLICATE_SAME_ACCESS);

    // Make handles inheritable by child process aka. wslhost.exe
    SetHandleInformation(hEvent, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(ServerHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    InitializeProcThreadAttributeList(NULL, 1, 0, &size);
    LPPROC_THREAD_ATTRIBUTE_LIST AttrList = malloc(size);
    InitializeProcThreadAttributeList(AttrList, 1, 0, &size);

    HANDLE Value[3] = { ServerHandle, hEvent, hTarget };
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
        PtrToUlong(hTarget));

    memset(&SInfoEx, 0, sizeof(STARTUPINFOEXW));
    SInfoEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    SInfoEx.lpAttributeList = AttrList;

    // bInheritHandles must be TRUE for wslhost.exe
    if (CreateProcessW(NULL, CommandLine, NULL, NULL, TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE,
        NULL, NULL, &SInfoEx.StartupInfo, &ProcInfo))
    {
        HANDLE lpHandles[2] = { ProcInfo.hProcess, hEvent };
        WaitForMultipleObjects(ARRAY_SIZE(lpHandles), lpHandles, FALSE, 0);
    }
    else
    {
        Log(GetLastError(), L"CreteProcess");
    }

    // Cleanup handles
    free(AttrList);
    CloseHandle(hEvent);
    CloseHandle(ProcInfo.hProcess);
    CloseHandle(ProcInfo.hThread);
}

#pragma warning(pop)

void ConfigStdHandles(HANDLE hStdInput, PULONG InputMode, HANDLE hStdOutput, PULONG OutputMode)
{

    if (GetFileType(hStdInput) == FILE_TYPE_CHAR
        && GetConsoleMode(hStdInput, InputMode))
    {
        // Switch to VT-100 Input Console
        ULONG NewMode = (*InputMode &
            ~(ENABLE_INSERT_MODE
                | ENABLE_ECHO_INPUT
                | ENABLE_LINE_INPUT
                | ENABLE_PROCESSED_INPUT)
            | (ENABLE_VIRTUAL_TERMINAL_INPUT
                | ENABLE_WINDOW_INPUT)); // & 0xFFFFFFD8 | 0x208
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

HRESULT CreateLxProcess(pWslInstance* wslInstance)
{
    ULONG InputMode, OutputMode;
    HANDLE ProcessHandle = NULL, ServerHandle = NULL;

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
    HANDLE ConsoleHandle = NtCurrentTeb()->
        ProcessEnvironmentBlock->ProcessParameters->Reserved2[0];

#if defined (_DEBUG) || defined (DEBUG)
    ConsolePid(ConsoleHandle);
#endif

    PSTR Args[] = {"-bash"};

    HRESULT result = (*wslInstance)->CreateLxProcess(
        wslInstance,
        "/bin/bash",
        ARRAY_SIZE(Args),
        Args,
        L"C:\\Users",
        L"C:\\Windows\\System32;C:\\Windows",
        NULL,
        0,
        &StdHandles,
        HandleToULong(ConsoleHandle),
        L"root",
        &ProcessHandle,
        &ServerHandle);

    if (result >= ERROR_SUCCESS)
    {
        if (SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            ULONG ExitStatus, status;

            InitializeInterop(wslInstance, ServerHandle);

            // Use the IOCTL to wait on the process to terminate
            status = DeviceIoControl(
                ProcessHandle,
                IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL,
                &ExitStatus, sizeof(int),
                &ExitStatus, sizeof(int),
                NULL, NULL);

            if(status)
                GetExitCodeProcess(ProcessHandle, &ExitStatus);
        }
    }

    // Restore Console mode to previous state
    SetConsoleMode(hIn, InputMode);
    SetConsoleMode(hOut, OutputMode);

    // Cleanup handles
    CloseHandle(ProcessHandle);
    CloseHandle(ServerHandle);

    return result;
}
