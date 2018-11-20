#include "CreateWinProcess.h" // include WinInternal.h
#include "Functions.h"

#ifndef PSEUDOCONSOLE_INHERIT_CURSOR
#define PSEUDOCONSOLE_INHERIT_CURSOR 0x1
#endif

NTSTATUS OpenAnonymousPipe(
    PHANDLE ReadPipeHandle,
    PHANDLE WritePipeHandle)
{
    HANDLE hNamedPipeFile = NULL, hFile = NULL;
    LARGE_INTEGER DefaultTimeOut;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING ObjName = { 0 };
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };

    HANDLE hPipeServer = CreateFileW(
        L"\\\\.\\pipe\\",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipeServer == INVALID_HANDLE_VALUE)
        Log(GetLastError(), L"CreateFileW");

    DefaultTimeOut.QuadPart = -2 * TICKS_PER_MIN;
    ObjectAttributes.ObjectName = &ObjName;
    ObjectAttributes.RootDirectory = hPipeServer;
    ObjectAttributes.Length = sizeof(ObjectAttributes);

    NTSTATUS Status = NtCreateNamedPipeFile(
        &hNamedPipeFile,
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
    Log(Status, L"NtCreateNamedPipeFile");

    ObjectAttributes.Length = sizeof(ObjectAttributes);
    ObjectAttributes.RootDirectory = hNamedPipeFile;
    ObjectAttributes.ObjectName = &ObjName;
    Status = NtOpenFile(
        &hFile,
        GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
        &ObjectAttributes,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_NON_DIRECTORY_FILE);
    Log(Status, L"NtOpenFile");

    // return handles to caller
    *ReadPipeHandle = hNamedPipeFile;
    *WritePipeHandle = hFile;
    NtClose(hPipeServer);
    return Status;
}

BOOL CreateWinProcess(
    BOOLEAN IsWithoutPipe,
    COORD* ConsoleSize,
    HANDLE hStdInput,
    HANDLE hStdOutput,
    HANDLE hStdError,
    PSTR ApplicationName,
    PSTR CurrentDirectory,
    PLX_CREATE_PROCESS_RESULT ProcResult)
{
    BOOL bRes;
    HRESULT hRes;
    HPCON hpCon;
    SIZE_T AttrSize, ReadSize;
    STARTUPINFOEXA SInfoEx = { 0 }; // Must set all members to Zero
    PROCESS_BASIC_INFORMATION BasicInfo;
    PEB64 Peb;

    InitializeProcThreadAttributeList(NULL, 1, 0, &AttrSize);
    LPPROC_THREAD_ATTRIBUTE_LIST AttrList = malloc(AttrSize);
    InitializeProcThreadAttributeList(AttrList, 1, 0, &AttrSize);

    if (IsWithoutPipe)
    {
        hRes = CreatePseudoConsole(
            *ConsoleSize,
            hStdInput,
            hStdOutput, PSEUDOCONSOLE_INHERIT_CURSOR, &hpCon);

        // Return hpCon to caller
        ProcResult->hpCon = hpCon;
        Log(hRes, L"CreatePseudoConsole");

        bRes = UpdateProcThreadAttribute(
            AttrList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, //0x20016u
            hpCon, sizeof(hpCon), NULL, NULL);
    }
    else
    {
        SInfoEx.StartupInfo.hStdInput = hStdInput;
        SInfoEx.StartupInfo.hStdOutput = hStdOutput;
        SInfoEx.StartupInfo.hStdError = hStdError;

        HANDLE Value[3] = { NULL };
        Value[0] = hStdInput;
        Value[1] = hStdOutput;
        Value[2] = hStdError;
        bRes = UpdateProcThreadAttribute(
            AttrList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, //0x20002u
            Value, sizeof(Value), NULL, NULL);
    }

    SInfoEx.StartupInfo.cb = sizeof(SInfoEx);
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    SInfoEx.StartupInfo.lpDesktop = "winsta0\\default";
    SInfoEx.lpAttributeList = AttrList;

    bRes = CreateProcessA(
        ApplicationName,
        NULL,
        NULL,
        NULL,
        TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
        NULL,
        CurrentDirectory,
        &SInfoEx.StartupInfo,
        &ProcResult->ProcInfo);

    wprintf(
        L"[*] CreateWinProcess hProcess: %lu hThread: %lu dwProcessId: %lu\n",
        ToULong(ProcResult->ProcInfo.hProcess),
        ToULong(ProcResult->ProcInfo.hThread),
        ProcResult->ProcInfo.dwProcessId);

    NTSTATUS Status = NtQueryInformationProcess(
        ProcResult->ProcInfo.hProcess,
        ProcessBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        NULL);

    if (NT_SUCCESS(Status))
    {
        ReadProcessMemory(
            ProcResult->ProcInfo.hProcess,
            BasicInfo.PebBaseAddress,
            &Peb,
            sizeof(PEB64),
            &ReadSize);

        // From IMAGE_OPTIONAL_HEADER structure
        if (Peb.ImageSubsystemMinorVersion == IMAGE_SUBSYSTEM_WINDOWS_GUI)
            ProcResult->IsGUISubsystem = TRUE;
    }

    free(AttrList);
    return bRes;
}

ULONG ProcessInteropMessages(
    HANDLE ReadPipeHandle,
    PLX_CREATE_PROCESS_RESULT ProcResult)
{
    UNREFERENCED_PARAMETER(ReadPipeHandle);
    UNREFERENCED_PARAMETER(ProcResult);

    DWORD ExitCode = 1;
    return ExitCode;
}
