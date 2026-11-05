cmake_minimum_required(VERSION 3.25)

project(Engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
find_package(glm CONFIG REQUIRED)
find_file(GLFW_STATIC_LIBRARY
    NAMES libglfw3.a
    PATHS ${GLFW_LIBRARY_DIRS}
    NO_DEFAULT_PATH
    REQUIRED
)

set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
add_custom_target(copy_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "${SHADER_OUTPUT_DIR}"
    VERBATIM
)

file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Shared/*.cpp"
)

add_executable(Engine
    ${ENGINE_SOURCES}
    third_party/glad/src/gl.c
)
add_dependencies(Engine copy_shaders)

target_include_directories(Engine PRIVATE
    include
    third_party/glad/include
    ${GLFW_INCLUDE_DIRS}
)

target_link_libraries(Engine PRIVATE
    "${GLFW_STATIC_LIBRARY}"
    glm::glm
)

target_compile_definitions(Engine PRIVATE
    ENGINE_SHADER_DIR="${SHADER_OUTPUT_DIR}"
)

if(WIN32)
    target_link_libraries(Engine PRIVATE opengl32 gdi32)
endif()

if(MINGW)
    target_link_options(Engine PRIVATE -static -static-libgcc -static-libstdc++)
endif()
