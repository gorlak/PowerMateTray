@echo off

setlocal

for /f "delims=" %%f in ( '"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -property installationPath' ) do set VSWHERE=%%f
echo VSWHERE=%VSWHERE%

pushd %~dp0\submodule\wxWidgets\build\msw

if "%1" equ "" set BUILD_DEBUG=1
if "%1" equ "" set BUILD_RELEASE=1
if "%1" equ "debug" set BUILD_DEBUG=1
if "%1" equ "release" set BUILD_RELEASE=1

if defined BUILD_DEBUG cmd.exe /c "call "%VSWHERE%"\VC\Auxiliary\Build\vcvars64.bat && nmake.exe /f makefile.vc TARGET_CPU=AMD64 RUNTIME_LIBS=static DEBUG_INFO=1 BUILD=debug"
if defined BUILD_RELEASE cmd.exe /c "call "%VSWHERE%"\VC\Auxiliary\Build\vcvars64.bat && nmake.exe /f makefile.vc TARGET_CPU=AMD64 RUNTIME_LIBS=static DEBUG_INFO=1 BUILD=release"

endlocal
popd

(
echo build/*
echo lib/vc_*/*
echo include/wx/msw/setup.h
)>".git\modules\submodule\wxWidgets\info\exclude"
