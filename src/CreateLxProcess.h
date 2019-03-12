#ifndef CREATELXPROCESS_H
#define CREATELXPROCESS_H

void
WINAPI
CreateProcessWorker(PTP_CALLBACK_INSTANCE Instance,
                    HANDLE ServerHandle,
                    PTP_WORK Work);

HRESULT
WINAPI
CreateLxProcess(PWslSession* wslSession,
                GUID* DistroID,
                PSTR CommandLine,
                PSTR* Arguments,
                ULONG ArgumentCount,
                PWSTR LxssUserName);

#endif //CREATELXPROCESS_H
