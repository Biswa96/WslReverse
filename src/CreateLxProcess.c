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

#define IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x34, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200D3u

void ConfigStdHandles(HANDLE hStdInput, ULONG InputMode, HANDLE hConout, ULONG ConoutMode)
{

    if (GetFileType(hStdInput) == FILE_TYPE_CHAR
        && GetConsoleMode(hStdInput, &InputMode))
    {
        // Switch to VT-100 Input Console
        ULONG NewMode = InputMode &
            ~(ENABLE_INSERT_MODE
                | ENABLE_ECHO_INPUT
                | ENABLE_LINE_INPUT
                | ENABLE_PROCESSED_INPUT)
            | (ENABLE_VIRTUAL_TERMINAL_INPUT
                | ENABLE_WINDOW_INPUT); // & 0xFFFFFFD8 | 0x208
        SetConsoleMode(hStdInput, NewMode);

        // Switch input to UTF-8
        SetConsoleCP(CP_UTF8);
    }

    if (GetConsoleMode(hConout, &ConoutMode))
    {
        // Switch to VT-100 Output Console
        ULONG NewMode = ConoutMode
            | DISABLE_NEWLINE_AUTO_RETURN
            | ENABLE_VIRTUAL_TERMINAL_PROCESSING
            | ENABLE_PROCESSED_OUTPUT; // 0xD
        SetConsoleMode(hConout, NewMode);

        // Switch output to UTF-8
        SetConsoleOutputCP(CP_UTF8);
    }
}

HRESULT CreateLxProcess(pWslInstance* wslInstance)
{
    ULONG InputMode = 0, ConoutMode = 0;
    HANDLE ProcessHandle = NULL;
    HANDLE ServerHandle = NULL;

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    HANDLE hCon = CreateFileW(
        L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

    // Configure Console Handles
    ConfigStdHandles(hIn, InputMode, hCon, ConoutMode);

    LXSS_STD_HANDLES StdHandles;
    memset(&StdHandles, 0, sizeof(LXSS_STD_HANDLES));
    StdHandles.hStdInput = hIn;
    StdHandles.hStdOutput = hOut;
    StdHandles.hStdError = hErr;
    StdHandles.hConout = hCon;

    ULONG ConsoleHandle = PtrToUlong(NtCurrentTeb()->
        ProcessEnvironmentBlock->ProcessParameters->Reserved2[0]);

    PSTR Args[] = {"--login"};

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

    if (result == ERROR_SUCCESS)
    {
        ULONG ExitStatus;

        SetHandleInformation(ProcessHandle, HANDLE_FLAG_INHERIT, 0);

        // Use the IOCTL to wait on the process to terminate
        DeviceIoControl(
            ProcessHandle,
            IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL,
            &ExitStatus, sizeof(int),
            &ExitStatus, sizeof(int),
            NULL, NULL);

        // InitializeInterop(ServerHandle);
    }

    // Restore Console mode to previous state
    SetConsoleMode(hIn, InputMode);
    SetConsoleMode(hCon, ConoutMode);

    // Cleanup handles
    CloseHandle(ProcessHandle);
    CloseHandle(ServerHandle);

    return result;
}
