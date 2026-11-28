cmake_minimum_required(VERSION 3.25)

project(Engine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_ENGINE "Build the engine library" ON)
option(BUILD_GRAPHICS "Build the graphics library" ON)
option(BUILD_IMGUI "Build Dear ImGui alongside Graphics" ON)

include(FetchContent)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.5.1
)
FetchContent_MakeAvailable(tomlplusplus)

find_package(glm CONFIG REQUIRED)

file(GLOB TOOLS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Tools/src/*.cpp"
)
add_library(Tools STATIC ${TOOLS_SOURCES})
target_include_directories(Tools PUBLIC
    Source/Tools/include
)
target_compile_definitions(Tools PUBLIC LOGGING_ENABLED)

if(WIN32)
    target_link_libraries(Tools PUBLIC winmm)
endif()

file(GLOB SHARED_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/Shared/src/*.cpp"
)
add_library(Shared STATIC ${SHARED_SOURCES})
target_include_directories(Shared PUBLIC
    Source/Shared/include
)
target_link_libraries(Shared PUBLIC Tools glm::glm)

if(BUILD_ENGINE)
    file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/src/*.cpp"
    )
    add_library(Engine STATIC ${ENGINE_SOURCES})
    target_include_directories(Engine PUBLIC Source/Engine/include)

    target_link_libraries(Engine PUBLIC Shared tomlplusplus::tomlplusplus)
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

    foreach(ASSET_DIR IN ITEMS shaders models textures)
        set(ASSET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/content/${ASSET_DIR}")
        set(ASSET_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/content/${ASSET_DIR}")
        file(GLOB_RECURSE ASSET_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false
            "${ASSET_SOURCE_DIR}/*"
        )
        file(REMOVE_RECURSE "${ASSET_OUTPUT_DIR}")
        file(COPY "${ASSET_SOURCE_DIR}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/content")
    endforeach()

    file(GLOB_RECURSE GRAPHICS_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Graphics/src/*.cpp"
    )

    add_library(Graphics STATIC
        ${GRAPHICS_SOURCES}
        Source/third_party/glad/src/gl.c
    )
    target_include_directories(Graphics PUBLIC
        Source/Graphics/include
        Source/Graphics/include/Resources
        Source/third_party/glad/include
        ${GLFW_INCLUDE_DIRS}
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

    if(BUILD_IMGUI)
        target_compile_definitions(Graphics PRIVATE IMGUI_ENABLED)
        target_include_directories(Graphics PRIVATE
            Source/third_party/imgui
            Source/third_party/imgui/backends
        )

        if(WIN32)
            target_link_libraries(Graphics PUBLIC psapi)
        endif()

        add_library(ImGui STATIC
            Source/third_party/imgui/imgui.cpp
            Source/third_party/imgui/imgui_draw.cpp
            Source/third_party/imgui/imgui_tables.cpp
            Source/third_party/imgui/imgui_widgets.cpp
            Source/third_party/imgui/backends/imgui_impl_glfw.cpp
            Source/third_party/imgui/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories(ImGui PUBLIC
            Source/third_party/imgui
            Source/third_party/imgui/backends
        )
        target_link_libraries(ImGui PUBLIC Graphics)
    endif()
endif()

if(BUILD_ENGINE AND BUILD_GRAPHICS)
    add_executable(EngineDemo
        Application/main.cpp
        Application/AssetResourceManager.cpp
        Application/DemoScene.cpp
    )
    target_link_libraries(EngineDemo PRIVATE Graphics Engine)

    if(BUILD_IMGUI)
        target_link_libraries(EngineDemo PRIVATE ImGui)
        target_compile_definitions(EngineDemo PRIVATE IMGUI_ENABLED)
    endif()

    if(MINGW)
        target_link_options(EngineDemo PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()

