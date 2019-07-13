#include "WinInternal.h"
#include "CreateProcessAsync.h"
#include "GetConhostServerId.h"
#include "Helpers.h"
#include "LxssUserSession.h"
#include "LxBus.h"
#include "SpawnWslHost.h"
#include "VmModeWorker.h"
#include <stdio.h>

#define DISABLED_INPUT_MODE (ENABLE_INSERT_MODE |\
                             ENABLE_ECHO_INPUT  |\
                             ENABLE_LINE_INPUT  |\
                             ENABLE_PROCESSED_INPUT)

#define ENABLE_INPUT_MODE (ENABLE_VIRTUAL_TERMINAL_INPUT |\
                           ENABLE_WINDOW_INPUT)

#define ENABLE_OUTPUT_MODE (DISABLE_NEWLINE_AUTO_RETURN |\
                            ENABLE_VIRTUAL_TERMINAL_PROCESSING |\
                            ENABLE_PROCESSED_OUTPUT)

typedef struct _SvcCommIo {
    HANDLE hIn;
    HANDLE hOut;
    HANDLE hErr;
    ULONG InMode;
    ULONG OutMode;
    ULONG ErrMode;
} SvcCommIo, *PSvcCommIo;

void
WINAPI
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
WINAPI
CreateProcessWorker(PTP_CALLBACK_INSTANCE Instance,
                    HANDLE ServerHandle,
                    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PTP_WORK WorkReturn;
    LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG WaitMsg;

    // Infinite loop to wait for client message
    while (TRUE)
    {
        WaitMsg.Timeout = INFINITE;

        Status = NtDeviceIoControlFile(ServerHandle,
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
                             CreateProcessAsync,
                             ToHandle(WaitMsg.ClientHandle),
                             NULL);
        LogStatus(Status, L"CreateProcessAsync TpAllocWork");

        WaitMsg.ClientHandle = 0;
        TpPostWork(WorkReturn);
        TpReleaseWork(WorkReturn);
    }

    NtClose(ServerHandle);
}

BOOL
WINAPI
InitializeInterop(HANDLE ServerHandle,
                  GUID* InitiatedDistroID)
{
    BOOL bRes;
    NTSTATUS Status;
    HANDLE hProc = NtCurrentProcess(), ServerHandleDup;

    bRes = SetHandleInformation(ServerHandle,
                                HANDLE_FLAG_INHERIT,
                                HANDLE_FLAG_INHERIT);

    Status = NtDuplicateObject(hProc, ServerHandle,
                               hProc, &ServerHandleDup,
                               0, 0, DUPLICATE_SAME_ACCESS);

    /**
    * Create a thread pool which waits for ClientHandle from LxssMessagePort
    * upon executing any Windows binary through init
    **/
    PTP_WORK WorkReturn;
    Status = TpAllocWork(&WorkReturn, CreateProcessWorker, ServerHandleDup, NULL);
    LogStatus(Status, L"CreateProcessWorker TpAllocWork");
    TpPostWork(WorkReturn);

    /* Execute the backend host without LxInstanceID aka. WSL1 */
    bRes = SpawnWslHost(ServerHandle, InitiatedDistroID, NULL);
    return bRes;
}

HRESULT
WINAPI
CreateLxProcess(ILxssUserSession* wslSession,
                GUID* DistroID,
                PSTR CommandLine,
                PSTR* Arguments,
                ULONG ArgumentCount,
                PWSTR LxssUserName)
{
    HRESULT hRes;
    HANDLE LxProcessHandle, ServerHandle;
    GUID InitiatedDistroID, LxInstanceID;
    SOCKET SockIn, SockOut, SockErr, ServerSocket;
    HANDLE HeapHandle = RtlGetProcessHeap();

    // Console Window handle of current process (if any)
    PRTL_USER_PROCESS_PARAMETERS ProcParam = GetUserProcessParameter();
    HANDLE ConsoleHandle = ProcParam->ConsoleHandle;
    wprintf(L"[+] ConHost: \n\t"
            L" ConhostServerId: %llu \n\t ConsoleHandle: 0x%p\n",
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
    PWSTR PathVariable = RtlAllocateHeap(HeapHandle, 0, nSize * sizeof(wchar_t));
    ExpandEnvironmentStringsW(L"%PATH%", PathVariable, nSize);

    // Set console size from screen buffer co-ordinates
    COORD WindowSize;
    CONSOLE_SCREEN_BUFFER_INFOEX ConBuffer;
    RtlZeroMemory(&ConBuffer, sizeof ConBuffer);
    ConBuffer.cbSize = sizeof ConBuffer;

    GetConsoleScreenBufferInfoEx(CommIo.hOut, &ConBuffer);
    WindowSize.X = ConBuffer.srWindow.Right - ConBuffer.srWindow.Left + 1;
    WindowSize.Y = ConBuffer.srWindow.Bottom - ConBuffer.srWindow.Top + 1;

    hRes = wslSession->lpVtbl->CreateLxProcess(
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
        &LxProcessHandle,
        &ServerHandle,
        &SockIn,
        &SockOut,
        &SockErr,
        &ServerSocket);

    if (SUCCEEDED(hRes) && (LxProcessHandle != NULL && ServerSocket == 0)) /* WSL1 */
    {
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;

        UNICODE_STRING LxInstanceIDstring, InitiatedDistroIDstring;
        RtlStringFromGUID(&LxInstanceID, &LxInstanceIDstring);
        RtlStringFromGUID(&InitiatedDistroID, &InitiatedDistroIDstring);

        // Get NT side Process ID of CommandLine process
        LXBUS_LX_PROCESS_HANDLE_GET_NT_PID_MSG LxProcMsg;

        Status = NtDeviceIoControlFile(LxProcessHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_LXBUS_LX_PROCESS_HANDLE_GET_NT_PID,
                                       NULL, 0,
                                       &LxProcMsg, sizeof LxProcMsg);

        wprintf(L"[+] CreateLxProcess: \n\t"
                L" NtPid: %u \n\t LxProcessHandle: 0x%p \n\t ServerHandle: 0x%p \n\t"
                L" LxInstanceID: %ls \n\t InitiatedDistroID: %ls\n",
                LxProcMsg.NtPid, LxProcessHandle, ServerHandle,
                LxInstanceIDstring.Buffer, InitiatedDistroIDstring.Buffer);

        RtlFreeUnicodeString(&LxInstanceIDstring);
        RtlFreeUnicodeString(&InitiatedDistroIDstring);

        if (SetHandleInformation(LxProcessHandle, HANDLE_FLAG_INHERIT, 0))
        {
            InitializeInterop(ServerHandle, &InitiatedDistroID);

            LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG WaitForSignalMsg;
            WaitForSignalMsg.TimeOut = INFINITE;

            /* Use the IOCTL to wait on the process to terminate */
            Status = NtDeviceIoControlFile(LxProcessHandle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           IOCTL_LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL,
                                           &WaitForSignalMsg, sizeof WaitForSignalMsg,
                                           &WaitForSignalMsg, sizeof WaitForSignalMsg);

            if (NT_SUCCESS(Status))
                wprintf(L"[+] WaitForSignalMsg.ExitStatus: %u\n", WaitForSignalMsg.ExitStatus);
            else
                LogStatus(Status, L"NtDeviceIoControlFile");
        }

        NtClose(LxProcessHandle);
        NtClose(ServerHandle);
    }
    else if (SUCCEEDED(hRes) && (LxProcessHandle == NULL && ServerSocket != 0)) /* WSL2 */
    {
        VmModeWorker(SockIn, SockOut, SockErr, ServerSocket, &StdHandles);
        closesocket(SockIn);
        closesocket(SockOut);
        closesocket(SockErr);
        closesocket(ServerSocket);
    }
    else
        Log(hRes, L"CreateLxProcess");

    // Restore Console mode to previous state
    SetConsoleMode(CommIo.hIn, CommIo.InMode);
    SetConsoleMode(CommIo.hOut, CommIo.OutMode);
    SetConsoleMode(CommIo.hErr, CommIo.ErrMode);

    // Cleanup
    RtlFreeHeap(HeapHandle, 0, PathVariable);
    return hRes;
}
