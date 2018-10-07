#include <Windows.h>
#include <winternl.h>
#include <stdio.h>

#ifndef CTL_CODE
#define FILE_ANY_ACCESS 0
#define FILE_DEVICE_CONSOLE 0x00000050
#define METHOD_NEITHER 3
#define CTL_CODE( DeviceType, Function, Method, Access ) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

//CdpConnectionFastIoctl
#define IOCTL_CDP_FAST_CONNECTION \
    CTL_CODE(FILE_DEVICE_CONSOLE, 0x08, METHOD_NEITHER, FILE_ANY_ACCESS) //0x500023u

#define FileFsDeviceInformation 4

#ifdef _MSC_VER

typedef struct _FILE_FS_DEVICE_INFORMATION {
    ULONG DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

NTSTATUS NTAPI NtQueryVolumeInformationFile(
    HANDLE            FileHandle,
    PIO_STATUS_BLOCK  IoStatusBlock,
    PVOID             FsInformation,
    ULONG             Length,
    ULONG             FsInformationClass);

#endif

void ConsolePid(HANDLE ConsoleHandle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION FsInformation;
    ULONG64 ConHostPid;
    NTSTATUS Status;

    Status = NtQueryVolumeInformationFile(
        ConsoleHandle,
        &IoStatusBlock,
        &FsInformation,
        sizeof(FILE_FS_DEVICE_INFORMATION),
        FileFsDeviceInformation);

    // FsInformation.Characteristics == FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL
    if (FsInformation.DeviceType == FILE_DEVICE_CONSOLE)
    {
        Status = NtDeviceIoControlFile(
            ConsoleHandle,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            IOCTL_CDP_FAST_CONNECTION,
            NULL, 0,
            &ConHostPid, sizeof(ULONG64));

        if (Status >= 0)
        {
            wprintf(
                L"ConHost PID: %lld Handle: %ld\n",
                ConHostPid,
                HandleToULong(ConsoleHandle));
        } 
    }
}
