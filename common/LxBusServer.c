#include "WinInternal.h"
#include "CreateProcessAsync.h"
#include "Helpers.h"
#include "LxssUserSession.h"
#include "LxBus.h"
#include <stdio.h>

#ifndef STATUS_PRIVILEGE_NOT_HELD
#define STATUS_PRIVILEGE_NOT_HELD ((NTSTATUS)0xC0000061L)
#endif

#define LXBUS_SERVER_NAME "minibus"
#define MESSAGE_TO_SEND "Hello from LxBus Server\n\n"

struct PipePair {
    HANDLE Read;
    HANDLE Write;
};

HRESULT
WINAPI
LxBusServer(ILxssUserSession* wslSession,
            GUID* DistroID)
{
    HRESULT hRes;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset = { 0 };
    char Buffer[100];
    HANDLE EventHandle = NULL, ServerHandle = NULL, ClientHandle = NULL;

    // Create a event to sync read/write
    Status = NtCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           TRUE);
    LogStatus(Status, L"NtCreateEvent");


    //
    // 1# Register a LxBus server
    //
    hRes = wslSession->lpVtbl->RegisterLxBusServer(
           wslSession, DistroID, LXBUS_SERVER_NAME, &ServerHandle);

    if (FAILED(hRes))
    {
        Log(hRes, L"RegisterLxBusServer");
        return hRes;
    }

    // Wait for connection from LxBus client infinitely
    LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG WaitMsg = { 0 };
    WaitMsg.Timeout = INFINITE;
    Status = NtDeviceIoControlFile(ServerHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION,
                                   &WaitMsg, sizeof WaitMsg,
                                   &WaitMsg, sizeof WaitMsg);
    ClientHandle = ToHandle(WaitMsg.ClientHandle);
    if (NT_SUCCESS(Status))
    {
        wprintf(L"[+] LxBus ServerHandle: 0x%p ClientHandle: 0x%p\n",
                ServerHandle, ClientHandle);
    }
    else
        LogStatus(Status, L"NtDeviceIoControlFile");


    //
    // 2# Read message from LxBus client
    //
    RtlZeroMemory(Buffer, sizeof Buffer);
    Status = NtReadFile(ClientHandle,
                        EventHandle,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Buffer,
                        sizeof Buffer,
                        &ByteOffset,
                        NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);

    printf("\nMessage from client:\n%s", Buffer);


    //
    // 3# Write message to LxBus client
    //
    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         MESSAGE_TO_SEND,
                         sizeof MESSAGE_TO_SEND,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);


    //
    // 4# Marshal write end of pipe
    //
    struct PipePair pipePairA = { NULL };
    Status = OpenAnonymousPipe(&pipePairA.Read, &pipePairA.Write);

    if (NT_SUCCESS(Status))
    {
        wprintf(L"[+] pipePairA.Read: 0x%p pipePairA.Write: 0x%p\n",
                pipePairA.Read, pipePairA.Write);
    }
    else
        LogStatus(Status, L"OpenAnonymousPipe");

    LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA HandleMsgA = { 0 };
    HandleMsgA.Handle = ToULong(pipePairA.Write);
    HandleMsgA.Type = LxOutputPipeType;

    Status = NtDeviceIoControlFile(ClientHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE,
                                   &HandleMsgA, sizeof HandleMsgA,
                                   &HandleMsgA, sizeof HandleMsgA);
    if (NT_SUCCESS(Status))
    {
        wprintf(L"HandleMsgA.HandleIdCount: %llu\n",
                HandleMsgA.HandleIdCount);
    }
    else
        LogStatus(Status, L"NtDeviceIoControlFile");

    // Write the HandleIdCount so that LxBus client can unmarshal
    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &HandleMsgA.HandleIdCount,
                         sizeof HandleMsgA.HandleIdCount,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);

    // Read message from pipe handle
    RtlZeroMemory(Buffer, sizeof Buffer);
    Status = NtReadFile(pipePairA.Read,
                        EventHandle,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Buffer,
                        sizeof Buffer,
                        &ByteOffset,
                        NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);

    printf("\nMessage from pipe:\n%s", Buffer);


    //
    // 5# Marshal read end of pipe
    //
    struct PipePair pipePairB = { NULL };
    Status = OpenAnonymousPipe(&pipePairB.Read, &pipePairB.Write);
    LogStatus(Status, L"OpenAnonymousPipe");
    if (NT_SUCCESS(Status))
    {
        wprintf(L"[+] pipePairB.Read: 0x%p pipePairB.Write: 0x%p\n",
                pipePairB.Read, pipePairB.Write);
    }
    else
        LogStatus(Status, L"OpenAnonymousPipe");

    LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA HandleMsgB = { 0 };
    HandleMsgB.Handle = ToULong(pipePairB.Read);
    HandleMsgB.Type = LxInputPipeType;

    Status = NtDeviceIoControlFile(ClientHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE,
                                   &HandleMsgB, sizeof HandleMsgB,
                                   &HandleMsgB, sizeof HandleMsgB);
    if (NT_SUCCESS(Status))
    {
        wprintf(L"HandleMsgB.HandleIdCount: %llu\n",
                HandleMsgB.HandleIdCount);
    }
    else
        LogStatus(Status, L"OpenAnonymousPipe");

    // Write the HandleIdCount so that LxBus client can unmarshal
    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &HandleMsgB.HandleIdCount,
                         sizeof HandleMsgB.HandleIdCount,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);

    // Write message to pipe handle
    Status = NtWriteFile(pipePairB.Write,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         MESSAGE_TO_SEND,
                         sizeof MESSAGE_TO_SEND,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);


    //
    // 6# Unmarshal standard I/O file descriptors
    //
    LXBUS_IPC_MESSAGE_MARSHAL_VFSFILE_MSG VfsMsg = { 0 };
    for (int i = 0; i < TOTAL_IO_HANDLES; i++)
    {
        // Read the marshalled fd
        Status = NtReadFile(ClientHandle,
                            EventHandle,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            &VfsMsg.HandleIdCount,
                            sizeof VfsMsg.HandleIdCount,
                            &ByteOffset,
                            NULL);
        if (Status == STATUS_PENDING)
            WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);
        if (NT_SUCCESS(Status))
        {
            wprintf(L"VfsMsg.HandleIdCount: %llu\n",
                    VfsMsg.HandleIdCount);
        }

        // Unmarshal it
        Status = NtDeviceIoControlFile(ClientHandle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE,
                                       &VfsMsg, sizeof VfsMsg,
                                       &VfsMsg, sizeof VfsMsg);
        if (NT_SUCCESS(Status))
        {
            wprintf(L"VfsMsg.Handle: 0x%p\n",
                    VfsMsg.Handle);
        }
    }


    //
    // 7# Unmarshal pid from client side
    //
    LXBUS_IPC_MESSAGE_MARSHAL_PROCESS_MSG ProcessMsg = { 0 };

    // Read ProcessIdCount from client side
    Status = NtReadFile(ClientHandle,
                        EventHandle,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &ProcessMsg.ProcessIdCount,
                        sizeof ProcessMsg.ProcessIdCount,
                        &ByteOffset,
                        NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);
    if (NT_SUCCESS(Status))
        wprintf(L"[+] ProcessMsg: \n\t ProcessIdCount: %llu\n\t", ProcessMsg.ProcessIdCount);

    // Unmarshal it
    Status = NtDeviceIoControlFile(ClientHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_PROCESS,
                                   &ProcessMsg, sizeof ProcessMsg,
                                   &ProcessMsg, sizeof ProcessMsg);
    if (NT_SUCCESS(Status))
        wprintf(L" ProcessHandle: 0x%p\n\t", ProcessMsg.ProcessHandle);
    else
        LogStatus(Status, L"NtDeviceIoControlFile");

    // Get NT side Process ID of LxBus client process
    LXBUS_LX_PROCESS_HANDLE_GET_NT_PID_MSG LxProcMsg = { 0 };

    Status = NtDeviceIoControlFile(ProcessMsg.ProcessHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_LX_PROCESS_HANDLE_GET_NT_PID,
                                   NULL, 0,
                                   &LxProcMsg, sizeof LxProcMsg);
    if (NT_SUCCESS(Status))
        wprintf(L" LxBusClientPID: %u\n", LxProcMsg.NtPid);
    else
        LogStatus(Status, L"NtDeviceIoControlFile");


    //
    // 8# Create a session leader from any process handle
    //
    HANDLE ProcessHandle = NULL;
    Status = NtDuplicateObject(NtCurrentProcess(),
                               NtCurrentProcess(),
                               NtCurrentProcess(),
                               &ProcessHandle,
                               0,
                               OBJ_INHERIT,
                               DUPLICATE_SAME_ACCESS);

    LXBUS_IPC_MESSAGE_MARSHAL_CONSOLE_MSG ConsoleMsg = { 0 };
    ConsoleMsg.Handle = ToULong(ProcessHandle);

    Status = NtDeviceIoControlFile(ClientHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_CONSOLE,
                                   &ConsoleMsg, sizeof ConsoleMsg,
                                   &ConsoleMsg, sizeof ConsoleMsg);
    if (NT_SUCCESS(Status))
    {
        wprintf(L"ConsoleMsg.ConsoleIdCount: %llu\n",
                ConsoleMsg.ConsoleIdCount);
    }
    else
        LogStatus(Status, L"NtDeviceIoControlFile");

    // Write ConsoleIdCount so that LxBus client can unmarshal it
    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &ConsoleMsg.ConsoleIdCount,
                         sizeof ConsoleMsg.ConsoleIdCount,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);


    //
    // 9# Create unnamed LxBus server
    //
    LXBUS_IPC_CONNECTION_CREATE_UNNAMED_SERVER_MSG UnnamedServerMsg = { 0 };

    Status = NtDeviceIoControlFile(ClientHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_CONNECTION_CREATE_UNNAMED_SERVER,
                                   NULL, 0,
                                   &UnnamedServerMsg, sizeof UnnamedServerMsg);
    if (NT_SUCCESS(Status))
    {
        wprintf(L"UnnamedServerMsg.ServerPortIdCount: %llu\n",
                UnnamedServerMsg.ServerPortIdCount);
    }
    else
        LogStatus(Status, L"NtDeviceIoControlFile");

    // Write ServerPortIdCount so that LxBus client can unmarshal it
    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &UnnamedServerMsg.ServerPortIdCount,
                         sizeof UnnamedServerMsg.ServerPortIdCount,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);
    // To be continued ...


    //
    // 10# Marshal fork token
    //
    ULONG Privilege = 3;
    PULONG_PTR ReturnedState = NULL;
    Status = RtlAcquirePrivilege(&Privilege, 1, 0, (PVOID*)&ReturnedState);
    LogStatus(Status, L"RtlAcquirePrivilege");
    if (Status == STATUS_PRIVILEGE_NOT_HELD)
    {
        wprintf(L"Enable \"Replace a process level token\" privilege in Local Security Policy...\n");
        goto Cleanup;
    }

    LXBUS_IPC_CONNECTION_MARSHAL_FORK_TOKEN_MSG TokenMsg = { 0 };
    TokenMsg.Handle = ToULong(*ReturnedState);

    Status = NtDeviceIoControlFile(ClientHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_FORK_TOKEN,
                                   &TokenMsg, sizeof TokenMsg,
                                   &TokenMsg, sizeof TokenMsg);
    if (NT_SUCCESS(Status))
    {
        wprintf(L"TokenMsg.TokenIdCount: %llu\n",
                TokenMsg.TokenIdCount);
    }
    else
        LogStatus(Status, L"NtDeviceIoControlFile");

    // Write TokenIdCount so that LxBus client can unmarshal it
    Status = NtWriteFile(ClientHandle,
                         EventHandle,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &TokenMsg.TokenIdCount,
                         sizeof TokenMsg.TokenIdCount,
                         &ByteOffset,
                         NULL);
    if (Status == STATUS_PENDING)
        WaitForMessage(ClientHandle, EventHandle, &IoStatusBlock);


    Sleep(1000);

Cleanup:
    NtClose(ProcessHandle);
    if (ReturnedState)
        RtlReleasePrivilege(ReturnedState);
    NtClose(pipePairA.Read);
    NtClose(pipePairA.Write);
    NtClose(pipePairB.Read);
    NtClose(pipePairB.Write);
    NtClose(EventHandle);
    NtClose(ServerHandle);
    NtClose(ToHandle(WaitMsg.ClientHandle));
    return Status;
}
