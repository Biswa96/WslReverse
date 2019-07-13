#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "../common/LxBus.h"

#define INFINITE 0xFFFFFFFF
#define LXBUS_SERVER_NAME "minibus"
#define MESSAGE_TO_SEND "Hello from LxBus Client\n\n"

void Log(int result, char* function)
{
    if (result < 0)
    {
        printf("%s error: %d; %s\n",
               function, errno, strerror(errno));
    }
}

int main(void)
{
    int lxfd, res;
    ssize_t bytes;
    char Buffer[100];

    /* 1# open lxss virtual device, root access only */
    lxfd = open("/dev/lxss", O_RDWR);
    Log(lxfd, "open");
    printf("lxfd: %d\n", lxfd);

    // Wait for connection from LxBus server infinitely
    LXBUS_BUS_CLIENT_CONNECT_SERVER_MSG ConnectMsg;
    ConnectMsg.Timeout = INFINITE;
    ConnectMsg.LxBusServerName = LXBUS_SERVER_NAME;
    ConnectMsg.Flags = LXBUS_CONNECT_WAIT_FOR_SERVER_FLAG;
    res = ioctl(lxfd,
                IOCTL_LXBUS_BUS_CLIENT_CONNECT_SERVER,
                &ConnectMsg);
    Log(res, "ioctl");
    printf("ServerFd: %d\n", ConnectMsg.ServerHandle);


    //
    // Set the correct file descriptor mode for access
    //
    res = fcntl(ConnectMsg.ServerHandle, F_SETFD, O_WRONLY);
    Log(res, "fcntl");

    //
    // 2# Write message to LxBus server
    //
    bytes = write(ConnectMsg.ServerHandle,
                  MESSAGE_TO_SEND,
                  sizeof MESSAGE_TO_SEND);
    Log(bytes, "write");


    //
    // 3# Read message from LxBus server
    //
    memset(Buffer, 0, sizeof Buffer);
    bytes = read(ConnectMsg.ServerHandle,
                 Buffer,
                 sizeof Buffer);
    Log(bytes, "read");
    printf("\nMessage from server:\n%s", Buffer);


    //
    // 4# Read the marshalled write end of pipe
    //
    LXBUS_IPC_MESSAGE_UNMARSHAL_HANDLE_DATA HandleMsgA = { 0 };

    bytes = read(ConnectMsg.ServerHandle,
                 &HandleMsgA.HandleIdCount,
                 sizeof HandleMsgA.HandleIdCount);
    Log(bytes, "read");
    printf("HandleMsgA.HandleIdCount: %lld\n", HandleMsgA.HandleIdCount);

    // Unmarshal it
    res = ioctl(ConnectMsg.ServerHandle,
                IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_HANDLE,
                &HandleMsgA);
    Log(res, "ioctl");
    printf("PipeFd: %d\n", HandleMsgA.Handle);

    // Write message to pipe handle
    bytes = write(HandleMsgA.Handle,
                  MESSAGE_TO_SEND,
                  sizeof MESSAGE_TO_SEND);
    Log(bytes, "write");


    //
    // 5# Read the marshalled read end of pipe
    //
    LXBUS_IPC_MESSAGE_UNMARSHAL_HANDLE_DATA HandleMsgB = { 0 };

    bytes = read(ConnectMsg.ServerHandle,
                 &HandleMsgB.HandleIdCount,
                 sizeof HandleMsgB.HandleIdCount);
    Log(bytes, "read");
    printf("HandleMsgB.HandleIdCount: %lld\n", HandleMsgB.HandleIdCount);

    // Unmarshal it
    res = ioctl(ConnectMsg.ServerHandle,
                IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_HANDLE,
                &HandleMsgB);
    Log(res, "ioctl");
    printf("PipeFd: %d\n", HandleMsgB.Handle);

    // Read message from pipe handle
    memset(Buffer, 0, sizeof Buffer);
    bytes = read(HandleMsgB.Handle,
                 Buffer,
                 sizeof Buffer);
    Log(bytes, "read");
    printf("\nMessage from pipe:\n%s", Buffer);


    //
    // 6# Marshal standard I/O file descriptors
    //
    int temp;
    LXBUS_IPC_MESSAGE_MARSHAL_VFSFILE_MSG VfsMsg = { 0 };

    for (VfsMsg.StdFd = 0; VfsMsg.StdFd < TOTAL_IO_HANDLES; VfsMsg.StdFd++)
    {
        printf("VfsMsg.StdFd: %d\n", VfsMsg.StdFd);
        temp = VfsMsg.StdFd;

        res = ioctl(ConnectMsg.ServerHandle,
                    IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_VFS_FILE,
                    &VfsMsg.StdFd);
        Log(res, "ioctl");
        printf("VfsMsg.HandleIdCount: %lld\n", VfsMsg.HandleIdCount);

        // Write HandleIdCount so that LxBus server can unmarshal it
        bytes = write(ConnectMsg.ServerHandle,
                      &VfsMsg.HandleIdCount,
                      sizeof VfsMsg.HandleIdCount);
        Log(bytes, "write");

        // Swap
        VfsMsg.HandleIdCount = 0;
        VfsMsg.StdFd = temp;
    }


    //
    // 7# Marshal current pid
    //
    LXBUS_IPC_MESSAGE_MARSHAL_PROCESS_MSG ProcessMsg = { 0 };
    ProcessMsg.ProcessId = getpid();

    res = ioctl(ConnectMsg.ServerHandle,
                IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_PROCESS,
                &ProcessMsg);
    Log(res, "ioctl");
    printf("[+] ProcessMsg: \n\t ProcessIdCount: %lld \n\t LxBusClientPID: %d\n",
           ProcessMsg.ProcessIdCount, getpid());

    // Write ProcessIdCount so that LxBus server can unmarshal it
    bytes = write(ConnectMsg.ServerHandle,
                  &ProcessMsg.ProcessIdCount,
                  sizeof ProcessMsg.ProcessIdCount);
    Log(bytes, "write");


    //
    // 8# Create a controlling terminal from unmarshalled handle
    //
    LXBUS_IPC_MESSAGE_UNMARSHAL_CONSOLE_MSG ConsoleMsg = { 0 };

    // Read ConsoleIdCount from server side
    bytes = read(ConnectMsg.ServerHandle,
                 &ConsoleMsg.ConsoleIdCount,
                 sizeof ConsoleMsg.ConsoleIdCount);
    Log(bytes, "read");
    printf("ConsoleMsg.ConsoleIdCount: %lld\n", ConsoleMsg.ConsoleIdCount);

    // Unmarshal it
    res = ioctl(ConnectMsg.ServerHandle,
                IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_CONSOLE,
                &ConsoleMsg);
    Log(res, "ioctl");
    printf("ConsoleFd: %d\n", ConsoleMsg.Handle);
    // res = ioctl(ConsoleMsg.Handle, TIOCSCTTY, NULL);


    //
    // 9# Connect to unnamed LxBus server
    //
    LXBUS_IPC_CONNECTION_CREATE_UNNAMED_SERVER_MSG UnnamedServerMsg = { 0 };

    // Read ServerPortIdCount from server side
    bytes = read(ConnectMsg.ServerHandle,
                 &UnnamedServerMsg.ServerPortIdCount,
                 sizeof UnnamedServerMsg.ServerPortIdCount);
    Log(bytes, "read");
    printf("UnnamedServerMsg.ServerPortIdCount: %lld\n", UnnamedServerMsg.ServerPortIdCount);
    // To be continued ...


    //
    // 10# Unmarshal fork token
    //
    LXBUS_IPC_CONNECTION_UNMARSHAL_FORK_TOKEN_MSG TokenMsg = { 0 };

    // Read TokenIdCount from server side
    bytes = read(ConnectMsg.ServerHandle,
                 &TokenMsg.TokenIdCount,
                 sizeof TokenMsg.TokenIdCount);
    Log(bytes, "read");
    printf("TokenMsg.TokenIdCount: %lld\n", TokenMsg.TokenIdCount);

    // Unmarshal it
    res = ioctl(ConnectMsg.ServerHandle,
                IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_FORK_TOKEN,
                &TokenMsg);
    Log(res, "ioctl");
    printf("TokenFd: %d\n", TokenMsg.Handle);


    // Cleanup
    close(lxfd);
    close(ConnectMsg.ServerHandle);
}
