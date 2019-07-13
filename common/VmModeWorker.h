#ifndef VMMODEWORKER_H
#define VMMODEWORKER_H

void
WINAPI
VmModeWorker(SOCKET SockIn,
             SOCKET SockOut,
             SOCKET SockErr,
             SOCKET ServerSocket,
             PLXSS_STD_HANDLES StdHandles);

#endif /* VMMODEWORKER_H */
