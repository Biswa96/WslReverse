#include "CreateWinProcess.h" // include WinInternal.h
#include "Functions.h"
#include "LxBus.h" // For IOCTLs values
#include <stdio.h>

NTSTATUS WaitForMessage(
    HANDLE ClientHandle,
    HANDLE hEvent,
    PIO_STATUS_BLOCK IoStatusBlock)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoRequestToCancel;
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -5 * TICKS_PER_MIN;

    Status = NtWaitForSingleObject(hEvent, FALSE, &Timeout);
    if (Status == STATUS_TIMEOUT)
    {
        NtCancelIoFileEx(ClientHandle, &IoRequestToCancel, IoStatusBlock);
        Status = NtWaitForSingleObject(hEvent, FALSE, NULL);
    }

    return Status;
}

void CreateProcessAsync(
    PTP_CALLBACK_INSTANCE Instance,
    HANDLE ClientHandle,
    PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    BOOL bRes;
    IO_STATUS_BLOCK Isb;
    LARGE_INTEGER ByteOffset = { 0 };
    LXSS_MESSAGE_PORT_OBJECT Buffer;
    LX_CREATE_PROCESS_RESULT ProcResult = { 0 };

    // Create an event to sync all reads and writes
    HANDLE hEvent = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);

    // Receive messages from client handle
    NTSTATUS Status = NtReadFile(ClientHandle, hEvent, NULL, NULL, &Isb, &Buffer, sizeof(Buffer), &ByteOffset, NULL);
    PLXSS_MESSAGE_PORT_OBJECT LxMsg = malloc(Buffer.NumberOfBytesToRead);
    Status = NtReadFile(ClientHandle, hEvent, NULL, NULL, &Isb, LxMsg, Buffer.NumberOfBytesToRead, &ByteOffset, NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, hEvent, &Isb);
    Log(Status, L"LxssMessagePortReceive");

    // Logging strings
    printf("[*] ApplicationPath: %s\n", LxMsg->Unknown + LxMsg->WinApplicationPathOffset);
    printf("[*] CommandArgument: %s\n", LxMsg->Unknown + LxMsg->WinCommandArgumentOffset);
    printf("[*] CurrentPath: %s\n", LxMsg->Unknown + LxMsg->WinCurrentPathOffset);
    printf("[*] WslEnv: %s\n", LxMsg->Unknown + LxMsg->WslEnvOffset);

    for (int i = 0; i < TOTAL_IO_HANDLES; i++)
    {
        // IoCreateFile creates \Device\lxss\{Instance-GUID}\VfsFile
        Status = NtDeviceIoControlFile(
            ClientHandle,
            NULL,
            NULL,
            NULL,
            &Isb,
            IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE,
            &LxMsg->VfsHandle[i], sizeof(LxMsg->VfsHandle[i]),
            &LxMsg->VfsHandle[i], sizeof(LxMsg->VfsHandle[i]));

        bRes = SetHandleInformation(
            ToHandle(LxMsg->VfsHandle[i].Handle),
            HANDLE_FLAG_INHERIT,
            HANDLE_FLAG_INHERIT);
    }

    COORD ConsoleSize;
    ConsoleSize.X = LxMsg->WindowWidth;
    ConsoleSize.Y = LxMsg->WindowHeight;

    HANDLE hReadPipe, hWritePipe;
    Status = OpenAnonymousPipe(&hReadPipe, &hWritePipe);
    Log(Status, L"OpenAnonymousPipe");

    bRes = CreateWinProcess(
        LxMsg->IsWithoutPipe,
        &ConsoleSize,
        ToHandle(LxMsg->VfsHandle[0].Handle), // Input
        ToHandle(LxMsg->VfsHandle[1].Handle), // Output
        ToHandle(LxMsg->VfsHandle[2].Handle), // Error
        LxMsg->Unknown + LxMsg->WinApplicationPathOffset,
        LxMsg->Unknown + LxMsg->WinCurrentPathOffset,
        &ProcResult);

    if(!bRes)
        Log(bRes, L"CreateWinProcess");

    // Check handle then marshal to init
    LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_MSG HandleMsg;
    HandleMsg.Handle = ToULong(hWritePipe);
    HandleMsg.Type = LxOutputPipeType;

    Status = NtDeviceIoControlFile(
        ClientHandle,
        NULL,
        NULL,
        NULL,
        &Isb,
        IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE,
        &HandleMsg, sizeof(HandleMsg),
        &HandleMsg, sizeof(HandleMsg));

    // Close pseudo console if command is without pipe
    // ClosePseudoConsole(ProcResult.hpCon);

    free(LxMsg);
    NtClose(hEvent);
    NtClose(ProcResult.ProcInfo.hProcess);
    NtClose(ProcResult.ProcInfo.hThread);
    NtClose(hReadPipe);
    NtClose(hWritePipe);
    NtClose(ClientHandle);
}
