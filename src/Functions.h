#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <wchar.h>

void Log(
    unsigned long Result,
    wchar_t* Function);

#define GUID_STRING 40
typedef struct _GUID GUID;

void PrintGuid(
    GUID* id,
    wchar_t* string);

void Usage(
    void);

#endif // FUNCTIONS_H
