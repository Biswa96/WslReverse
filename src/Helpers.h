#ifndef HELPERS_H
#define HELPERS_H

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

void
WINAPI
LogResult(HRESULT hResult, PWSTR Function);

void
WINAPI
LogStatus(NTSTATUS Status, PWSTR Function);

NTSTATUS
NTAPI
MbsToWcs(PSTR src, PUNICODE_STRING dst);

void
WINAPI
Usage(void);

#endif // HELPERS_H
