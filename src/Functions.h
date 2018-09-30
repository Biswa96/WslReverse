#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Windows.h>

void Log(HRESULT Result, PWSTR Function);
void PrintGuid(GUID* guid);
void Usage();

#endif //FUNCTIONS_H
