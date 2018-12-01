#ifndef LXBUS_H
#define LXBUS_H

#ifndef CTL_CODE
#define FILE_ANY_ACCESS 0
#define METHOD_NEITHER 3
#define FILE_DEVICE_UNKNOWN 0x22
#define CTL_CODE( DeviceType, Function, Method, Access ) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

// LxCore!LxpControlDeviceIoctlServerPort
#define IOCTL_ADSS_IPC_SERVER_WAIT_FOR_CONNECTION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0C, METHOD_NEITHER, FILE_ANY_ACCESS) //0x220033u

typedef union _LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG {
    unsigned int Timeout;
    unsigned int ClientHandle;
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

typedef union _LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA {
    struct {
        unsigned int Handle;
        LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_TYPE Type;
    };
    unsigned long long HandleIdCount;
} LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA, *PLXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA;

// LxCore!LxBuspIpcConnectionUnmarshalVfsFile
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2E, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200BBu

typedef union _LXBUS_IPC_VFS_HANDLE {
    struct {
        unsigned int Handle;
        LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_TYPE Type;
    };
    unsigned long long HandleIdCount;
} LXBUS_IPC_VFS_HANDLE, *PLXBUS_IPC_VFS_HANDLE;

#define TOTAL_IO_HANDLES 3

#pragma pack(push, 1)
typedef struct _LXSS_MESSAGE_PORT_RECEIVE_OBJECT {
    unsigned int NumberOfBytesToRead;
    unsigned int BufferSize;
    LXBUS_IPC_VFS_HANDLE VfsHandle[TOTAL_IO_HANDLES];
    char Unknown[32];
    unsigned int WinApplicationPathOffset;
    unsigned int WinCurrentPathOffset;
    unsigned int WinCommandArgumentOffset;
    unsigned int WinCommandArgumentCount;
    unsigned int WslEnvOffset;
    unsigned short WindowHeight;
    unsigned short WindowWidth;
    unsigned char IsWithoutPipe;
} LXSS_MESSAGE_PORT_RECEIVE_OBJECT, *PLXSS_MESSAGE_PORT_RECEIVE_OBJECT;
#pragma pack (pop)

// Interop Flags
#define INTEROP_RESTORE_CONSOLE_STATE_MODE 1u // tcsetattr(3) restores size with TCSETS ioctl
#define INTEROP_LXBUS_WRITE_NT_PROCESS_STATUS 6u // write(2) NT process ExitStatus in LxBus client handle
#define INTEROP_LX_SEND_CONSOLE_SIZE 7u // init sends this to LxBus client handle
#define INTEROP_LXBUS_READ_NT_PROCESS_STATUS 8u // read(2) the handle message for ioctl TIOCGWINSZ request

typedef struct _LXSS_MESSAGE_PORT_SEND_OBJECT {
    struct {
        unsigned int CreateNtProcessFlag;
        unsigned int BufferSize;
        unsigned int LastError;
    } InteropMessage;
    unsigned int UnknownA;
    LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA HandleMessage;
    unsigned int IsSubsystemGUI;
    unsigned int UnknownB;
} LXSS_MESSAGE_PORT_SEND_OBJECT, *PLXSS_MESSAGE_PORT_SEND_OBJECT;

typedef struct _LXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE {
    unsigned int TerminalResizeFlag;
    unsigned int BufferSize;
    unsigned short WindowHeight;
    unsigned short WindowWidth;
} LXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE, *PLXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE;

// LxCore!LxpControlDeviceIoctlLxProcess
#define IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x34, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200D3u

typedef union _LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG {
    unsigned int TimeOut;
    unsigned int ExitStatus;
} LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG, *PLXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG;

#endif //LXBUS_H
