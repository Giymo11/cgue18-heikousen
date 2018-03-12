@echo off

set PATH=%~dp0extern\depot_tools;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set GYP_MSVS_VERSION=2017

cmd /c gclient

rem Sync
cd "%~dp0extern"
cmd /c fetch v8
cd "%~dp0extern\v8"
cmd /c gclient sync

rem Generate
cmd /c git checkout 6.7.41
cmd /c python tools/dev/v8gen.py x64.release

rem Fix args.gn
echo is_component_build = false >>  out.gn\x64.release\args.gn
echo v8_static_library = true >>  out.gn\x64.release\args.gn

rem Compile
cmd /c ninja -C out.gn/x64.release

rem Copy generated libs
cd "%~dp0extern\v8\out.gn\x64.release\obj"
for /r %%a in (*.lib) do xcopy "%%a" "%~dp0extern\dist\lib" /Y /i
cd "%~dp0"

rem Copy headers
mkdir "%~dp0extern\dist\include"
copy "%~dp0extern\v8\include" "%~dp0extern\dist\include\v8"