#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "WslSession.h"
#include "WslInstance.h"
#include "wgetopt.h"

void Log(HRESULT Result, PWSTR Function);
void PrintGuid(GUID* guid);
void Usage();
