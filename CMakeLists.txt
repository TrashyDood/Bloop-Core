cmake_minimum_required(VERSION 3.25)

project(Engine LANGUAGES C CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_ENGINE "Build the engine library" ON)
option(BUILD_GRAPHICS "Build the graphics library" OFF)

find_package(glm CONFIG REQUIRED)

file(GLOB SHARED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Shared/src/*.cpp"
)
add_library(Shared STATIC ${SHARED_SOURCES})
target_include_directories(Shared PUBLIC
    Shared/include
)
target_link_libraries(Shared PUBLIC glm::glm)
target_compile_definitions(Shared PUBLIC LOGGING_ENABLED)

if(WIN32)
    target_link_libraries(Shared PUBLIC winmm)
endif()

if(BUILD_ENGINE)
    file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Engine/src/*.cpp"
    )
    add_library(Engine STATIC ${ENGINE_SOURCES})
    target_include_directories(Engine PUBLIC Engine/include)
    
    target_link_libraries(Engine PUBLIC Shared)
        target_compile_definitions(Engine PUBLIC
        ENGINE_ENABLED
    )
endif()

if(BUILD_GRAPHICS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    pkg_check_modules(STB REQUIRED IMPORTED_TARGET stb)
    find_file(GLFW_STATIC_LIBRARY
        NAMES libglfw3.a
        PATHS ${GLFW_LIBRARY_DIRS}
        NO_DEFAULT_PATH
        REQUIRED
    )

    set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
    set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders")
    file(GLOB_RECURSE SHADER_ASSETS CONFIGURE_DEPENDS LIST_DIRECTORIES false
        "${SHADER_SOURCE_DIR}/*"
    )
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${SHADER_ASSETS})
    file(REMOVE_RECURSE "${SHADER_OUTPUT_DIR}")
    file(COPY "${SHADER_SOURCE_DIR}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/assets")

    set(MODEL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/models")
    set(MODEL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/models")
    file(GLOB_RECURSE MODEL_ASSETS CONFIGURE_DEPENDS LIST_DIRECTORIES false
        "${MODEL_SOURCE_DIR}/*"
    )
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${MODEL_ASSETS})
    file(REMOVE_RECURSE "${MODEL_OUTPUT_DIR}")
    file(COPY "${MODEL_SOURCE_DIR}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/assets")

    set(TEXTURE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures")
    set(TEXTURE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/textures")
    file(GLOB_RECURSE TEXTURE_ASSETS CONFIGURE_DEPENDS LIST_DIRECTORIES false
        "${TEXTURE_SOURCE_DIR}/*"
    )
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${TEXTURE_ASSETS})
    file(REMOVE_RECURSE "${TEXTURE_OUTPUT_DIR}")
    file(COPY "${TEXTURE_SOURCE_DIR}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/assets")

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
    target_include_directories(Graphics PRIVATE
        third_party/obj_loader
    )
    target_link_libraries(Graphics PUBLIC
        Shared
        "${GLFW_STATIC_LIBRARY}"
        glm::glm
        PkgConfig::STB
    )
    target_compile_definitions(Graphics PUBLIC
        GRAPHICS_ENABLED
        STB_IMAGE_IMPLEMENTATION
    )

    if(WIN32)
        target_link_libraries(Graphics PUBLIC opengl32 gdi32)
    endif()
endif()

if(BUILD_ENGINE AND BUILD_GRAPHICS)
    add_executable(EngineDemo
        src/main.cpp
        third_party/imgui/imgui.cpp
        third_party/imgui/imgui_draw.cpp
        third_party/imgui/imgui_tables.cpp
        third_party/imgui/imgui_widgets.cpp
        third_party/imgui/backends/imgui_impl_glfw.cpp
        third_party/imgui/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(EngineDemo PRIVATE
        third_party/imgui
        third_party/imgui/backends
    )
    target_link_libraries(EngineDemo PRIVATE Graphics Engine)

    if(MINGW)
        target_link_options(EngineDemo PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

if(BUILD_TESTING AND BUILD_ENGINE)
    add_executable(EngineTests tests/UnitTests.cpp)
    target_link_libraries(EngineTests PRIVATE Engine)

    if(BUILD_GRAPHICS)
        target_sources(EngineTests PRIVATE Graphics/src/Camera.cpp Graphics/src/GlfwInput.cpp)
        target_include_directories(EngineTests PRIVATE
            Graphics/include
            third_party/glad/include
            ${GLFW_INCLUDE_DIRS}
        )
        target_compile_definitions(EngineTests PRIVATE GRAPHICS_ENABLED)
    endif()

    add_test(NAME EngineTests COMMAND EngineTests)
endif()
