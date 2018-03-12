@echo off
mkdir %~dp0\extern\build
mkdir %~dp0\extern\dist

set "DIST_DIR=%~dp0extern\dist"
set "DIST_DIR=%DIST_DIR:\=/%"

mkdir "%~dp0extern\build\bullet"
cd "%~dp0extern\build\bullet"
cmake -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" -DBUILD_BULLET2_DEMOS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_CPU_DEMOS=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF -DBUILD_UNIT_TESTS=OFF -DINCLUDE_INSTALL_DIR="%DIST_DIR%/include/" -DLIB_DESTINATION="%DIST_DIR%/lib" -DUSE_GRAPHICAL_BENCHMARK=OFF -DBULLET2_USE_OPEN_MP_MULTITHREADING=ON -DINSTALL_LIBS=ON -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 15 2017 Win64" %~dp0extern\bullet3
cd "%~dp0"

mkdir "%~dp0extern\build\glfw"
cd "%~dp0extern\build\glfw"
cmake -DGLFW_INSTALL=1 -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DUSE_MSVC_RUNTIME_LIBRARY_DLL=OFF -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" -G "Visual Studio 15 2017 Win64" %~dp0extern\glfw
cd "%~dp0"

mkdir "%~dp0extern\build\glm"
cd "%~dp0extern\build\glm"
cmake -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" -DGLM_INSTALL_ENABLE=ON -G "Visual Studio 15 2017 Win64" %~dp0extern\glm
cd "%~dp0"

cmake --build "%~dp0/extern/build/bullet" --target all_build --config Release -- /m:16
cmake --build "%~dp0/extern/build/bullet" --target install --config Release

cmake --build "%~dp0/extern/build/glfw" --target all_build --config Release -- /m:16
cmake --build "%~dp0/extern/build/glfw" --target install --config Release

cmake --build "%~dp0/extern/build/glm" --target all_build --config Release -- /m:16
cmake --build "%~dp0/extern/build/glm" --target install --config Release