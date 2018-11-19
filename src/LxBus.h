#ifndef LXBUS_H
#define LXBUS_H

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

// LxCore!LxpControlDeviceIoctlLxProcess
#define IOCTL_ADSS_LX_PROCESS_HANDLE_WAIT_FOR_SIGNAL \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x34, METHOD_NEITHER, FILE_ANY_ACCESS) //0x2200D3u

#endif //LXBUS_H
