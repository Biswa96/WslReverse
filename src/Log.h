#ifndef LOG_H
#define LOG_H

#include <wchar.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define GUID_STRING 40
typedef struct _GUID GUID;

void LogResult(long hResult, wchar_t* Function);
void LogStatus(long Status, wchar_t* Function);

void Usage(void);
void PrintGuid(GUID* id, wchar_t* string);

#endif // LOG_H
