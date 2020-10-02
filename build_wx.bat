@echo off

setlocal
pushd

cd %~dp0\submodule\wxWidgets\build\msw
cmd.exe /c "call "%VCINSTALLDIR%"\Auxiliary\Build\vcvars64.bat && nmake.exe /f makefile.vc TARGET_CPU=AMD64 RUNTIME_LIBS=static DEBUG_INFO=1 BUILD=debug %*"
cmd.exe /c "call "%VCINSTALLDIR%"\Auxiliary\Build\vcvars64.bat && nmake.exe /f makefile.vc TARGET_CPU=AMD64 RUNTIME_LIBS=static DEBUG_INFO=1 BUILD=release %*"

endlocal
popd

(
echo build/*
echo lib/vc_*/*
echo include/wx/msw/setup.h
)>".git\modules\submodule\wxWidgets\info\exclude"
