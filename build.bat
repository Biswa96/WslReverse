:: batch file to compile whole project
:: some helful links:
:: https://stackoverflow.com/questions/50848318/


@echo off

:: set environments for X86_64 build
cd %~dp0
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
where cl.exe link.exe

:: disable warnings
set CCOPT=/wd"4201" /wd"4214"

:: set environment vriables
set BIN=bin
set CFLAGS=/c /nologo /Os /MD /W4
set LFLAGS=/nologo /MACHINE:X64
set LIBS=ntdll.lib ole32.lib shell32.lib ws2_32.lib

:: cleanup
rd /s /q %BIN%
mkdir %BIN%

:: #1 build common static library
set SRC=common

mkdir %BIN%\%SRC%
cl %CFLAGS% %CCOPT% /Fo%BIN%\%SRC%\\ %SRC%\*.c
link /lib %LFLAGS% %BIN%\%SRC%\*.obj /OUT:%BIN%\%SRC%.lib

:: #2 build frontend executable
set SRC=frontend

mkdir %BIN%\%SRC%
cl %CFLAGS% %CCOPT% /I"common" /Fo%BIN%\%SRC%\\ %SRC%\WslReverse.c
link %LFLAGS% %LIBS% %BIN%\common.lib %BIN%\%SRC%\*.obj /OUT:%BIN%\WslReverse.exe

:: #3 build backend executable
set SRC=backend

mkdir %BIN%\%SRC%
cl %CFLAGS% %CCOPT% /I"common" /Fo%BIN%\%SRC%\\ %SRC%\WslReverseHost.c
link %LFLAGS% %LIBS% %BIN%\common.lib %BIN%\%SRC%\*.obj /OUT:%BIN%\WslReverseHost.exe

dir /B %BIN%\*.exe
pause
exit /b

::END#