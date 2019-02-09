# LxBusClient

WSL LxBus client to connect with LxBus server in Windows side. 

## How to build 

Open WSL in `linux_files` folder, just run `make` command. On success, there will be `LxBusClient` file in `bin` folder (with WslReverse.exe).

## Requirements

* Root user. 
* `RootLxBusAccess` DWORD registry enabled. 
* Any latest C library installed in WSL distribution. 

## How to use

* Import `Lxss_Service.REG` registry from Others folder, reboot PC. 
* Run `WslReverse.exe --bus Distro_Name` command as administrator. 
* Open another Command Prompt, type `WslReverse.exe --run Distro_Name`, go to `/bin` folder, run `./LxBusClient` ELF64 binary as root. 
* One side will wait forever until other side will be executed. 
* If something goes wrong close Command Prompt window from Task Manager. 


For further details, see the main [README](../README.md) file. 
