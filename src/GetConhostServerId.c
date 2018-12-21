#include "WinInternal.h"
#include <stdio.h>

#ifndef FILE_DEVICE_CONSOLE
#define FILE_DEVICE_CONSOLE 0x00000050
#endif

// ConDrv!CdpConnectionFastIoctl
#define IOCTL_CDP_FAST_CONNECTION \
    CTL_CODE(FILE_DEVICE_CONSOLE, 0x08, METHOD_NEITHER, FILE_ANY_ACCESS) //0x500023u

unsigned long long GetConhostServerId(
    void* ConsoleHandle)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION FsInformation;
    unsigned long long ConHostPid = 0;
    NTSTATUS Status;

    Status = ZwQueryVolumeInformationFile(
        ConsoleHandle,
        &IoStatusBlock,
        &FsInformation,
        sizeof(FsInformation),
        FileFsDeviceInformation);

    // FsInformation.Characteristics == FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL
    if (FsInformation.DeviceType == FILE_DEVICE_CONSOLE)
    {
        Status = ZwDeviceIoControlFile(
            ConsoleHandle,
            NULL,
            NULL,
            NULL,
            &IoStatusBlock,
            IOCTL_CDP_FAST_CONNECTION,
            NULL, 0,
            &ConHostPid, sizeof(ConHostPid));

        if (Status >= 0)
            return ConHostPid;
    }

    return 0;
}
