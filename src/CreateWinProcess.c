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
    UNICODE_STRING PipeObj = { 0 };
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
        Log(GetLastError(), L"CreateFileW pipe");

    DefaultTimeOut.QuadPart = -2 * TICKS_PER_MIN;
    ObjectAttributes.ObjectName = &PipeObj;
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
    ObjectAttributes.ObjectName = &PipeObj;

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
    PLXSS_MESSAGE_PORT_RECEIVE_OBJECT LxReceiveMsg,
    PLX_CREATE_PROCESS_RESULT ProcResult)
{
    BOOL bRes;
    HPCON hpCon = NULL;
    SIZE_T AttrSize;
    STARTUPINFOEXW SInfoEx = { 0 }; // Must set all members to Zero
    PROCESS_BASIC_INFORMATION BasicInfo;
    PEB64 Peb;

    InitializeProcThreadAttributeList(NULL, 1, 0, &AttrSize);
    LPPROC_THREAD_ATTRIBUTE_LIST AttrList = malloc(AttrSize);
    InitializeProcThreadAttributeList(AttrList, 1, 0, &AttrSize);

    if (LxReceiveMsg->IsWithoutPipe)
    {
        COORD ConsoleSize;
        ConsoleSize.X = LxReceiveMsg->WindowWidth;
        ConsoleSize.Y = LxReceiveMsg->WindowHeight;

        HRESULT hRes = CreatePseudoConsole(
            ConsoleSize,
            ToHandle(LxReceiveMsg->VfsHandle[0].Handle),
            ToHandle(LxReceiveMsg->VfsHandle[1].Handle),
            PSEUDOCONSOLE_INHERIT_CURSOR,
            &hpCon);

        // Return hpCon to caller
        ProcResult->hpCon = hpCon;
        Log(hRes, L"CreatePseudoConsole");

        bRes = UpdateProcThreadAttribute(
            AttrList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, //0x20016u
            hpCon, sizeof(hpCon), NULL, NULL);
    }
    else
    {
        SInfoEx.StartupInfo.hStdInput = ToHandle(LxReceiveMsg->VfsHandle[0].Handle);
        SInfoEx.StartupInfo.hStdOutput = ToHandle(LxReceiveMsg->VfsHandle[1].Handle);
        SInfoEx.StartupInfo.hStdError = ToHandle(LxReceiveMsg->VfsHandle[2].Handle);

        HANDLE Value[3] = { NULL };
        Value[0] = ToHandle(LxReceiveMsg->VfsHandle[0].Handle);
        Value[1] = ToHandle(LxReceiveMsg->VfsHandle[1].Handle);
        Value[2] = ToHandle(LxReceiveMsg->VfsHandle[2].Handle);

        bRes = UpdateProcThreadAttribute(
            AttrList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, //0x20002u
            Value, sizeof(Value), NULL, NULL);
    }

    SInfoEx.StartupInfo.cb = sizeof(SInfoEx);
    SInfoEx.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    SInfoEx.StartupInfo.lpDesktop = L"winsta0\\default";
    SInfoEx.lpAttributeList = AttrList;

    WCHAR w_ApplicationName[MAX_PATH], w_CurrentDirectory[MAX_PATH];
    PSTR ApplicationName = LxReceiveMsg->Unknown + LxReceiveMsg->WinApplicationPathOffset;
    PSTR CurrentDirectory = LxReceiveMsg->Unknown + LxReceiveMsg->WinCurrentPathOffset;

    mbstowcs_s(NULL, w_ApplicationName, MAX_PATH, ApplicationName, MAX_PATH);
    mbstowcs_s(NULL, w_CurrentDirectory, MAX_PATH, CurrentDirectory, MAX_PATH);

    bRes = CreateProcessW(
        w_ApplicationName,
        NULL,
        NULL,
        NULL,
        TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
        NULL,
        w_CurrentDirectory,
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
            NULL);

        // From IMAGE_OPTIONAL_HEADER structure
        if (Peb.ImageSubsystemMinorVersion == IMAGE_SUBSYSTEM_WINDOWS_GUI)
            ProcResult->IsSubsystemGUI |= RESTORE_CONSOLE_STATE_MODE;
    }

    // Set lasterror always success intentionally
    ProcResult->LastError = ERROR_SUCCESS;

    free(AttrList);
    return bRes;
}
