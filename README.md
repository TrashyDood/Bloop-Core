# Bloop Core Engine

A work-in-progress C++17 OpenGL demo with a GLFW window, GLAD, GLM, and Dear ImGui. It renders animated cubes and pyramids, with a flying camera, keyboard/mouse input, and an in-app panel for camera, display, and frame-rate settings. The input handler supports typed keyboard and mouse codes, including wildcard bindings.

Controls: WASD to move, Q/E vertically, mouse buttons to orbit, scroll to zoom, F for wireframe, G for orthographic view, V for VSync, R to reset the camera, and Esc to exit.

## Build and run

This project uses CMake and is configured for MSYS2 UCRT64 on Windows.

Graphics are disabled by default. Configure with `-DENABLE_GRAPHICS=ON` to build the GLFW/OpenGL `Graphics` library and demo. `Graphics` depends on `Shared`, not `EngineCore`; the demo composes `Graphics` and `EngineCore`. `EngineCore` owns scene timing and lifecycle, with optional hooks for platform event processing and presentation. Without graphics, CMake builds `Shared`, `EngineCore`, and the core tests.

1. Install the recommended VS Code extensions when prompted.
2. Set `msys2.root` in your VS Code user settings to your MSYS2 installation folder (for example, `C:\msys2`). No environment variable or global `PATH` change is required.
3. Run **MSYS2: Set Up Environment** once to install the required tools and libraries.
4. Run **MSYS2: Sync CMake Tools Paths** to populate the CMake executable and `PATH` in workspace settings from `msys2.root`. Run it again if you change the MSYS2 location.
5. Run **CMake: Configure** to configure the `default` preset.
6. Press **Ctrl+Shift+B** or run **CMake: Build** to build.
7. Run **Run Engine** to build and launch the selected CMake launch target.

## Run unit tests

Run **CMake: Run Tests** to build the project and run the tests from the `default` CTest preset. CMake Tools also exposes the discovered tests in its Test Explorer.
