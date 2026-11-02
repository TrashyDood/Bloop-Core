cmake_minimum_required(VERSION 3.25)

project(Engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 CONFIG QUIET)
if(TARGET glfw)
    set(GLFW_TARGET glfw)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)
    set(GLFW_TARGET PkgConfig::GLFW)
endif()

add_executable(Engine
    src/main.cpp
    third_party/glad/src/gl.c
)

set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")
configure_file(shaders/vertex.glsl "${SHADER_OUTPUT_DIR}/vertex.glsl" COPYONLY)
configure_file(shaders/fragment.glsl "${SHADER_OUTPUT_DIR}/fragment.glsl" COPYONLY)

target_include_directories(Engine PRIVATE
    third_party/glad/include
)

target_compile_definitions(Engine PRIVATE
    ENGINE_SHADER_DIR="${SHADER_OUTPUT_DIR}"
)

target_link_libraries(Engine PRIVATE
    ${GLFW_TARGET}
)

if(WIN32)
    target_link_libraries(Engine PRIVATE opengl32)
endif()
