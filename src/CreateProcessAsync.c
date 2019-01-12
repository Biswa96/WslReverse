#include "CreateWinProcess.h" // include WinInternal.h
#include "Functions.h"
#include "LxBus.h"
#include <stdio.h>

NTSTATUS WaitForMessage(
    HANDLE ClientHandle,
    HANDLE EventHandle,
    PIO_STATUS_BLOCK IoStatusBlock)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoRequestToCancel = { 0 };
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -5 * TICKS_PER_MIN;

    Status = ZwWaitForSingleObject(EventHandle, FALSE, &Timeout);
    if (Status == STATUS_TIMEOUT)
    {
        ZwCancelIoFileEx(ClientHandle, &IoRequestToCancel, IoStatusBlock);
        Status = ZwWaitForSingleObject(EventHandle, FALSE, NULL);
    }

    return Status;
}

ULONG ProcessInteropMessages(
    HANDLE ReadPipeHandle,
    PLX_CREATE_PROCESS_RESULT ProcResult)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PROCESS_BASIC_INFORMATION BasicInfo = { 0 };
    LARGE_INTEGER ByteOffset = { 0 };
    LXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE LxTerminalMsg = { 0 };

    // Create an event to sync all reads and writes
    HANDLE EventHandle = NULL;
    Status = ZwCreateEvent(
        &EventHandle,
        EVENT_ALL_ACCESS,
        NULL,
        NotificationEvent,
        TRUE);

    // Read buffer from TIOCGWINSZ ioctl
    Status = ZwReadFile(
        ReadPipeHandle,
        EventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        &LxTerminalMsg,
        sizeof(LxTerminalMsg),
        &ByteOffset,
        NULL);
    if (Status == STATUS_PENDING)
    {
        HANDLE Handles[2] = { EventHandle, ProcResult->ProcInfo.hProcess };

        ZwWaitForMultipleObjects(
            ARRAY_SIZE(Handles),
            Handles,
            WaitAny,
            FALSE,
            NULL); // Temporary solution
    }

    Status = ZwQueryInformationProcess(
        ProcResult->ProcInfo.hProcess,
        ProcessBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        NULL);

    // Resize pseudo console when winsize.ws_row and winsize.ws_col received
    COORD ConsoleSize; 
    ConsoleSize.X = LxTerminalMsg.WindowWidth;
    ConsoleSize.Y = LxTerminalMsg.WindowHeight;
    ResizePseudoConsole(ProcResult->hpCon, ConsoleSize);

    ZwClose(EventHandle);
    return BasicInfo.ExitStatus;
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
    HANDLE EventHandle = NULL, HeapHandle = GetProcessHeap();
    RTL_CRITICAL_SECTION CriticalSection;

    // Create an event to sync all reads and writes
    bRes = RtlInitializeCriticalSectionEx(&CriticalSection, 0, 0);
    Status = ZwCreateEvent(
        &EventHandle,
        EVENT_ALL_ACCESS,
        NULL,
        SynchronizationEvent,
        FALSE);

    // Receive messages from client handle
    Status = ZwReadFile(
        ClientHandle,
        EventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        &Buffer,
        sizeof(Buffer),
        &ByteOffset,
        NULL);

    LxReceiveMsg = RtlAllocateHeap(
        HeapHandle,
        HEAP_ZERO_MEMORY,
        Buffer.NumberOfBytesToRead);

    Status = ZwReadFile(
        ClientHandle,
        EventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        LxReceiveMsg,
        Buffer.NumberOfBytesToRead,
        &ByteOffset,
        NULL);
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
        // Unmarshal standard I/O handles from file descriptors
        Status = ZwDeviceIoControlFile(
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

    // Create Windows process using unmarshalled VFS handles
    LX_CREATE_PROCESS_RESULT ProcResult = { 0 };
    bRes = CreateWinProcess(LxReceiveMsg, &ProcResult);
    if(!bRes)
        Log(GetLastError(), L"CreateWinProcess");

    // Create pipes to get console resize message
    HANDLE ReadPipeHandle = NULL, WritePipeHandle = NULL;
    Status = OpenAnonymousPipe(&ReadPipeHandle, &WritePipeHandle);
    Log(Status, L"OpenAnonymousPipe");

    // Marshal hWritePipe handle to get struct winsize from TIOCGWINSZ ioctl
    LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA HandleMessage = { 0 };
    HandleMessage.Handle = ToULong(WritePipeHandle);
    HandleMessage.Type = LxOutputPipeType;

    Status = ZwDeviceIoControlFile(
        ClientHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE,
        &HandleMessage, sizeof(HandleMessage),
        &HandleMessage, sizeof(HandleMessage));

    // Send this buffer so that Lx side can unmarshal the pipe
    LXSS_MESSAGE_PORT_SEND_OBJECT LxSendMsg = { 0 };
    LxSendMsg.InteropMessage.CreateNtProcessFlag = INTEROP_LXBUS_READ_NT_PROCESS_STATUS;
    LxSendMsg.InteropMessage.BufferSize = sizeof(LxSendMsg);
    LxSendMsg.InteropMessage.LastError = ProcResult.LastError;
    LxSendMsg.IsSubsystemGUI = ProcResult.IsSubsystemGUI;
    LxSendMsg.HandleMessage = HandleMessage;

    Status = ZwWriteFile(
        ClientHandle,
        EventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        &LxSendMsg,
        sizeof(LxSendMsg),
        &ByteOffset,
        NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);

    // Send NT process ExitStatus from ProcessInteropMessages
    LxSendMsg.InteropMessage.CreateNtProcessFlag = INTEROP_LXBUS_WRITE_NT_PROCESS_STATUS;
    LxSendMsg.InteropMessage.BufferSize = sizeof(LxSendMsg.InteropMessage);
    LxSendMsg.InteropMessage.LastError = ProcessInteropMessages(ReadPipeHandle, &ProcResult);

    Status = ZwWriteFile(
        ClientHandle,
        EventHandle,
        NULL,
        NULL,
        &IoStatusBlock,
        &LxSendMsg.InteropMessage,
        sizeof(LxSendMsg.InteropMessage),
        &ByteOffset,
        NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);

    // Close pseudo console if command is without pipe
    if(ProcResult.hpCon)
        ClosePseudoConsole(ProcResult.hpCon);

    // Cleanup
    RtlFreeHeap(HeapHandle, 0, LxReceiveMsg);
    ZwClose(EventHandle);
    if(ProcResult.ProcInfo.hProcess)
        ZwClose(ProcResult.ProcInfo.hProcess);
    if(ProcResult.ProcInfo.hThread)
        ZwClose(ProcResult.ProcInfo.hThread);
    ZwClose(ReadPipeHandle);
    ZwClose(WritePipeHandle);
    ZwClose(ClientHandle);
    RtlDeleteCriticalSection(&CriticalSection);
}
