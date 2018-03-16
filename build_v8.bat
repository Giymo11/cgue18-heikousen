@echo off

set PATH=%~dp0extern\depot_tools;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
set GYP_MSVS_VERSION=2017

cmd /c gclient

rem Fetch
cd "%~dp0extern"
cmd /c fetch v8

rem Pin version and sync
cd "%~dp0extern\v8"
cmd /c git checkout 6.7.43
cmd /c gclient sync

rem Generate
cmd /c python tools/dev/v8gen.py x64.release

rem Fix args.gn
echo v8_static_library = true >> out.gn\x64.release\args.gn
echo v8_use_external_startup_data = false >> out.gn\x64.release\args.gn 
cmd /c gn gen x64.release

rem rem Compile
cmd /c ninja -C out.gn/x64.release
cd "%~dp0"

rem Copy generated libs
cd "%~dp0extern\v8\out.gn\x64.release\obj"
for /r %%a in (*.lib) do xcopy "%%a" "%~dp0extern\dist\lib" /Y /i
cd "%~dp0"

rem Copy headers
mkdir "%~dp0extern\dist\include\v8"
xcopy /s/Y "%~dp0extern\v8\include" "%~dp0extern\dist\include\v8"