#ifndef WININTERNAL_H
#define WININTERNAL_H

#include <Windows.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (Status >= 0)
#endif
#define NtCurrentProcess() ((void*)(long long)-1)
#define NtCurrentThread() ((void*)(long long)-2)

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

/*
* structures are copied directly from ntdll.pdb with pdbex tool
* Thanks to Petr Benes aka. wbenny@GitHub
*/

typedef struct _UNICODE_STRING
{
    unsigned short Length;
    unsigned short MaximumLength;
    wchar_t* Buffer;
} UNICODE_STRING, * PUNICODE_STRING; /* size: 0x0010 */

typedef struct _CURDIR
{
    struct _UNICODE_STRING DosPath;
    void* Handle;
} CURDIR, * PCURDIR; /* size: 0x0018 */

typedef struct _STRING
{
    unsigned short Length;
    unsigned short MaximumLength;
    char* Buffer;
} STRING, ANSI_STRING, * PSTRING, * PANSI_STRING; /* size: 0x0010 */

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    unsigned short Flags;
    unsigned short Length;
    unsigned long TimeStamp;
    struct _STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, * PRTL_DRIVE_LETTER_CURDIR; /* size: 0x0018 */

#define RTL_MAX_DRIVE_LETTERS 32

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    unsigned long MaximumLength;
    unsigned long Length;
    unsigned long Flags;
    unsigned long DebugFlags;
    void* ConsoleHandle;
    unsigned long ConsoleFlags;
    void* StandardInput;
    void* StandardOutput;
    void* StandardError;
    struct _CURDIR CurrentDirectory;
    struct _UNICODE_STRING DllPath;
    struct _UNICODE_STRING ImagePathName;
    struct _UNICODE_STRING CommandLine;
    void* Environment;
    unsigned long StartingX;
    unsigned long StartingY;
    unsigned long CountX;
    unsigned long CountY;
    unsigned long CountCharsX;
    unsigned long CountCharsY;
    unsigned long FillAttribute;
    unsigned long WindowFlags;
    unsigned long ShowWindowFlags;
    struct _UNICODE_STRING WindowTitle;
    struct _UNICODE_STRING DesktopInfo;
    struct _UNICODE_STRING ShellInfo;
    struct _UNICODE_STRING RuntimeData;
    struct _RTL_DRIVE_LETTER_CURDIR CurrentDirectores[RTL_MAX_DRIVE_LETTERS];
    unsigned __int64 EnvironmentSize;
    unsigned __int64 EnvironmentVersion;
    void* PackageDependencyData;
    unsigned long ProcessGroupId;
    unsigned long LoaderThreads;
    struct _UNICODE_STRING RedirectionDllName;
    struct _UNICODE_STRING HeapPartitionName;
    unsigned __int64* DefaultThreadpoolCpuSetMasks;
    unsigned long DefaultThreadpoolCpuSetMaskCount;
    long __PADDING__[1];
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS; /* size: 0x0440 */

typedef struct _PEB
{
    unsigned char InheritedAddressSpace;
    unsigned char ReadImageFileExecOptions;
    unsigned char BeingDebugged;
    union
    {
        unsigned char BitField;
        struct /* bitfield */
        {
            unsigned char ImageUsesLargePages : 1; /* bit position: 0 */
            unsigned char IsProtectedProcess : 1; /* bit position: 1 */
            unsigned char IsImageDynamicallyRelocated : 1; /* bit position: 2 */
            unsigned char SkipPatchingUser32Forwarders : 1; /* bit position: 3 */
            unsigned char IsPackagedProcess : 1; /* bit position: 4 */
            unsigned char IsAppContainer : 1; /* bit position: 5 */
            unsigned char IsProtectedProcessLight : 1; /* bit position: 6 */
            unsigned char IsLongPathAwareProcess : 1; /* bit position: 7 */
        }; /* bitfield */
    }; /* size: 0x0001 */
    unsigned char Padding0[4];
    void* Mutant;
    void* ImageBaseAddress;
    struct _PEB_LDR_DATA* Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS* ProcessParameters;
    void* SubSystemData;
    void* ProcessHeap;
    struct _RTL_CRITICAL_SECTION* FastPebLock;
    union _SLIST_HEADER* volatile AtlThunkSListPtr;
    void* IFEOKey;
    union
    {
        unsigned long CrossProcessFlags;
        struct /* bitfield */
        {
            unsigned long ProcessInJob : 1; /* bit position: 0 */
            unsigned long ProcessInitializing : 1; /* bit position: 1 */
            unsigned long ProcessUsingVEH : 1; /* bit position: 2 */
            unsigned long ProcessUsingVCH : 1; /* bit position: 3 */
            unsigned long ProcessUsingFTH : 1; /* bit position: 4 */
            unsigned long ProcessPreviouslyThrottled : 1; /* bit position: 5 */
            unsigned long ProcessCurrentlyThrottled : 1; /* bit position: 6 */
            unsigned long ProcessImagesHotPatched : 1; /* bit position: 7 */
            unsigned long ReservedBits0 : 24; /* bit position: 8 */
        }; /* bitfield */
    }; /* size: 0x0004 */
    unsigned char Padding1[4];
    union
    {
        void* KernelCallbackTable;
        void* UserSharedInfoPtr;
    }; /* size: 0x0008 */
    unsigned long SystemReserved;
    unsigned long AtlThunkSListPtr32;
    void* ApiSetMap;
    unsigned long TlsExpansionCounter;
    unsigned char Padding2[4];
    void* TlsBitmap;
    unsigned long TlsBitmapBits[2];
    void* ReadOnlySharedMemoryBase;
    void* SharedData;
    void** ReadOnlyStaticServerData;
    void* AnsiCodePageData;
    void* OemCodePageData;
    void* UnicodeCaseTableData;
    unsigned long NumberOfProcessors;
    unsigned long NtGlobalFlag;
    union _LARGE_INTEGER CriticalSectionTimeout;
    unsigned __int64 HeapSegmentReserve;
    unsigned __int64 HeapSegmentCommit;
    unsigned __int64 HeapDeCommitTotalFreeThreshold;
    unsigned __int64 HeapDeCommitFreeBlockThreshold;
    unsigned long NumberOfHeaps;
    unsigned long MaximumNumberOfHeaps;
    void** ProcessHeaps;
    void* GdiSharedHandleTable;
    void* ProcessStarterHelper;
    unsigned long GdiDCAttributeList;
    unsigned char Padding3[4];
    struct _RTL_CRITICAL_SECTION* LoaderLock;
    unsigned long OSMajorVersion;
    unsigned long OSMinorVersion;
    unsigned short OSBuildNumber;
    unsigned short OSCSDVersion;
    unsigned long OSPlatformId;
    unsigned long ImageSubsystem;
    unsigned long ImageSubsystemMajorVersion;
    unsigned long ImageSubsystemMinorVersion;
    unsigned char Padding4[4];
    unsigned __int64 ActiveProcessAffinityMask;
    unsigned long GdiHandleBuffer[60];
    void* PostProcessInitRoutine /* function */;
    void* TlsExpansionBitmap;
    unsigned long TlsExpansionBitmapBits[32];
    unsigned long SessionId;
    unsigned char Padding5[4];
    union _ULARGE_INTEGER AppCompatFlags;
    union _ULARGE_INTEGER AppCompatFlagsUser;
    void* pShimData;
    void* AppCompatInfo;
    struct _UNICODE_STRING CSDVersion;
    const struct _ACTIVATION_CONTEXT_DATA* ActivationContextData;
    struct _ASSEMBLY_STORAGE_MAP* ProcessAssemblyStorageMap;
    const struct _ACTIVATION_CONTEXT_DATA* SystemDefaultActivationContextData;
    struct _ASSEMBLY_STORAGE_MAP* SystemAssemblyStorageMap;
    unsigned __int64 MinimumStackCommit;
    void* SparePointers[4];
    unsigned long SpareUlongs[5];
    void* WerRegistrationData;
    void* WerShipAssertPtr;
    void* pUnused;
    void* pImageHeaderHash;
    union
    {
        unsigned long TracingFlags;
        struct /* bitfield */
        {
            unsigned long HeapTracingEnabled : 1; /* bit position: 0 */
            unsigned long CritSecTracingEnabled : 1; /* bit position: 1 */
            unsigned long LibLoaderTracingEnabled : 1; /* bit position: 2 */
            unsigned long SpareTracingBits : 29; /* bit position: 3 */
        }; /* bitfield */
    }; /* size: 0x0004 */
    unsigned char Padding6[4];
    unsigned __int64 CsrServerReadOnlySharedMemoryBase;
    unsigned __int64 TppWorkerpListLock;
    struct _LIST_ENTRY TppWorkerpList;
    void* WaitOnAddressHashTable[128];
    void* TelemetryCoverageHeader;
    unsigned long CloudFileFlags;
    unsigned long CloudFileDiagFlags;
    char PlaceholderCompatibilityMode;
    char PlaceholderCompatibilityModeReserved[7];
    struct _LEAP_SECOND_DATA* LeapSecondData;
    union
    {
        unsigned long LeapSecondFlags;
        struct /* bitfield */
        {
            unsigned long SixtySecondEnabled : 1; /* bit position: 0 */
            unsigned long Reserved : 31; /* bit position: 1 */
        }; /* bitfield */
    }; /* size: 0x0004 */
    unsigned long NtGlobalFlag2;
} PEB, * PPEB; /* size: 0x07c8 */

typedef struct _TEB {
    PVOID Unused[12];
    PPEB ProcessEnvironmentBlock;
    ULONG LastErrorValue;
} TEB, * PTEB;

/* macros with PEB & TEB */
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
