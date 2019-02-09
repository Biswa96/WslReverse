#include "CreateWinProcess.h"
#include "WinInternal.h"
#include "Log.h"
#include "LxBus.h"
#include <stdio.h>

NTSTATUS
WaitForMessage(HANDLE ClientHandle,
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

NTSTATUS
OpenAnonymousPipe(PHANDLE ReadPipeHandle,
                  PHANDLE WritePipeHandle)
{
    NTSTATUS Status;
    HANDLE hPipeServer = NULL, hNamedPipeFile = NULL, hFile = NULL;
    LARGE_INTEGER DefaultTimeOut;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PipeObj = { 0 };
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };

    hPipeServer = CreateFileW(L"\\\\.\\pipe\\",
                              GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

    if (hPipeServer == INVALID_HANDLE_VALUE)
        LogResult(GetLastError(), L"CreateFileW");

    DefaultTimeOut.QuadPart = -2 * TICKS_PER_MIN;
    ObjectAttributes.ObjectName = &PipeObj;
    ObjectAttributes.RootDirectory = hPipeServer;
    ObjectAttributes.Length = sizeof ObjectAttributes;

    Status = ZwCreateNamedPipeFile(&hNamedPipeFile,
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
    LogStatus(Status, L"ZwCreateNamedPipeFile");

    ObjectAttributes.Length = sizeof ObjectAttributes;
    ObjectAttributes.RootDirectory = hNamedPipeFile;
    ObjectAttributes.ObjectName = &PipeObj;

    Status = ZwOpenFile(&hFile,
                        GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_NON_DIRECTORY_FILE);
    LogStatus(Status, L"ZwOpenFile");

    // Return handles to caller
    *ReadPipeHandle = hNamedPipeFile;
    *WritePipeHandle = hFile;
    ZwClose(hPipeServer);
    return Status;
}

ULONG 
ProcessInteropMessages(HANDLE ReadPipeHandle,
                       PLX_CREATE_PROCESS_RESULT ProcResult)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PROCESS_BASIC_INFORMATION BasicInfo = { 0 };
    LARGE_INTEGER ByteOffset = { 0 };
    LXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE LxTerminalMsg = { 0 };

    // Create an event to sync all reads and writes
    HANDLE EventHandle = NULL;
    Status = ZwCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           TRUE);
    LogStatus(Status, L"ZwCreateEvent");

    // Read buffer from TIOCGWINSZ ioctl
    Status = ZwReadFile(ReadPipeHandle,
                        EventHandle,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &LxTerminalMsg,
                        sizeof LxTerminalMsg,
                        &ByteOffset,
                        NULL);
    LogStatus(Status, L"ZwReadFile");

    if (Status == STATUS_PENDING)
    {
        HANDLE Handles[2] = { NULL };
        Handles[0] = EventHandle;
        Handles[1] = ProcResult->ProcInfo.hProcess;

        ZwWaitForMultipleObjects(ARRAY_SIZE(Handles),
                                 Handles,
                                 WaitAny,
                                 FALSE,
                                 NULL); // Temporary solution
    }

    Status = ZwQueryInformationProcess(ProcResult->ProcInfo.hProcess,
                                       ProcessBasicInformation,
                                       &BasicInfo,
                                       sizeof BasicInfo,
                                       NULL);
    LogStatus(Status, L"ZwQueryInformationProcess");

    // Resize pseudo console when winsize.ws_row and winsize.ws_col received
    COORD ConsoleSize; 
    ConsoleSize.X = LxTerminalMsg.WindowWidth;
    ConsoleSize.Y = LxTerminalMsg.WindowHeight;
    ResizePseudoConsole(ProcResult->hpCon, ConsoleSize);

    ZwClose(EventHandle);
    return BasicInfo.ExitStatus;
}

void
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
    LXSS_MESSAGE_PORT_RECEIVE_OBJECT Buffer;
    PLXSS_MESSAGE_PORT_RECEIVE_OBJECT LxReceiveMsg = NULL;
    HANDLE EventHandle = NULL;
    HANDLE HeapHandle = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap;
    RTL_CRITICAL_SECTION CriticalSection;

    // Create an event to sync all reads and writes
    bRes = RtlInitializeCriticalSectionEx(&CriticalSection, 0, 0);
    Status = ZwCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);
    LogStatus(Status, L"ZwCreateEvent");

    // Receive messages from client handle
    Status = ZwReadFile(ClientHandle,
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

    Status = ZwReadFile(ClientHandle,
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
        LogStatus(Status, L"ZwReadFile");

    for (int i = 0; i < TOTAL_IO_HANDLES; i++)
    {
        // Unmarshal standard I/O handles from file descriptors
        wprintf(L"[+] VfsMsg: \n\t HandleIdCount: %llu\n\t",
                LxReceiveMsg->VfsMsg[i].HandleIdCount);

        Status = ZwDeviceIoControlFile(ClientHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE,
                                       &LxReceiveMsg->VfsMsg[i], sizeof LxReceiveMsg->VfsMsg[i],
                                       &LxReceiveMsg->VfsMsg[i], sizeof LxReceiveMsg->VfsMsg[i]);

        if (NT_SUCCESS(Status))
        {
            wprintf(L" Handle: %u\n",
                    LxReceiveMsg->VfsMsg[i].Handle);
        }
        else
            LogStatus(Status, L"ZwDeviceIoControlFile");

        bRes = SetHandleInformation(ToHandle(LxReceiveMsg->VfsMsg[i].Handle),
                                    HANDLE_FLAG_INHERIT,
                                    HANDLE_FLAG_INHERIT);
    }

    // Create Windows process using unmarshalled VFS handles
    LX_CREATE_PROCESS_RESULT ProcResult = { 0 };
    bRes = CreateWinProcess(LxReceiveMsg, &ProcResult);

    // Create pipes to get console resize message
    HANDLE ReadPipeHandle = NULL, WritePipeHandle = NULL;
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
    LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA HandleMsg = { 0 };
    HandleMsg.Handle = ToULong(WritePipeHandle);
    HandleMsg.Type = LxOutputPipeType;

    Status = ZwDeviceIoControlFile(ClientHandle,
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
        LogStatus(Status, L"ZwDeviceIoControlFile");

    // Send this buffer so that Lx side can unmarshal the pipe
    LXSS_MESSAGE_PORT_SEND_OBJECT LxSendMsg = { 0 };
    LxSendMsg.InteropMessage.CreateNtProcessFlag = INTEROP_LXBUS_READ_NT_PROCESS_STATUS;
    LxSendMsg.InteropMessage.BufferSize = sizeof LxSendMsg;
    LxSendMsg.InteropMessage.LastError = ProcResult.LastError;
    LxSendMsg.IsSubsystemGUI = ProcResult.IsSubsystemGUI;
    LxSendMsg.HandleMessage = HandleMsg;

    Status = ZwWriteFile(ClientHandle,
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
    LogStatus(Status, L"ZwWriteFile");

    // Send NT process ExitStatus from ProcessInteropMessages
    LxSendMsg.InteropMessage.CreateNtProcessFlag = INTEROP_LXBUS_WRITE_NT_PROCESS_STATUS;
    LxSendMsg.InteropMessage.BufferSize = sizeof LxSendMsg.InteropMessage;
    LxSendMsg.InteropMessage.LastError = ProcessInteropMessages(ReadPipeHandle, &ProcResult);

    Status = ZwWriteFile(ClientHandle,
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
    LogStatus(Status, L"ZwWriteFile");

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
