# WslReverseHost 

Backend program to host WSL/Linux services and backend processes. 

# How to build 

In Visual Studio, this is automatically compiled from main solution file.
In case of mingw-w64 toolchain, the main makefile also automatically compile this. 

# How to use

WslReverse.exe executes WslReverseHost.exe (like wslhost.exe) with valid parameters
in background for WSL/Linux services and daemons (like ssh, dbus etc). 
The valid command for WslReveseHost is as following:

    WslReverseHost.exe [CurrentDistroID] [ServerHandle] [EventHandle] [ProcessHandle] [LxInstanceID] 

It is not easy to execute WslReverseHost from command line without running
WslReverse because the handles (i.e. command parameters) should be valid and inherited. 

# How it works

Here are working steps:

* Validate argument counts. 
* Create ILxssUserSession COM instance of `CurrentDistroID`. 
* Get LxssInstance ONLY if it exist i.e. distribution is running, else exit. 
* Set the `EventHandle` from WslReverse.exe in signaled mode. 
* Wait for the WslReverse.exe `ProcessHandle` infinitely until it is terminated. 
* If `ProcessHandle` is terminated and `ServerHandle` exist then creates
a worker thread and continuously running the background daemons/services in WSL. 
* If `ServerHandle` is terminated, close the worker thread as well as
the forked `/init` process. 
