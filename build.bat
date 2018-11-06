@echo off
::Set Environments for X86_64 build
cd %~dp0
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
where cl.exe link.exe

::Set Environment Variables
set NAME=WslReverse.exe
set BINDIR="bin"
set CFLAGS=/c /W4 /sdl /MD /Fo%BINDIR%\\
set CCOPT=/DRS_FIVE
set LFLAGS=/MACHINE:X64
set LIBS=Ole32.lib Shell32.lib NtDll.lib

::Build
mkdir %BINDIR%

rem build with new 19H1 RegisterDistribution() function
cl.exe %CFLAGS% src\*.c
link.exe %LFLAGS% %LIBS% %BINDIR%\*.obj /OUT:%BINDIR%\%NAME%

rem build with old RS5 RegisterDistribution() function
cl.exe %CFLAGS% %CCOPT% src\*.c
link.exe %LFLAGS% %LIBS% %BINDIR%\*.obj /OUT:%BINDIR%\Old%NAME%

rem get stats
dir %BINDIR%\*.exe
