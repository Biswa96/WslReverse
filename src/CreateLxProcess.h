#ifndef CREATELXPROCESS_H
#define CREATELXPROCESS_H

HRESULT
WINAPI
CreateLxProcess(PWslSession* wslSession,
                GUID* DistroID,
                PSTR CommandLine,
                PSTR* Arguments,
                ULONG ArgumentCount,
                PWSTR LxssUserName);

#endif //CREATELXPROCESS_H
