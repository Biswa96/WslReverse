#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <sdkddkver.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "WslInstance.h"
#include "WslSession.h"
#include "wgetopt.h"

void Log(HRESULT result, PCWSTR function);
void PrintGuid(GUID* guid);
void Usage();
