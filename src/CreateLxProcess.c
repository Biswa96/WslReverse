#include "CreateLxProcess.h"
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
    StdHandles.StdIn.Handle = FileHandle;
    StdHandles.StdIn.Pipe = TRUE;
    StdHandles.StdOut.Handle = FileHandle;
    StdHandles.StdOut.Pipe = TRUE;
    StdHandles.StdErr.Handle = FileHandle;
    StdHandles.StdErr.Pipe = TRUE;
#endif

    // Console Window handle of current process (if any)
    ULONG ConsoleHandle = PtrToUlong(NtCurrentTeb()->
        ProcessEnvironmentBlock->ProcessParameters->Reserved2[0]);

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
        ConsoleHandle,
        L"root",
        &ProcessHandle,
        &ServerHandle);

    if (result >= ERROR_SUCCESS)
    {
        if (SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            ULONG ExitStatus, status;

            //InitializeInterop(wslInstance, ServerHandle);

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
