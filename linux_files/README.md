# LxBusClient

WSL LxBus client to connect with LxBus server in Windows side. 

## How to build 

Open WSL in `linux_files` folder, just run `make` command. On success,
there will be `LxBusClient` file in `bin` folder (with WslReverse.exe).

## Requirements

* Root user. 
* `RootLxBusAccess` DWORD registry enabled. 
* Any latest C standard library installed in WSL distribution. 

## How to use

* Import [`LxCoreFlags.REG`](../Others/LxCoreFlags.REG) registry from Others folder, then reboot PC. 
* Run `WslReverse.exe --bus Distro_Name` command as administrator. 

* Open another Command Prompt, type `WslReverse.exe --run Distro_Name`,
go to `/bin` folder, run `./LxBusClient` ELF64 binary as root. 

* One side will wait forever until other side will be executed/connected. 
* If something goes wrong close Command Prompt window from Task Manager. 

## How it works

This Linux binary works as a client of LxBus. LxBus connection is limited
to root user only. After executing this binary, it waits _infinitely_ for
LxBus server, in this case it'll be `WslReverse.exe --bus <Distribution Name>`.
Then both binaries exchange some messages back and forth.
For further details, see the main [README](../README.md) file. 
