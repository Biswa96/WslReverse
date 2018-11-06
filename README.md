# WslReverse

[![Licence](https://img.shields.io/github/license/Biswa96/WslReverse.svg?style=for-the-badge)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![Top Language](https://img.shields.io/github/languages/top/Biswa96/WslReverse.svg?style=for-the-badge)](https://github.com/Biswa96/WslReverse.git)
[![Code size](https://img.shields.io/github/languages/code-size/Biswa96/WslReverse.svg?style=for-the-badge)]()

Reveal hidden COM interface between WSL and Lxss Manager Service. Heavily inspired by kernel guru **Alex Ionescu's project [lxss](https://github.com/ionescu007/lxss)**. This project is just a concept, not a fully developed program and should be used for testing purposes. 

## How to build

Clone this repository. Open the solution (.sln) or project (.vcxproj) file in Visual Studio and build it. Alternatively, run Visual Studio developer command prompt, go to the cloned folder and run this command: `msbuild.exe /p:Configuration=Release`. You can also build with mingw-w64 toolchain. Go to the folder in terminal run `mingw32-make` command for mingw-w64/msys2/cygwin. Some values are not defined in mingw-w64 toolchain. It will be updated soon.

## How to use

This project only shows the hidden COM methods which may change in future Windows version. The current COM vtable, used in this project, is according with 19H1 Insider version. For RS5 or October update version 1809, use the old executable compiled with `RS_FIVE` option. Use the new executable from Latest Windows 10 Insider 19H1 Preview Builds. Here are the options of WslReverse: 

```
Usage: WslReverse.exe [-] [option] [argument]
Options:
    -d, --get-id       [distribution name]      Get distribution GUID.
    -G, --get-default                           Get default distribution GUID.
    -g, --get-config   [distribution name]      Get distribution configuration.
    -h, --help                                  Show list of options.
    -i, --install      [distribution name]      Install distribution.
    -l, --list                                  List all distributions with pending ones.
    -r, --run          [distribution name]      Run bash in provided distribution.
    -S, --set-default  [distribution name]      Set default distribution.
    -s, --set-config   [distribution name]      Set configuration for distribution.
    -t, --terminate    [distribution name]      Terminate running distribution.
    -u, --uninstall    [distribution name]      Uninstall distribution.
```

## Project Overview

Here are the overview of source files according to their dependencies:

```
src\
    |
    +-- ConsolePid: Shows associated ConHost PID by IOCTL from condrv.sys
    +-- WinInternal: Crafted RTL_USER_PROCESS_PARAMETERS structure from PEB
    +-- CreateProcessAsync: Create Windows process with LxBus IPC mechanism
        |
        +-- CreateLxProcess: Run WSL pico processes
            |
            |   +-- Function: Helping functions to manipulate input & output
            |   +-- wgetopt: Converted from Cygwin getopt file for wide characters
            |   +-- WslInstance: LxssInstance COM interface
            |   +-- WslSession: LxssSession COM interface
            |   |
            +-- WslReverse: Main function with option processing
```

Check out the Others folder to unleashes the hidden beast. Here are the list of files in Other folders: 

* [Lxss_Service.REG](Others/Lxss_Service.REG): Enables Adss Bus, Force case sensitivity in DRVFS, Enable default flag and more fun stuffs. 
* [ExtractResource.c](Others/ExtractResource.c): Extract `init` and `bsdtar` from LxssManager.dll file. From Windows 10 insider build 18242, this doesn't work because `init` and `bsdtar` placed separately from `LxssManager.DLL` file. 
* [SuspendUpgrade.c](Others/SuspendUpgrade.c): Suspend upgrade and uninstallation procedure. 

## License 

This project is licensed under GNU Public License v3 or higher. You are free to study, modify or distribute the source code. 

```
WslReverse -- (c) Copyright 2018 Biswapriyo Nath

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```
