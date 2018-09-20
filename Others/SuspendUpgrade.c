/*Function name EnsureNoConcurrentUpgrades() in wslconfig.exe*/
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        printf("enter Distribution ID with curly braces\n");
        exit(1);
    }

    char PipeName[100];
    HANDLE hPipe;
    ULONG BufferSize = 0x400u;
    
    sprintf(PipeName, "%s%s", "\\\\.\\pipe\\wslconfig_upgrade_", argv[1]);
    printf("PipeName: %s\n", PipeName);

    hPipe = CreateNamedPipe(
        PipeName,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE,
        PIPE_UNLIMITED_INSTANCES,
        BufferSize,
        BufferSize,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL
    );

    if (hPipe != INVALID_HANDLE_VALUE) {
        printf("Pipe created\nPress any key to close...\n");
        getchar();
        CloseHandle(hPipe);
    }
    else {
        printf("CreateNamedPipe Error: 0x%lx\n", GetLastError());
        exit(1);
    }
    return 0;
}
