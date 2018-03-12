@echo off

mkdir "%~dp0build"
cd "%~dp0build"
cmake -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" -G "Visual Studio 15 2017 Win64" %~dp0
cd "%~dp0"

cmake --build "%~dp0build" --target all_build --config Debug -- /m:16