# WslReverseHost 

Backend program to host WSL/Linux services and backend processes. 

# How to build 

For now, only msbuild and Visual Studio can build this with solution file. 

# How to use

WslReverse.exe automatically executes WslReverseHost.exe (like wslhost.exe)
in background for WSL/Linux services and daemons (like ssh, dbus etc).
The valid command for WslReveseHost is as following:

    WslReverseHost.exe [CurrentDistroID] [ServerHandle] [EventHandle] [ProcessHandle] [LxInstanceId] 

It is not possible to execute WslReverseHost from command line without running
WSlReverse because the Handles should be valid ones.

# How it works

Here are working steps:

* Validates argument counts. 
* Create ILxssUserSession COM instance of `CurrentDistroID`. 
* Get LxssInstance ONLY if it exist i.e. distribution is running, else exit. 
* Set the `EventHandle` from WslReverse.exe in signaled mode. 
* Wait for the WslReverse.exe `ProcessHandle` infinitely until it is terminated. 
* If `ProcessHandle` is terminated and `ServerHandle` exist creates
a worker thread and continuously running the background daemons/services in WSL. 
* If `ServerHandle` is terminated, close the worker thread as well as
the forked `/init` process. 
