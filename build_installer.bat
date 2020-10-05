@echo off

setlocal

for /f "tokens=3" %%f in ( 'findstr /C:"#define VERSION_MAJOR" resource.h' ) do SET MAJOR=%%f
for /f "tokens=3" %%f in ( 'findstr /C:"#define VERSION_MINOR" resource.h' ) do SET MINOR=%%f
for /f "tokens=3" %%f in ( 'findstr /C:"#define VERSION_PATCH" resource.h' ) do SET PATCH=%%f

%~dp0submodule\InnoSetup\ISCC.exe installer.iss /d"_AppVersionMajor=%MAJOR%" /d"_AppVersionMinor=%MINOR%" /d"_AppVersionPatch=%PATCH%"

endlocal