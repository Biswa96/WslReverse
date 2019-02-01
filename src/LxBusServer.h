#ifndef CREATELXBUSSERVER_H
#define CREATELXBUSSERVER_H

typedef long HRESULT;
typedef struct _GUID GUID;
typedef struct _WslSession WslSession, *PWslSession;

HRESULT LxBusServer(PWslSession* wslSession, GUID* DistroID);

#endif //CREATELXBUSSERVER_H
