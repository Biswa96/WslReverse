#ifndef CREATELXPROCESS_H
#define CREATELXPROCESS_H

typedef long HRESULT;
typedef struct _GUID GUID;
typedef struct _WslSession WslSession, *PWslSession;

HRESULT CreateLxProcess(
    PWslSession* wslSession,
    GUID* DistroID);

#endif //CREATELXPROCESS_H
