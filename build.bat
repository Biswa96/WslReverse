@echo off
::Set Environments for X86_64 build
cd %~dp0
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
where cl.exe link.exe

::Set Environment Variables
set NAME=WslReverse.exe
set BINDIR="bin\\"
set CFLAGS=/c /W4 /sdl /Fo%BINDIR%
set LFLAGS=/OUT:%NAME% /MACHINE:X64
set LIBS=Ole32.lib Shell32.lib NtDll.lib

::Build
mkdir %BINDIR%
cl.exe %CFLAGS% src\*.c
cd %BINDIR%
link.exe %LFLAGS% %LIBS% *.obj
dir *.exe
cd ..\
