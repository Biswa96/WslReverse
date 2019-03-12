#include "WinInternal.h"
#include <stdio.h>

void LxssDevice(void)
{
    NTSTATUS Status;
    HANDLE LxssRootHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING ObjectName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlZeroMemory(&ObjectName, sizeof ObjectName);
    Status = RtlInitUnicodeStringEx(&ObjectName, L"\\Device\\lxss");

    RtlZeroMemory(&ObjectAttributes, sizeof ObjectAttributes);
    ObjectAttributes.Length = sizeof(ObjectAttributes);
    ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
    ObjectAttributes.ObjectName = &ObjectName;

    Status = ZwOpenFile(&LxssRootHandle,
                        FILE_WRITE_DATA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        0);

    wprintf(L"Status: 0x%lX\n", Status);
}
