cmake_minimum_required(VERSION 3.25)

project(Engine LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_GRAPHICS "Build the graphics library and demo application" OFF)

find_package(glm CONFIG REQUIRED)

add_library(Shared STATIC
    Shared/src/Shared.cpp
    Shared/src/Transform.cpp
)
target_include_directories(Shared PUBLIC
    Shared/include
)
target_link_libraries(Shared PUBLIC glm::glm)

if(WIN32)
    target_link_libraries(Shared PUBLIC winmm)
endif()

add_library(EngineCore
    Engine/src/InputHandler.cpp
    Engine/src/Scene.cpp
)
target_include_directories(EngineCore PUBLIC Engine/include)
target_link_libraries(EngineCore PUBLIC Shared)

if(ENABLE_GRAPHICS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    find_file(GLFW_STATIC_LIBRARY
        NAMES libglfw3.a
        PATHS ${GLFW_LIBRARY_DIRS}
        NO_DEFAULT_PATH
        REQUIRED
    )

    set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders")
    add_custom_target(copy_shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
            "${SHADER_OUTPUT_DIR}"
        VERBATIM
    )

    set(MODEL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/models")
    add_custom_target(copy_models
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/models"
            "${MODEL_OUTPUT_DIR}"
        VERBATIM
    )

    file(GLOB GRAPHICS_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Graphics/src/*.cpp"
    )

    add_library(Graphics STATIC
        ${GRAPHICS_SOURCES}
        third_party/glad/src/gl.c
    )
    target_include_directories(Graphics PUBLIC
        Graphics/include
        third_party/glad/include
        ${GLFW_INCLUDE_DIRS}
    )
    target_link_libraries(Graphics PUBLIC
        Shared
        "${GLFW_STATIC_LIBRARY}"
        glm::glm
    )

    if(WIN32)
        target_link_libraries(Graphics PUBLIC opengl32 gdi32)
    endif()

    add_executable(Engine
        Application/src/main.cpp
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/imgui/backends/imgui_impl_opengl3.cpp
    )
    add_dependencies(Engine copy_shaders copy_models)
    target_include_directories(Engine PRIVATE
        third_party/imgui
        third_party/imgui/backends
    )
    target_link_libraries(Engine PRIVATE Graphics EngineCore)
    target_compile_definitions(Engine PRIVATE
        ENABLE_GRAPHICS=1
        ENGINE_SHADER_DIR="${SHADER_OUTPUT_DIR}"
        ENGINE_MODEL_DIR="${MODEL_OUTPUT_DIR}"
    )

    if(MINGW)
        target_link_options(Engine PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(EngineTests tests/UnitTests.cpp)
    target_link_libraries(EngineTests PRIVATE EngineCore)

    if(ENABLE_GRAPHICS)
        target_sources(EngineTests PRIVATE Graphics/src/Camera.cpp Graphics/src/GlfwInput.cpp)
        target_include_directories(EngineTests PRIVATE
            Graphics/include
            third_party/glad/include
            ${GLFW_INCLUDE_DIRS}
        )
        target_compile_definitions(EngineTests PRIVATE ENABLE_GRAPHICS=1)
    endif()

    add_test(NAME EngineTests COMMAND EngineTests)
endif()
