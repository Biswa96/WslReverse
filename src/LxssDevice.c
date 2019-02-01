#include "WinInternal.h"
#include <stdio.h>

void LxssDevice(void)
{
    NTSTATUS Status;
    HANDLE LxssRootHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DestinationString = { 0 };
    OBJECT_ATTRIBUTES ObjectAttributes = { 0 };

    RtlInitUnicodeString(&DestinationString, L"\\Device\\lxss");
    ObjectAttributes.Length = sizeof(ObjectAttributes);
    ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
    ObjectAttributes.ObjectName = &DestinationString;
    Status = ZwOpenFile(&LxssRootHandle,
                        FILE_WRITE_DATA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        0);

    wprintf(L"Status: 0x%lX\n", Status);
}
