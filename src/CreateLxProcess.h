#ifndef CREATELXPROCESS_H
#define CREATELXPROCESS_H

#include "WslSession.h"

HRESULT CreateLxProcess(
    PWslSession* WslSession,
    GUID* DistroID);

#endif //CREATELXPROCESS_H
