#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <wchar.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define GUID_STRING 40
typedef struct _GUID GUID;

void Log(
    unsigned long Result,
    wchar_t* Function);

void PrintGuid(
    GUID* id,
    wchar_t* string);

void Usage(
    void);

#endif // FUNCTIONS_H
