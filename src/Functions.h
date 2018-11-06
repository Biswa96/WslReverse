#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Windows.h>

void Log(HRESULT Result, PWSTR Function);
void PrintGuid(GUID* id, PWSTR string);
void Usage();

#endif //FUNCTIONS_H
