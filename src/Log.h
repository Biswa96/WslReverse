#ifndef LOG_H
#define LOG_H

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

void
WINAPI
LogResult(HRESULT hResult, PWSTR Function);

void
WINAPI
LogStatus(NTSTATUS Status, PWSTR Function);

void
WINAPI
Usage(void);

#endif // LOG_H
