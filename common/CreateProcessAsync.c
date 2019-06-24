#include "WinInternal.h"
#include "LxBus.h"
#include "CreateWinProcess.h"
#include "Helpers.h"
#include <stdio.h>

NTSTATUS
WINAPI
WaitForMessage(HANDLE ClientHandle,
               HANDLE EventHandle,
               PIO_STATUS_BLOCK IoRequestToCancel)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -5 * TICKS_PER_MIN;

    Status = NtWaitForSingleObject(EventHandle, FALSE, &Timeout);
    if (Status == STATUS_TIMEOUT)
    {
        NtCancelIoFileEx(ClientHandle, IoRequestToCancel, &IoStatusBlock);
        Status = NtWaitForSingleObject(EventHandle, FALSE, NULL);
    }

    return Status;
}

NTSTATUS
WINAPI
OpenAnonymousPipe(PHANDLE ReadPipeHandle,
                  PHANDLE WritePipeHandle)
{
    NTSTATUS Status;
    HANDLE hPipeServer, hNamedPipeFile, hFile;
    LARGE_INTEGER DefaultTimeOut;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING ObjectName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    hPipeServer = CreateFileW(L"\\\\.\\pipe\\",
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

    if (hPipeServer == INVALID_HANDLE_VALUE)
        Log(RtlGetLastWin32Error(), L"CreateFileW");

    DefaultTimeOut.QuadPart = -2 * TICKS_PER_MIN;

    RtlZeroMemory(&ObjectName, sizeof ObjectName);
    RtlZeroMemory(&ObjectAttributes, sizeof ObjectAttributes);
    ObjectAttributes.ObjectName = &ObjectName;
    ObjectAttributes.RootDirectory = hPipeServer;
    ObjectAttributes.Length = sizeof ObjectAttributes;

    Status = NtCreateNamedPipeFile(&hNamedPipeFile,
                                   GENERIC_READ | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
                                   &ObjectAttributes,
                                   &IoStatusBlock,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_CREATE,
                                   FILE_PIPE_INBOUND,
                                   FILE_PIPE_BYTE_STREAM_TYPE,
                                   FILE_PIPE_BYTE_STREAM_MODE,
                                   FILE_PIPE_QUEUE_OPERATION,
                                   1,
                                   PAGE_SIZE,
                                   PAGE_SIZE,
                                   &DefaultTimeOut);
    LogStatus(Status, L"NtCreateNamedPipeFile");

    ObjectAttributes.Length = sizeof ObjectAttributes;
    ObjectAttributes.RootDirectory = hNamedPipeFile;
    ObjectAttributes.ObjectName = &ObjectName;

    Status = NtOpenFile(&hFile,
                        GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_NON_DIRECTORY_FILE);
    LogStatus(Status, L"NtOpenFile");

    // Return handles to caller
    *ReadPipeHandle = hNamedPipeFile;
    *WritePipeHandle = hFile;
    if(hPipeServer)
        NtClose(hPipeServer);
    return Status;
}

ULONG
WINAPI
ProcessInteropMessages(HANDLE ReadPipeHandle,
                       PLX_CREATE_PROCESS_RESULT ProcResult)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PROCESS_BASIC_INFORMATION BasicInfo;
    LARGE_INTEGER ByteOffset = { 0 };
    LXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE LxTerminalMsg;
    RtlZeroMemory(&LxTerminalMsg, sizeof LxTerminalMsg);

    // Create an event to sync all reads and writes
    HANDLE EventHandle = NULL;
    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);
    LogStatus(Status, L"NtCreateEvent");

    // Read buffer from TIOCGWINSZ ioctl
    Status = NtReadFile(ReadPipeHandle,
                        EventHandle,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &LxTerminalMsg,
                        sizeof LxTerminalMsg,
                        &ByteOffset,
                        NULL);
    LogStatus(Status, L"NtReadFile");

    if (Status == STATUS_PENDING)
    {
        HANDLE Handles[2];
        Handles[0] = EventHandle;
        Handles[1] = ProcResult->ProcInfo.hProcess;

        NtWaitForMultipleObjects(ARRAY_SIZE(Handles),
                                 Handles,
                                 WaitAny,
                                 FALSE,
                                 NULL); // Temporary solution
    }

    RtlZeroMemory(&BasicInfo, sizeof BasicInfo);
    Status = NtQueryInformationProcess(ProcResult->ProcInfo.hProcess,
                                       ProcessBasicInformation,
                                       &BasicInfo,
                                       sizeof BasicInfo,
                                       NULL);
    LogStatus(Status, L"NtQueryInformationProcess");

    // Resize pseudo console when winsize.ws_row and winsize.ws_col received
    COORD ConsoleSize; 
    ConsoleSize.X = LxTerminalMsg.WindowWidth;
    ConsoleSize.Y = LxTerminalMsg.WindowHeight;
    ResizePseudoConsole(ProcResult->hpCon, ConsoleSize);

    if(EventHandle)
        NtClose(EventHandle);
    return BasicInfo.ExitStatus;
}

void
WINAPI
CreateProcessAsync(PTP_CALLBACK_INSTANCE Instance,
                   HANDLE ClientHandle,
                   PTP_WORK Work)
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    BOOL bRes;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset = { 0 };
    LXSS_MESSAGE_PORT_RECEIVE_OBJECT Buffer, *LxReceiveMsg = NULL;
    HANDLE EventHandle = NULL;
    HANDLE HeapHandle = RtlGetProcessHeap();

    // Create an event to sync all reads and writes
    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    LogStatus(Status, L"NtCreateEvent");

    // Receive messages from client handle
    Status = NtReadFile(ClientHandle,
                        EventHandle,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &Buffer,
                        sizeof Buffer,
                        &ByteOffset,
                        NULL);

    LxReceiveMsg = RtlAllocateHeap(HeapHandle,
                                   HEAP_ZERO_MEMORY,
                                   Buffer.NumberOfBytesToRead);

    Status = NtReadFile(ClientHandle,
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

    // Logging strings
    if (NT_SUCCESS(Status))
    {
        wprintf(L"[+] CreateProcessAsync: \n\t ApplicationPath: %hs\n\t"
                L" CommandArgument: %hs\n\t CurrentPath: %hs\n\t WslEnv: %hs\n",
                LxReceiveMsg->Unknown + LxReceiveMsg->WinApplicationPathOffset,
                LxReceiveMsg->Unknown + LxReceiveMsg->WinCommandArgumentOffset,
                LxReceiveMsg->Unknown + LxReceiveMsg->WinCurrentPathOffset,
                LxReceiveMsg->Unknown + LxReceiveMsg->WslEnvOffset);
    }
    else
        LogStatus(Status, L"NtReadFile");

    for (int i = 0; i < TOTAL_IO_HANDLES; i++)
    {
        // Unmarshal standard I/O handles from file descriptors
        wprintf(L"[+] VfsMsg: \n\t HandleIdCount: %llu\n\t",
                LxReceiveMsg->VfsMsg[i].HandleIdCount);

        Status = NtDeviceIoControlFile(ClientHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE,
                                       &LxReceiveMsg->VfsMsg[i], sizeof LxReceiveMsg->VfsMsg[i],
                                       &LxReceiveMsg->VfsMsg[i], sizeof LxReceiveMsg->VfsMsg[i]);

        if (NT_SUCCESS(Status))
            wprintf(L" Handle: %u\n", LxReceiveMsg->VfsMsg[i].Handle);
        else
            LogStatus(Status, L"NtDeviceIoControlFile");

        bRes = SetHandleInformation(ToHandle(LxReceiveMsg->VfsMsg[i].Handle),
                                    HANDLE_FLAG_INHERIT,
                                    HANDLE_FLAG_INHERIT);
    }

    // Create Windows process using unmarshalled VFS handles
    LX_CREATE_PROCESS_RESULT ProcResult;
    RtlZeroMemory(&ProcResult, sizeof ProcResult);
    bRes = CreateWinProcess(LxReceiveMsg, &ProcResult);
    if(!bRes)
        wprintf(L"Can't create Win32 process...\n");

    // Create pipes to get console resize message
    HANDLE ReadPipeHandle, WritePipeHandle;
    Status = OpenAnonymousPipe(&ReadPipeHandle, &WritePipeHandle);

    if (NT_SUCCESS(Status))
    {
        wprintf(L"[+] OpenAnonymousPipe: \n\t"
                L" ReadPipeHandle: 0x%p \n\t WritePipeHandle: 0x%p\n",
                ReadPipeHandle, WritePipeHandle);
    }
    else
        LogStatus(Status, L"OpenAnonymousPipe");

    // Marshal hWritePipe handle to get struct winsize from TIOCGWINSZ ioctl
    LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA HandleMsg;
    HandleMsg.Handle = ToULong(WritePipeHandle);
    HandleMsg.Type = LxOutputPipeType;

    Status = NtDeviceIoControlFile(ClientHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE,
                                   &HandleMsg, sizeof HandleMsg,
                                   &HandleMsg, sizeof HandleMsg);

    if (NT_SUCCESS(Status))
    {
        wprintf(L"[+] HandleMsg: \n\t HandleIdCount: %llu\n",
                HandleMsg.HandleIdCount);
    }
    else
        LogStatus(Status, L"NtDeviceIoControlFile");

    // Send this buffer so that Lx side can unmarshal the pipe
    LXSS_MESSAGE_PORT_SEND_OBJECT LxSendMsg;
    RtlZeroMemory(&LxSendMsg, sizeof LxSendMsg);

    LxSendMsg.InteropMessage.CreateNtProcessFlag = INTEROP_LXBUS_READ_NT_PROCESS_STATUS;
    LxSendMsg.InteropMessage.BufferSize = sizeof LxSendMsg;
    LxSendMsg.InteropMessage.LastError = ProcResult.LastError;
    LxSendMsg.IsSubsystemGUI = ProcResult.IsSubsystemGUI;
    LxSendMsg.HandleMessage.HandleIdCount = HandleMsg.HandleIdCount;

    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &LxSendMsg,
                         sizeof LxSendMsg,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);
    LogStatus(Status, L"NtWriteFile");

    // Send NT process ExitStatus from ProcessInteropMessages
    LxSendMsg.InteropMessage.CreateNtProcessFlag = INTEROP_LXBUS_WRITE_NT_PROCESS_STATUS;
    LxSendMsg.InteropMessage.BufferSize = sizeof LxSendMsg.InteropMessage;
    LxSendMsg.InteropMessage.LastError = ProcessInteropMessages(ReadPipeHandle, &ProcResult);

    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &LxSendMsg.InteropMessage,
                         sizeof LxSendMsg.InteropMessage,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);
    LogStatus(Status, L"NtWriteFile");

    // Close pseudo console if command is without pipe
    if(ProcResult.hpCon)
        ClosePseudoConsole(ProcResult.hpCon);

    // Cleanup
    if(LxReceiveMsg)
        RtlFreeHeap(HeapHandle, 0, LxReceiveMsg);
    NtClose(EventHandle);
    if(ProcResult.ProcInfo.hProcess)
        NtClose(ProcResult.ProcInfo.hProcess);
    if(ProcResult.ProcInfo.hThread)
        NtClose(ProcResult.ProcInfo.hThread);
    NtClose(ReadPipeHandle);
    NtClose(WritePipeHandle);
    NtClose(ClientHandle);
}
