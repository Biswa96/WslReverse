# WslReverse

[![Licence](https://img.shields.io/github/license/Biswa96/WslReverse.svg)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![Top Language](https://img.shields.io/github/languages/top/Biswa96/WslReverse.svg)](https://github.com/Biswa96/WslReverse.git)
[![Code size](https://img.shields.io/github/languages/code-size/Biswa96/WslReverse.svg)]()

Experiments with hidden COM interface and LxBus IPC mechanism in WSL.
Heavily inspired by kernel guru **Alex Ionescu's project [lxss]**.
This project is just a concept, not a fully developed program and
should be used for testing purposes. 

[lxss]: https://github.com/ionescu007/lxss.git 

## How to build

Clone this repository. Open the solution (.sln) or project (.vcxproj) file
in Visual Studio and build it. Alternatively, run Visual Studio developer
command prompt, go to the cloned folder and run `msbuild` command.
This project can also be built with mingw-w64 toolchain. Open terminal in the
cloned folder and run `make` command. The binaries will be in `/bin` folder. 

## How to use

Download the binary from [Release page], no installation steps are required.
This project only shows the hidden COM methods which may change in future
Windows version. The COM vtable, used in this project, is according to
_latest Windows 10 20H1 Insider Preview_, that is build 18917 and above.
Here are the options of WslReverse: 

[Release page]: https://github.com/Biswa96/WslReverse/releases

```
Usage: WslReverse.exe [-] [option] [argument]

Options:
  -b, --bus          [Distro]      Create own LxBus server (as administrator).
  -d, --get-id       [Distro]      Get distribution ID.
  -e, --export       [Distro]  [File Name]
                                   Exports selected distribution to a tar file.
  -G, --get-default                Get default distribution ID.
  -g, --get-config   [Distro]      Get distribution configuration.
  -h, --help                       Show this help information.
  -i, --install      [Distro]  [Install Folder]  [File Name]
                                   Install tar file as a new distribution.
  -l, --list                       List all distributions with pending ones.
  -r, --run          [Distro]      Run bash in provided distribution.
  -S, --set-default  [Distro]      Set default distribution.
  -s, --set-config   [Distro]      Set configuration for distribution.
  -t, --terminate    [Distro]      Terminate running distribution.
  -u, --uninstall    [Distro]      Uninstall distribution.
```


## Project layout

Most of the definitions are in `LxBus.h` and `WinInternal.h` header files.
The project layout of source files:

* common:

  - CreateLxProcess: Run WSL1 pico processes
  - CreateProcessAsync: Create worker thread for LxBus IPC mechanism
  - CreateWinProcess: Create Windows process with LxBus server
  - GetConhostServerId: Shows associated ConHost PID by IOCTL from condrv.sys
  - Helpers: Helping functions to log return values and more
  - LxBus: Required IOCTLs and associated structures
  - LxBusServer: Send/Receive various types of messages with LxBus Server
  - LxssUserSession: LxssUserSession COM interface
  - SpawnWslHost: Compose backend process command line and create process
  - VmModeWorker: Run WSL2 processes
  - wgetopt: Converted from Cygwin getopt file for wide characters
  - WinInternal: Crafted RTL_USER_PROCESS_PARAMETERS and PEB structures

* frontend:

  - WslReverse: Main function with option processing

* backend:

  - WslReverseHost: Main function for backend processing

* linux_files:

  - LxBusClient: Client process which connect to LxBus server in forntend

* wslcli:

  - WslClient: WslClient COM interface for wsl.exe, bash.exe, wslconfig.exe and wslhost.exe.


## Take a long ride with :minibus:

To use LxBus, import the [LxCoreFlags registry file](Others/LxCoreFlags.REG).
Then reboot PC. Compile the [LxBusClient.c](linux_files/LxBusClient.c)
with `make` in WSL. Execute WslRevese with `-b` or `--bus` option as administrator
and LxBusClient as root user in WSL. Those two binaries exchange some messages
between WSL and Windows side using LxBus via. LxCore driver. Here are some of them:

| Step No. | LxBus Server (as Administrator)          | LxBus Client (as root)                |
|:--------:|:----------------------------------------:|:-------------------------------------:|
|  1       | Register LxBus server, wait for client   | Open lxss device, connect to server   |
|  2       | Read message from LxBus client           | Write message to LxBus server         |
|  3       | Write message to LxBus client            | Read message from LxBus server        |
|  4       | Marshal W-end pipe, read from R-end pipe | Unmarshal W-end pipe, write message   |
|  5       | Marshal R-end pipe, write to W-end pipe  | Unmarshal R-end pipe, read message    |
|  6       | Unmarshal standard I/O file descriptors  | Marshal standard I/O file descriptors |
|  7       | Unmarshal and get PID from client side   | Marshal current PID                   |
|  8       | Marshal console message                  | Unmarshal console message             |
|  9       | Create unnamed LxBus server              | To be continued ...                   |
| 10       | Marshal fork token                       | Unmarshal fork token                  |

For detailed explanation, see Alex Ionescu's presentation [@34min]
at BlackHat USA 2016. There are many things that can be done with LxBus
IPC mechanism. What interesting thing do you want to do with LxBus? :yum: 

[@34min]: https://youtu.be/36Ykla27FIo?t=2077


## Trace Syscalls

This works with **WSL1 only** because LxCore does not involve _directly_ with WSL2.
First import [LxCoreFlags registry file](Others/LxCoreFlags.REG). Then enable
local kernel mode debugging with these two command as administrator and reboot PC.

    bcdedit /debug on
    bcdedit /dbgsettings local

This enables some DWORD registry flags. Behind the scene, LxCore _mainly_ checks if
`PrintSysLevel` and `PrintLogLevel` are both zero and `TraceLastSyscall` is present.
For the same host machine, use [DebugView] as administrator or use KD for VM.

<img src=Others/LxCoreSyscalls.PNG>

Run any WSL1 distribution and see the logs and every syscalls and dmesg.
The functions behind these logs format are like this:

    DbgPrintEx(0, 0, "LX: (%p, %p) %s", PEPROCESS, PKTHREAD, Syscall);
    DbgPrintEx(0, 0, "LX: (%p, %p) /dev/kmsg: %Z", PEPROCESS, PKTHREAD, Version);
    DbgPrintEx(0, 0, "LX: (%p, %p) /dev/log: %d: %Z: %Z\n", PEPROCESS, PKTHREAD, x, y, z);
    DbgPrintEx(0, 0, "LX: (%p, %p) (%Z) %s\n", PEPROCESS, PKTHREAD, Command, LxCoreFunction);

[DebugView]: https://docs.microsoft.com/en-us/sysinternals/downloads/debugview

## Trace Events

* List of Event Providers and associated GUID:

|           Provider Name               |             Provider GUID              |    File Name     |
|:-------------------------------------:|:--------------------------------------:|:----------------:|
| Microsoft.Windows.Lxss.Manager        | {B99CDB5A-039C-5046-E672-1A0DE0A40211} | LxssManager.dll  |
| Microsoft.Windows.Lxss.Heartbeat      | {0451AB4F-F74D-4008-B491-EB2E5F5D8B89} | LxssManager.dll  |
| Microsoft.Windows.Subsystem.LxCore    | {0CD1C309-0878-4515-83DB-749843B3F5C9} | LxCore.sys       |
| Microsoft.Windows.Subsystem.Lxss      | {D90B9468-67F0-5B3B-42CC-82AC81FFD960} | Wsl.exe          |


## Acknowledgments

This project uses some definitions and data types from followings. Thanks to:

* Alex Ionescu for [lxss project] and all the IOCTLs details ([MIT License]).
* Steven G, Wen Jia Liu and others for ProcessHacker's collection of [native API] ([GPLv3 License]).
* Petr Beneš for [pdbex tool](https://github.com/wbenny/pdbex).

[lxss project]: https://github.com/ionescu007/lxss
[MIT License]: https://github.com/ionescu007/lxss/blob/master/LICENSE
[native API]: https://github.com/processhacker/processhacker/tree/master/phnt
[GPLv3 License]: https://github.com/processhacker/processhacker/blob/master/LICENSE.txt


## License

WslReverse is licensed under the GNU General Public License v3.
A full copy of the license is provided in [LICENSE](LICENSE).

    WslReverse -- Experiments with COM interface and LxBus IPC mechanism in WSL.
    Copyright (c) 2018-19 Biswapriyo Nath
    
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
