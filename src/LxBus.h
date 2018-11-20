#ifndef LXBUS_H
#define LXBUS_H

// Suppress warnings
#pragma warning(push)
#pragma warning(disable:4201 4214)
#pragma pack(push, 1)

#ifndef CTL_CODE
#define FILE_ANY_ACCESS 0
#define METHOD_NEITHER 3
#define FILE_DEVICE_UNKNOWN 0x00000022
#define CTL_CODE( DeviceType, Function, Method, Access ) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

// LxCore!LxpControlDeviceIoctlServerPort
#define IOCTL_ADSS_IPC_SERVER_WAIT_FOR_CONNECTION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0C, METHOD_NEITHER, FILE_ANY_ACCESS) //0x220033u

typedef union _LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG {
    unsigned long Timeout;
    unsigned long ClientHandle;
} LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG, *PLXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG;

// LxCore!LxBuspIpcConnectionMarshalHandle
#define IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x27, METHOD_NEITHER, FILE_ANY_ACCESS) //0x22009Fu

typedef enum _LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_TYPE {
    ConsoleHandleType,
    LxInputPipeType,
    LxOutputPipeType,
    NtInputPipeType,
    NtOutputPipeType
} LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_TYPE;

typedef union _LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_MSG {
    struct {
        unsigned long Handle;
        LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_TYPE Type;
    };
    unsigned long long HandleIdCount;
} LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_MSG;

// LxCore!LxBuspIpcConnectionUnmarshalVfsFile
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2E, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200BBu

typedef union _LXBUS_IPC_VFS_HANDLE {
    struct {
        unsigned long Handle;
        LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_TYPE Type;
    };
    unsigned long long HandleIdCount;
} LXBUS_IPC_VFS_HANDLE, *PLXBUS_IPC_VFS_HANDLE;

#define TOTAL_IO_HANDLES 3

typedef struct _LXSS_MESSAGE_PORT_OBJECT {
    unsigned long NumberOfBytesToRead;
    unsigned long BufferSize;
    LXBUS_IPC_VFS_HANDLE VfsHandle[TOTAL_IO_HANDLES];
    char Unknown[32];
    unsigned long WinApplicationPathOffset;
    unsigned long WinCurrentPathOffset;
    unsigned long WinCommandArgumentOffset;
    unsigned long WinCommandArgumentCount;
    unsigned long WslEnvOffset;
    unsigned short WindowHeight;
    unsigned short WindowWidth;
    unsigned char IsWithoutPipe;
} LXSS_MESSAGE_PORT_OBJECT, *PLXSS_MESSAGE_PORT_OBJECT;

// LxCore!LxpControlDeviceIoctlLxProcess
#define IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x34, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200D3u

typedef struct _LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG {
    unsigned long ExitStatus;
} LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG, *PLXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG;

#pragma pack (pop)
#pragma warning(pop)
#endif //LXBUS_H
