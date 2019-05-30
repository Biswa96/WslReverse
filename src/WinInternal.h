#ifndef WININTERNAL_H
#define WININTERNAL_H

#include <Windows.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (Status >= 0)
#endif
#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)

// From processhacker /phlib/include/phsup.h
#define TICKS_PER_NS ((long long)1 * 10)
#define TICKS_PER_MS (TICKS_PER_NS * 1000)
#define TICKS_PER_SEC (TICKS_PER_MS * 1000)
#define TICKS_PER_MIN (TICKS_PER_SEC * 60)
#define PAGE_SIZE 0x1000

// From processhacker /phnt/include/ntioapi.h
// NamedPipeConfiguration for NtQueryInformationFile
#define FILE_PIPE_INBOUND 0
#define FILE_PIPE_OUTBOUND 1
#define FILE_PIPE_FULL_DUPLEX 2

// NamedPipeType for NtCreateNamedPipeFile
#define FILE_PIPE_BYTE_STREAM_TYPE 0
#define FILE_PIPE_MESSAGE_TYPE 1
#define FILE_PIPE_ACCEPT_REMOTE_CLIENTS 0
#define FILE_PIPE_REJECT_REMOTE_CLIENTS 2
#define FILE_PIPE_TYPE_VALID_MASK 3

// ReadMode for NtCreateNamedPipeFile
#define FILE_PIPE_BYTE_STREAM_MODE 0
#define FILE_PIPE_MESSAGE_MODE 1

// CompletionMode for NtCreateNamedPipeFile
#define FILE_PIPE_QUEUE_OPERATION 0
#define FILE_PIPE_COMPLETE_OPERATION 1

// Flags from winternl.h
#define OBJ_INHERIT 2
#define OBJ_CASE_INSENSITIVE 64
#ifndef __MINGW32__
#define FILE_CREATE 2
#define FILE_SYNCHRONOUS_IO_NONALERT 32
#define FILE_NON_DIRECTORY_FILE 64
#endif

// Some handmade
#define ToULong(x) (unsigned long)(unsigned long long)(x)
#define ToHandle(x) (void*)(unsigned long long)(x)

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, ANSI_STRING, *PSTRING, *PANSI_STRING;

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _CURDIR {
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR {
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#define RTL_MAX_DRIVE_LETTERS 32

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;
    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];
    SIZE_T EnvironmentSize;
    SIZE_T EnvironmentVersion;
    PVOID PackageDependencyData;
    ULONG ProcessGroupId;
    ULONG LoaderThreads;
    UNICODE_STRING RedirectionDllName;
    UNICODE_STRING HeapPartitionName;
    PVOID DefaultThreadpoolCpuSetMasks;
    ULONG DefaultThreadpoolCpuSetMaskCount;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    union {
        UCHAR BitField;
        struct {
            UCHAR ImageUsesLargePages : 1;
            UCHAR IsProtectedProcess : 1;
            UCHAR IsImageDynamicallyRelocated : 1;
            UCHAR SkipPatchingUser32Forwarders : 1;
            UCHAR IsPackagedProcess : 1;
            UCHAR IsAppContainer : 1;
            UCHAR IsProtectedProcessLight : 1;
            UCHAR IsLongPathAwareProcess : 1;
        };
    };
    HANDLE Mutant;
    HMODULE ImageBaseAddress;
    PVOID Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID SubSystemData;
    HANDLE ProcessHeap;
    PRTL_CRITICAL_SECTION FastPebLock;
    PSLIST_HEADER AtlThunkSListPtr;
    PVOID IFEOKey;
    union {
        ULONG CrossProcessFlags;
        struct {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ProcessPreviouslyThrottled : 1;
            ULONG ProcessCurrentlyThrottled : 1;
            ULONG ProcessImagesHotPatched : 1;
            ULONG ReservedBits0 : 24;
        };
    };
    union {
        PVOID KernelCallbackTable;
        PVOID UserSharedInfoPtr;
    };
    ULONG SystemReserved;
    PSLIST_HEADER AtlThunkSListPtr32;
    PVOID ApiSetMap;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[2];
    PVOID ReadOnlySharedMemoryBase;
    PVOID SharedData;
    PVOID ReadOnlyStaticServerData;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;
    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;
    LARGE_INTEGER CriticalSectionTimeout;
    ULONG_PTR HeapSegmentReserve;
    ULONG_PTR HeapSegmentCommit;
    ULONG_PTR HeapDeCommitTotalFreeThreshold;
    ULONG_PTR HeapDeCommitFreeBlockThreshold;
    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PVOID ProcessHeaps;
    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    ULONG GdiDCAttributeList;
    PVOID LoaderLock;
    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG_PTR ActiveProcessAffinityMask;
    ULONG GdiHandleBuffer[60];
    void(__stdcall *PostProcessInitRoutine)(void);
    PVOID TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];
    ULONG SessionId;
    LARGE_INTEGER AppCompatFlags;
    LARGE_INTEGER AppCompatFlagsUser;
    PVOID pShimData;
    PVOID AppCompatInfo;
    UNICODE_STRING CSDVersion;
    PVOID ActivationContextData;
    PVOID ProcessAssemblyStorageMap;
    PVOID SystemDefaultActivationContextData;
    PVOID SystemAssemblyStorageMap;
    ULONG_PTR MinimumStackCommit;
    PVOID SparePointers[4];
    ULONG SpareUlongs[5];
    PVOID WerRegistrationData;
    PVOID WerShipAssertPtr;
    PVOID pUnused;
    PVOID pImageHeaderHash;
    union {
        ULONG TracingFlags;
        struct {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        };
    };
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
    ULONG_PTR TppWorkerpListLock;
    LIST_ENTRY TppWorkerpList;
    PVOID WaitOnAddressHashTable[128];
    PVOID TelemetryCoverageHeader;
    ULONG CloudFileFlags;
    ULONG CloudFileDiagFlags;
    UCHAR PlaceholderCompatibilityMode;
    UCHAR PlaceholderCompatibilityModeReserved[7];
    PVOID LeapSecondData;
    union {
        ULONG LeapSecondFlags;
        struct {
            ULONG SixtySecondEnabled : 1;
            ULONG Reserved : 31;
        };
    };
    ULONG NtGlobalFlag2;
} PEB, *PPEB;

typedef struct _TEB {
    PVOID Unused[12];
    PPEB ProcessEnvironmentBlock;
    ULONG LastErrorValue;
} TEB, *PTEB;

// macros with PEB & TEB
#define RtlGetCommandLineW() NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CommandLine.Buffer
#define RtlGetLastWin32Error() NtCurrentTeb()->LastErrorValue
#define RtlGetProcessHeap() NtCurrentTeb()->ProcessEnvironmentBlock->ProcessHeap
#define NtCurrentPeb() NtCurrentTeb()->ProcessEnvironmentBlock
#define GetUserProcessParameter() NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID
(NTAPI* PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved);

typedef struct _FILE_FS_DEVICE_INFORMATION {
    ULONG DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsDriverPathInformation,
    FileFsVolumeFlagsInformation,
    FileFsSectorSizeInformation,
    FileFsDataCopyInformation,
    FileFsMetadataSizeInformation,
    FileFsFullSizeInformationEx,
    FileFsMaximumInformation
} FSINFOCLASS, *PFSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    ULONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0
} PROCESSINFOCLASS;

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

typedef enum _WAIT_TYPE {
    WaitAll,
    WaitAny,
    WaitNotification
} WAIT_TYPE;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelIoFileEx(
    _In_ HANDLE FileHandle,
    _In_opt_ PIO_STATUS_BLOCK IoRequestToCancel,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock);

NTSTATUS
NTAPI
NtClose(
    _In_ HANDLE Handle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateNamedPipeFile(
    _Out_ PHANDLE FileHandle,
    _In_ ULONG DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_ ULONG NamedPipeType,
    _In_ ULONG ReadMode,
    _In_ ULONG CompletionMode,
    _In_ ULONG MaximumInstances,
    _In_ ULONG InboundQuota,
    _In_ ULONG OutboundQuota,
    _In_opt_ PLARGE_INTEGER DefaultTimeout);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateObject(
    _In_ HANDLE SourceProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_opt_ HANDLE TargetProcessHandle,
    _Out_opt_ PHANDLE TargetHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Options);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG OpenOptions);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FSINFOCLASS FsInformationClass);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _Out_writes_bytes_(BufferSize) PVOID Buffer,
    _In_ SIZE_T BufferSize,
    _Out_opt_ PSIZE_T NumberOfBytesRead);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEvent(
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects(
    _In_ ULONG Count,
    _In_reads_(Count) HANDLE Handles[],
    _In_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForSingleObject(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFile(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE Event,
    _In_opt_ PIO_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcContext,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_opt_ PULONG Key);

NTSYSAPI
NTSTATUS
NTAPI
RtlAcquirePrivilege(
    _In_ PULONG Privilege,
    _In_ ULONG NumPriv,
    _In_ ULONG Flags,
    _Out_ PVOID* ReturnedState);

NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size);

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PANSI_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString);

NTSYSAPI
VOID
NTAPI
RtlDeleteCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection);

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ PVOID BaseAddress);

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    _In_ PUNICODE_STRING UnicodeString);

NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
    _In_ PUNICODE_STRING GuidString,
    _Out_ GUID* Guid);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ PSTR SourceString);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx(
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount,
    _In_ DWORD Flags);

NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_ PWSTR SourceString);

NTSYSAPI
VOID
NTAPI
RtlReleasePrivilege(
    _In_ PVOID StatePointer);

NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
    _In_ GUID* Guid,
    _Out_ PUNICODE_STRING GuidString);

#undef RtlZeroMemory
NTSYSAPI
VOID
NTAPI
RtlZeroMemory(
    _In_ PVOID Destination,
    _In_ SIZE_T Length);

NTSYSAPI
NTSTATUS
NTAPI
TpAllocWork(
    _Out_ PTP_WORK* WorkReturn,
    _In_ PTP_WORK_CALLBACK Callback,
    _Inout_opt_ PVOID Context,
    _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron);

NTSYSAPI
VOID
NTAPI
TpPostWork(
    _Inout_ PTP_WORK Work);

NTSYSAPI
VOID
NTAPI
TpReleaseWork(
    _Inout_ PTP_WORK Work);

NTSYSAPI
VOID
NTAPI
TpWaitForWork(
    _Inout_ PTP_WORK Work,
    _In_ BOOLEAN CancelPendingCallbacks);

#endif // WININTERNAL_H
