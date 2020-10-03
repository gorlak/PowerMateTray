## PowerMateTray ##

PowerMateTray is a Windows notification area (AKA "system tray") application for application that provides access to the basic features of the bluetooth version of the Griffin PowerMate. The bluetooth version of this device only had an official software release for macOS. This program implements basic Windows session volume articulation.

If you fork and extend this project, drop me a line on twitter as I'd be happy to see it grow to support other generally useful features.

After cloning the repository open an Visual Studio command prompt in your preferred version and target architecture:

```
git submodule update --init --recursive
```
   
will pull down necessary submodules. Then run

```
build_wx.bat
```
   
to build wxWidgets. Then you can open the solution and build the matching architecture configuration (x64 or Win32)
