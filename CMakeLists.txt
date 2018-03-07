cmake_minimum_required(VERSION 3.10)
project(heikousen)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_definitions(-DGLFW_INCLUDE_VULKAN)

add_executable(heikousen main.cpp)
set(BINARY heikousen)


find_package(bullet REQUIRED CONFIGS "BulletConfig.cmake" HINTS "${LIB_DIR}")
include_directories(${BULLET_INCLUDE_DIRS})
if (UNIX)
    include_directories("/usr/include/bullet")
endif(UNIX)
target_link_libraries(${BINARY} ${BULLET_LIBRARIES})





