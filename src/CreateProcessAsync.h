#ifndef CREATEPROCESSASYNC_H
#define CREATEPROCESSASYNC_H

typedef long NTSTATUS;
typedef void* HANDLE;
typedef HANDLE *PHANDLE;
typedef struct _IO_STATUS_BLOCK *PIO_STATUS_BLOCK;
typedef struct _TP_WORK *PTP_WORK;
typedef struct _TP_CALLBACK_INSTANCE *PTP_CALLBACK_INSTANCE;

NTSTATUS
WaitForMessage(HANDLE ClientHandle,
               HANDLE EventHandle,
               PIO_STATUS_BLOCK IoStatusBlock);

NTSTATUS
OpenAnonymousPipe(PHANDLE ReadPipeHandle,
                  PHANDLE WritePipeHandle);

void
CreateProcessAsync(PTP_CALLBACK_INSTANCE Instance,
                   HANDLE ClientHandle,
                   PTP_WORK Work);

#endif //CREATEPROCESSASYNC_H
