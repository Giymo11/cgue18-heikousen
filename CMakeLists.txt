cmake_minimum_required(VERSION 3.10)
project(heikousen)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(LIB_DIR "${CMAKE_BINARY_DIR}/extern")

add_definitions(-DGLFW_INCLUDE_VULKAN)

file(GLOB SOURCE_FILES "src/*.cpp")
file(GLOB INCLUDE_FILES "include/*.cpp")

add_executable(heikousen ${SOURCE_FILES})
set(BINARY heikousen)

find_package(bullet REQUIRED
        CONFIGS "BulletConfig.cmake")
find_package(Vulkan REQUIRED)

if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
else()
    find_package(glfw3 REQUIRED HINTS "${LIB_DIR}")
endif()

message("Vulkan found? " ${Vulkan_FOUND})
message("Bullet found? " ${BULLET_FOUND})
message("GLFW3 found? " ${GLFW_FOUND})

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(
        ${BULLET_INCLUDE_DIRS}
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS})

if (UNIX)
    include_directories("/usr/include/bullet")
endif()


# case sensitive!
target_link_libraries(
        ${BINARY}
        ${BULLET_LIBRARIES}
        ${Vulkan_LIBRARIES}
        ${GLFW_STATIC_LIBRARIES}
        )





