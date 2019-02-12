#ifndef LOG_H
#define LOG_H

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define GUID_STRING 40

void
WINAPI
LogResult(HRESULT hResult, PWSTR Function);

void
WINAPI
LogStatus(NTSTATUS Status, PWSTR Function);

void
WINAPI
Usage(void);

void
WINAPI
PrintGuid(GUID* id, PWSTR string);

#endif // LOG_H
