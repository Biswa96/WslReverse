#include "CreateWinProcess.h" // include WinInternal.h and LxBus.h
#include "Functions.h"
#include <stdio.h>

NTSTATUS WaitForMessage(
    HANDLE ClientHandle,
    HANDLE EventHandle,
    PIO_STATUS_BLOCK IoStatusBlock)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoRequestToCancel;
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -5 * TICKS_PER_MIN;

    Status = NtWaitForSingleObject(EventHandle, FALSE, &Timeout);
    if (Status == STATUS_TIMEOUT)
    {
        NtCancelIoFileEx(ClientHandle, &IoRequestToCancel, IoStatusBlock);
        Status = NtWaitForSingleObject(EventHandle, FALSE, NULL);
    }

    return Status;
}

ULONG ProcessInteropMessages(
    HANDLE ReadPipeHandle,
    PLX_CREATE_PROCESS_RESULT ProcResult)
{
    UNREFERENCED_PARAMETER(ReadPipeHandle);
    UNREFERENCED_PARAMETER(ProcResult);
    DWORD ExitCode = ERROR_INVALID_FUNCTION;
    return ExitCode;
}

void CreateProcessAsync(
    PTP_CALLBACK_INSTANCE Instance,
    HANDLE ClientHandle,
    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    BOOL bRes;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset = { 0 };
    LXSS_MESSAGE_PORT_RECEIVE_OBJECT Buffer;
    PLXSS_MESSAGE_PORT_RECEIVE_OBJECT LxReceiveMsg = NULL;
    LX_CREATE_PROCESS_RESULT ProcResult = { 0 };

    // Create an event to sync all reads and writes
    HANDLE EventHandle = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);

    // Receive messages from client handle
    Status = NtReadFile(ClientHandle, EventHandle, NULL, NULL, &IoStatusBlock, &Buffer, sizeof(Buffer), &ByteOffset, NULL);
    LxReceiveMsg = malloc(Buffer.NumberOfBytesToRead);
    Status = NtReadFile(ClientHandle, EventHandle, NULL, NULL, &IoStatusBlock, LxReceiveMsg, Buffer.NumberOfBytesToRead, &ByteOffset, NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);
    Log(Status, L"LxssMessagePortReceive");

    // Logging strings
    wprintf(
        L"[*] ApplicationPath: %hs\n[*] CommandArgument: %hs\n"
        L"[*] CurrentPath: %hs\n[*] WslEnv: %hs\n",
        LxReceiveMsg->Unknown + LxReceiveMsg->WinApplicationPathOffset,
        LxReceiveMsg->Unknown + LxReceiveMsg->WinCommandArgumentOffset,
        LxReceiveMsg->Unknown + LxReceiveMsg->WinCurrentPathOffset,
        LxReceiveMsg->Unknown + LxReceiveMsg->WslEnvOffset);

    for (int i = 0; i < TOTAL_IO_HANDLES; i++)
    {
        // IoCreateFile creates \Device\lxss\{Instance-GUID}\VfsFile
        Status = NtDeviceIoControlFile(
            ClientHandle,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE,
            &LxReceiveMsg->VfsHandle[i], sizeof(LxReceiveMsg->VfsHandle[i]),
            &LxReceiveMsg->VfsHandle[i], sizeof(LxReceiveMsg->VfsHandle[i]));

        bRes = SetHandleInformation(
            ToHandle(LxReceiveMsg->VfsHandle[i].Handle),
            HANDLE_FLAG_INHERIT,
            HANDLE_FLAG_INHERIT);
    }

    if(!CreateWinProcess(LxReceiveMsg, &ProcResult))
        Log(bRes, L"CreateWinProcess");

    //Make pipes for receving console resize message
    HANDLE hReadPipe, hWritePipe;
    Status = OpenAnonymousPipe(&hReadPipe, &hWritePipe);
    Log(Status, L"OpenAnonymousPipe");

    // Marshal hWritePipe handle to get struct winsize from TIOCGWINSZ ioctl
    LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA HandleMsg;
    HandleMsg.Handle = ToULong(hWritePipe);
    HandleMsg.Type = LxOutputPipeType;

    Status = NtDeviceIoControlFile(
        ClientHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE,
        &HandleMsg, sizeof(HandleMsg),
        &HandleMsg, sizeof(HandleMsg));

    ProcessInteropMessages(hReadPipe, &ProcResult);

    // Close pseudo console if command is without pipe
    // ClosePseudoConsole(ProcResult.hpCon);

    free(LxReceiveMsg);
    NtClose(EventHandle);
    NtClose(ProcResult.ProcInfo.hProcess);
    NtClose(ProcResult.ProcInfo.hThread);
    NtClose(hReadPipe);
    NtClose(hWritePipe);
    NtClose(ClientHandle);
}
