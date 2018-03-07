cmake_minimum_required(VERSION 3.10)
project(vulkan_testo)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-DGLFW_INCLUDE_VULKAN)

add_executable(heikousen main.cpp)
set(BINARY heikousen)


find_package(bullet REQUIRED CONFIGS "BulletConfig.cmake")
include_directories(${BULLET_INCLUDE_DIRS})
if (UNIX)
    include_directories("/usr/include/bullet")
endif(UNIX)
target_link_libraries(${BINARY} ${BULLET_LIBRARIES})





