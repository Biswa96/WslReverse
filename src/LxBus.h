#ifndef LXBUS_H
#define LXBUS_H

#ifndef CTL_CODE
#define FILE_ANY_ACCESS 0
#define METHOD_NEITHER 3
#define FILE_DEVICE_UNKNOWN 0x22
#define CTL_CODE( DeviceType, Function, Method, Access ) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

/*
* 0x22002Bu
* LxCore!LxBusRegisterServerIoctl
* ServerName must be less than USHRT_MAX except "lxssmanager"
*/
#define IOCTL_LXBUS_BUS_CLIENT_REGISTER_SERVER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 10, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef union _LXBUS_BUS_CLIENT_REGISTER_SERVER_MSG {
    char* LxBusServerName;
    unsigned long long ServerHandle;
} LXBUS_BUS_CLIENT_REGISTER_SERVER_MSG, *PLXBUS_BUS_CLIENT_REGISTER_SERVER_MSG;

/*
* 0x22002Fu
* LxCore!LxBusConnectServerIoctl
* Only allows when RootLxBusAccess REG_DWORD enabled
*/
#define IOCTL_LXBUS_BUS_CLIENT_CONNECT_SERVER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 11, METHOD_NEITHER, FILE_ANY_ACCESS)

#define LXBUS_CONNECT_WAIT_FOR_SERVER_FLAG 1
#define LXBUS_CONNECT_UNNAMED_SERVER_FLAG 2

typedef union _LXBUS_BUS_CLIENT_CONNECT_SERVER_MSG {
    struct {
        char* LxBusServerName;
        unsigned int Timeout;
        unsigned int Flags;
    };
    signed int ServerHandle;
} LXBUS_BUS_CLIENT_CONNECT_SERVER_MSG, *PLXBUS_BUS_CLIENT_CONNECT_SERVER_MSG;

/*
* 0x220033u
* LxCore!LxpControlDeviceIoctlServerPort
* IoCreateFile creates \Device\lxss\{Instance-GUID}\MessagePort
*/
#define IOCTL_LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 12, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef union _LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG {
    unsigned int Timeout;
    unsigned int ClientHandle;
} LXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG, *PLXBUS_IPC_SERVER_WAIT_FOR_CONNECTION_MSG;

/*
* 0x22006Fu
* LxCore!LxpControlDeviceCreateInstance
*/
#define IOCTL_LXSS_CONTROL_DEVICE_CREATE_INSTANCE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 27, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _LXSS_CONTROL_DEVICE_CREATE_INSTANCE {
    char x[88];
} LXSS_CONTROL_DEVICE_CREATE_INSTANCE, *PLXSS_CONTROL_DEVICE_CREATE_INSTANCE;

/*
* 0x220073u
* LxCore!LxpInstanceSetState
*/
#define IOCTL_LXSS_SET_INSTANCE_STATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 28, METHOD_NEITHER, FILE_ANY_ACCESS)

/*
* 0x220077u
* LxCore!LxpInstanceGetState
*/
#define IOCTL_LXSS_GET_INSTANCE_STATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 29, METHOD_NEITHER, FILE_ANY_ACCESS)

#define LXSS_INSTANCE_STATE_START 0
#define LXSS_INSTANCE_STATE_STOP 1
#define LXSS_INSTANCE_STATE_DESTORY 2
#define LXSS_INSTANCE_STATE_EXIT 3

/*
* 0x220097u
* LxCore!LxBuspIpcConnectionMarshalProcess
*/
#define IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_PROCESS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 37, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef union _LXBUS_IPC_MESSAGE_MARSHAL_PROCESS_MSG {
    unsigned int ProcessId;
    void* ProcessHandle;
    unsigned long long ProcessIdCount;
} LXBUS_IPC_MESSAGE_MARSHAL_PROCESS_MSG, *PLXBUS_IPC_MESSAGE_MARSHAL_PROCESS_MSG;

/*
* 0x22009Bu
* LxCore!LxBuspIpcConnectionUnmarshalProcess
*/
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_PROCESS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 38, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef LXBUS_IPC_MESSAGE_MARSHAL_PROCESS_MSG LXBUS_IPC_MESSAGE_UNMARSHAL_PROCESS_MSG;

/*
* 0x22009Fu
* LxCore!LxBuspIpcConnectionMarshalHandle
* ObReferenceObjectByHandle allows IoFileObjectType only
*/
#define IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_HANDLE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 39, METHOD_NEITHER, FILE_ANY_ACCESS)

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

/*
* 0x2200A3u
* LxCore!LxBuspIpcConnectionUnmarshalHandle
*/
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_HANDLE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 40, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef LXBUS_IPC_MESSAGE_MARSHAL_HANDLE_DATA LXBUS_IPC_MESSAGE_UNMARSHAL_HANDLE_DATA;

/*
* 0x2200A7u
* LxCore!LxBuspIpcConnectionMarshalConsole
* ObReferenceObjectByHandle allows PsProcessType only
*/
#define IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_CONSOLE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 41, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef union _LXBUS_IPC_MESSAGE_MARSHAL_CONSOLE_MSG {
    unsigned int Handle;
    unsigned long long ConsoleIdCount;
} LXBUS_IPC_MESSAGE_MARSHAL_CONSOLE_MSG, *PLXBUS_IPC_MESSAGE_MARSHAL_CONSOLE_MSG;

typedef struct _LXBUS_IPC_MESSAGE_SEND_CONSOLE_DATA {
    unsigned int Flag;
    unsigned int BufferSize;
    LXBUS_IPC_MESSAGE_MARSHAL_CONSOLE_MSG ConsoleMessage;
} LXBUS_IPC_MESSAGE_SEND_CONSOLE_DATA, *PLXBUS_IPC_MESSAGE_SEND_CONSOLE_DATA;

/*
* 0x2200ABu
* LxCore!LxBuspIpcConnectionUnmarshalConsole
*/
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_CONSOLE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 42, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef LXBUS_IPC_MESSAGE_MARSHAL_CONSOLE_MSG LXBUS_IPC_MESSAGE_UNMARSHAL_CONSOLE_MSG;

/*
* 0x2200AFu
* LxCore!LxBuspIpcConnectionMarshalForkToken
* SeSinglePrivilegeCheck allows only when SeAssignPrimaryTokenPrivilege enabled
*/
#define IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_FORK_TOKEN \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 43, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef union _LXBUS_IPC_CONNECTION_MARSHAL_FORK_TOKEN_MSG {
    unsigned int Handle;
    unsigned long long TokenIdCount;
} LXBUS_IPC_CONNECTION_MARSHAL_FORK_TOKEN_MSG, *PLXBUS_IPC_CONNECTION_MARSHAL_FORK_TOKEN_MSG;

/*
* 0x2200B3u
* LxCore!LxBuspIpcConnectionUnmarshalForkToken
*/
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_FORK_TOKEN \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 44, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef LXBUS_IPC_CONNECTION_MARSHAL_FORK_TOKEN_MSG LXBUS_IPC_CONNECTION_UNMARSHAL_FORK_TOKEN_MSG;

/*
* 0x2200B7u
* LxCore!LxBuspIpcConnectionMarshalVfsFile
* StdFd greate than 2 (i.e. stderr) is invalid
*/
#define IOCTL_LXBUS_IPC_CONNECTION_MARSHAL_VFS_FILE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 45, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef union _LXBUS_IPC_MESSAGE_MARSHAL_VFSFILE_MSG {
    unsigned int StdFd;
    void* Handle;
    unsigned long long HandleIdCount;
} LXBUS_IPC_MESSAGE_MARSHAL_VFSFILE_MSG, *PLXBUS_IPC_MESSAGE_MARSHAL_VFSFILE_MSG;

/*
* 0x2200BBu
* LxCore!LxBuspIpcConnectionUnmarshalVfsFile
* IoCreateFile creates \Device\lxss\{Instance-GUID}\VfsFile
*/
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_VFS_FILE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 46, METHOD_NEITHER, FILE_ANY_ACCESS) 

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
#define INTEROP_RESTORE_CONSOLE_STATE_MODE 1 // tcsetattr(3) restores size with TCSETS ioctl
#define INTEROP_LXBUS_WRITE_NT_PROCESS_STATUS 6 // write(2) NT process ExitStatus in LxBus client handle
#define INTEROP_LX_SEND_CONSOLE_SIZE 7 // init sends this to LxBus client handle
#define INTEROP_LXBUS_READ_NT_PROCESS_STATUS 8 // read(2) the handle message for ioctl TIOCGWINSZ request

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

/*
* Resize message is created from ioctl(fd, TIOCGWINSZ, &winsize);
* Resize message is applied to ResizePseudoConsole(ProcResult->hpCon, ConsoleSize);
*/
typedef struct _LXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE {
    unsigned int TerminalResizeFlag;
    unsigned int BufferSize;
    unsigned short WindowHeight;
    unsigned short WindowWidth;
} LXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE, *PLXBUS_TERMINAL_WINDOW_RESIZE_MESSAGE;

/*
* 0x2200BFu
* LxCore!LxBusIpcNtProcessFileIoctl
*/
#define IOCTL_LXBUS_IPC_NT_PROCESS_FILE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 47, METHOD_NEITHER, FILE_ANY_ACCESS)

/*
* 0x2200C3u
* LxCore!LxBuspIpcConnectionCreateUnnamedServer
* IoCreateFile creates \Device\lxss\{Instance-GUID}\ServerPort
*/
#define IOCTL_LXBUS_IPC_CONNECTION_CREATE_UNNAMED_SERVER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 48, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _LXBUS_IPC_CONNECTION_CREATE_UNNAMED_SERVER_MSG {
    unsigned long long ServerPortHandle;
    unsigned long long ServerPortIdCount;
} LXBUS_IPC_CONNECTION_CREATE_UNNAMED_SERVER_MSG, *PLXBUS_IPC_CONNECTION_CREATE_UNNAMED_SERVER_MSG;

/*
* 0x2200C7u
* LxCore!LxBuspIpcConnectionUnmarshalServerPort
*/
#define IOCTL_LXBUS_IPC_CONNECTION_UNMARSHAL_SERVER_PORT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 49, METHOD_NEITHER, FILE_ANY_ACCESS)

/*
* 0x2200CBu
* LxCore!LxBuspIpcConnectionReleaseConsole
* LxCore!LxBuspIpcConnectionReleaseHandle
* LxCore!LxBuspIpcConnectionReleaseForkToken
*/
#define IOCTL_LXBUS_IPC_CONNECTION_RELEASE_HANDLES \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 50, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef enum _LXBUS_IPC_CONNECTION_RELEASE_HANDLE_TYPE {
    ConsoleHandleReleaseType,
    MarshalHandleReleaseType,
    ForkTokenReleaseType,
    InvalidReleaseType
} LXBUS_IPC_CONNECTION_RELEASE_HANDLE_TYPE;

typedef struct _LXBUS_IPC_MESSAGE_RELEASE_HANDLE_DATA {
    long long Handle;
    LXBUS_IPC_CONNECTION_MARSHAL_HANDLE_TYPE Type;
    int Ununsed;
} LXBUS_IPC_MESSAGE_RELEASE_HANDLE_DATA, *PLXBUS_IPC_MESSAGE_RELEASE_HANDLE_DATA;

/*
* 0x2200CFu
* LxCore!LxBuspIpcConnectionDisconnectConsole
*/
#define IOCTL_LXBUS_IPC_CONNECTION_DISCONNECT_CONSOLE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 51, METHOD_NEITHER, FILE_ANY_ACCESS)

/*
* 0x2200D3u
* LxCore!LxpControlDeviceIoctlLxProcess
*/
#define IOCTL_LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 52, METHOD_NEITHER, FILE_ANY_ACCESS) 

typedef union _LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG {
    unsigned int TimeOut;
    unsigned int ExitStatus;
} LXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG, *PLXBUS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL_MSG;

#endif //LXBUS_H
