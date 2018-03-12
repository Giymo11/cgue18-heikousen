cmake_minimum_required(VERSION 3.10)
project(heikousen)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

set(LIB_DIR "${CMAKE_BINARY_DIR}/extern")

add_definitions(-DGLFW_INCLUDE_VULKAN)


file(GLOB SOURCE_FILES "src/*.[ch]pp")
file(GLOB INCLUDE_FILES "include/*.cpp")
file(GLOB SHADER_FILES "shader/*.vert" "shader/*.frag")

find_program(GLSL_EXECUTABLE glslangValidator)
function(preprocess_glsl out_var)
    set(result)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shader)
    foreach (in_f ${ARGN})
        file(RELATIVE_PATH out_f ${CMAKE_CURRENT_SOURCE_DIR} ${in_f})
        set(out_f "${CMAKE_CURRENT_BINARY_DIR}/${out_f}.spv")
        add_custom_command(OUTPUT ${out_f}
                COMMAND ${GLSL_EXECUTABLE} -V -o ${out_f} ${in_f}
                DEPENDS ${in_f}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Compiling shader ${in_f}"
                VERBATIM
                )
        list(APPEND result ${out_f})
    endforeach ()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

preprocess_glsl(COMPILED_SHADERS ${SHADER_FILES})

find_package(Bullet REQUIRED HINTS "${CMAKE_SOURCE_DIR}/extern/dist/lib/cmake/bullet")
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED HINTS "${CMAKE_SOURCE_DIR}/extern/dist/lib/cmake/glm")

if (UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
else()
    find_package(glfw3 REQUIRED HINTS "${CMAKE_SOURCE_DIR}/extern/dist/lib/cmake/glfw3")
endif()

link_directories(${BULLET_LIBRARY_DIRS})

message("Vulkan found? " ${Vulkan_FOUND})
message("Bullet found? " ${BULLET_FOUND})
message("GLFW3 found? " ${GLFW_FOUND})
message("GLM found? " ${GLM_FOUND})

add_executable(heikousen ${SOURCE_FILES} ${SHADER_FILES} ${COMPILED_SHADERS})
set(BINARY heikousen)

if (UNIX)
    target_include_directories(
        heikousen PRIVATE
        "/usr/include/bullet"
    )
endif()

target_include_directories(
    heikousen PRIVATE
    ${BULLET_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    )

# case sensitive!
target_link_libraries(
    ${BINARY}
    ${BULLET_LIBRARIES}
    vulkan
    glm
    glfw
    )
