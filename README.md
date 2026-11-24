# Bloop Core Engine

A work-in-progress C++17 OpenGL demo with a GLFW window, GLAD, and GLM. It renders animated cubes and pyramids, with a flying camera and keyboard/mouse input. When enabled, Dear ImGui provides an in-app panel for camera, display, and frame-rate settings. The input handler supports typed keyboard and mouse codes, including wildcard bindings.

Controls: Player 1 uses WASD to move and Q/E to rotate; Player 2 uses keypad 8/4/2/6 to move and keypad 7/9 to rotate. Esc exits.

## Build and run

This project uses CMake and is configured for MSYS2 UCRT64 on Windows.

The default VS Code CMake preset builds both the `Engine` and `Graphics` libraries, plus the `EngineDemo` GLFW/OpenGL application. Both libraries use `Shared` for common logic. Set `BUILD_ENGINE` and `BUILD_GRAPHICS` independently to choose which libraries to build. Dear ImGui is built alongside `Graphics` only when `BUILD_GRAPHICS` and `BUILD_IMGUI` are enabled; `EngineDemo` requires both `BUILD_ENGINE` and `BUILD_GRAPHICS` and uses ImGui when enabled. The default preset enables `BUILD_IMGUI`, which otherwise defaults to `OFF`. Graphics dependencies are installed by the setup task below. The `Engine` library owns scene timing and lifecycle, with optional hooks for platform event processing and presentation.

1. Install the recommended VS Code extensions when prompted.
2. Set `msys2.root` in your VS Code user settings to your MSYS2 installation folder (for example, `C:\msys2`). No environment variable or global `PATH` change is required.
3. Run **MSYS2: Set Up Environment** once to install the required tools and libraries.
4. Run **MSYS2: Sync CMake Tools Paths** to populate the CMake executable and `PATH` in workspace settings from `msys2.root`. Run it again if you change the MSYS2 location.
5. Run **CMake: Configure** to configure the `default` preset.
6. Press **Ctrl+Shift+B** or run **CMake: Build** to build.
7. Run **Run Engine** to build and launch the selected CMake launch target.

## Run unit tests

Run **CMake: Run Core Tests** to configure, build, and run the core unit tests without requiring Graphics or GLFW. From a terminal, use:

```powershell
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

The existing **CMake: Run Tests** task still runs tests with the `default` preset, which also builds Graphics and includes its input/camera tests when enabled.
