#ifndef CREATEPROCESSASYNC_H
#define CREATEPROCESSASYNC_H

NTSTATUS
WINAPI
WaitForMessage(HANDLE ClientHandle,
               HANDLE EventHandle,
               PIO_STATUS_BLOCK IoRequestToCancel);

NTSTATUS
WINAPI
OpenAnonymousPipe(PHANDLE ReadPipeHandle,
                  PHANDLE WritePipeHandle);

void
WINAPI
CreateProcessAsync(PTP_CALLBACK_INSTANCE Instance,
                   HANDLE ClientHandle,
                   PTP_WORK Work);

#endif //CREATEPROCESSASYNC_H
