#ifndef CREATEWINPROCESS_H
#define CREATEWINPROCESS_H

#include "WinInternal.h"

typedef struct _LX_CREATE_PROCESS_RESULT {
    PROCESS_INFORMATION ProcInfo;
    ULONG LastError;
    BOOLEAN IsGUISubsystem;
    HPCON hpCon;
} LX_CREATE_PROCESS_RESULT, *PLX_CREATE_PROCESS_RESULT;

NTSTATUS OpenAnonymousPipe(
    PHANDLE ReadPipeHandle,
    PHANDLE WritePipeHandle);

BOOL CreateWinProcess(
    BOOLEAN IsWithoutPipe,
    COORD* ConsoleSize,
    HANDLE hStdInput,
    HANDLE hStdOutput,
    HANDLE hStdError,
    PSTR ApplicationName,
    PSTR CurrentDirectory,
    PLX_CREATE_PROCESS_RESULT ProcResult);

ULONG ProcessInteropMessages(
    HANDLE ReadPipeHandle,
    PLX_CREATE_PROCESS_RESULT ProcResult);

#endif //CREATEWINPROCESS_H
