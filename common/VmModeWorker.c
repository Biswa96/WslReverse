#include "WinInternal.h"
#include "Helpers.h"
#include "LxssUserSession.h"
#include "SpawnWslHost.h"
#include <stdio.h>

struct LX_RELAY_WORKER_THREAD_CONTEXT {
    union {
        HANDLE hRead;
        SOCKET sRead;
    };
    union {
        HANDLE hWrite;
        SOCKET sWrite;
    };
};

#define BUFF_SIZE 400

DWORD
WINAPI
WorkerThread(PVOID Parameter)
{
    char szBuffer[BUFF_SIZE];
    DWORD nbWritten, nbRead;
    struct LX_RELAY_WORKER_THREAD_CONTEXT* Context = Parameter;

    while (ReadFile(Context->hRead, szBuffer, BUFF_SIZE, &nbRead, NULL))
    {
        if (nbRead == 0)
            break;
        WriteFile(Context->hWrite, szBuffer, nbRead, &nbWritten, NULL);
    }

    return 0;
}

#define VMBUS_MESSAGE_MODE_RECEIVED 5
#define VMBUS_MESSAGE_MODE_EXIT 6

struct VMBUS_MESSAGE_PORT_RECEIVE_OBJECT {
    int MessageMode;
    int MessageSize;
};

void
WINAPI
VmModeWorker(SOCKET SockIn,
             SOCKET SockOut,
             SOCKET SockErr,
             SOCKET ServerSocket,
             PLXSS_STD_HANDLES StdHandles)
{
    struct LX_RELAY_WORKER_THREAD_CONTEXT wtcIn, wtcOut, wtcErr;
    HANDLE hRead = NULL, hWrite = NULL;
    HANDLE hHeap = GetProcessHeap();

    /* read from stdin, write to sockin */
    hRead = GetStdHandle(STD_INPUT_HANDLE);
    if (StdHandles->StdIn.Handle)
        hRead = ToHandle(StdHandles->StdIn.Handle);

    wtcIn.hRead = hRead;
    wtcIn.sWrite = SockIn;
    CreateThread(NULL, 0, WorkerThread, &wtcIn, 0, NULL);

    /* read from sockout, write to stdout */
    hWrite = GetStdHandle(STD_OUTPUT_HANDLE);
    if (StdHandles->StdOut.Handle)
        hWrite = ToHandle(StdHandles->StdOut.Handle);

    wtcOut.sRead = SockOut;
    wtcOut.hWrite = hWrite;
    CreateThread(NULL, 0, WorkerThread, &wtcOut, 0, NULL);

    /* read from sockerr, write to stderr */
    hWrite = GetStdHandle(STD_ERROR_HANDLE);
    if (StdHandles->StdErr.Handle)
        hWrite = ToHandle(StdHandles->StdErr.Handle);

    wtcErr.sRead = SockErr;
    wtcErr.hWrite = hWrite;
    CreateThread(NULL, 0, WorkerThread, &wtcErr, 0, NULL);

    int ret;
    struct VMBUS_MESSAGE_PORT_RECEIVE_OBJECT RecvMsg;

    while (1)
    {
        ret = recv(ServerSocket, (char*) &RecvMsg, sizeof RecvMsg, 0);
        if (ret < (int) sizeof RecvMsg)
            break;

        if (RecvMsg.MessageMode == VMBUS_MESSAGE_MODE_RECEIVED)
        {
            continue;
        }
        else if (RecvMsg.MessageMode == VMBUS_MESSAGE_MODE_EXIT)
        {
            void* ExitCode;
            int size = RecvMsg.MessageSize - sizeof RecvMsg;
            ExitCode = RtlAllocateHeap(hHeap, 0, size);

            ret = recv(ServerSocket, (char*) ExitCode, size, 0);
            ret = send(ServerSocket, (const char*) &RecvMsg, RecvMsg.MessageSize, 0);
            RtlFreeHeap(hHeap, 0, ExitCode);
            break;
        }
        else
            break;
    }

    return;
}
