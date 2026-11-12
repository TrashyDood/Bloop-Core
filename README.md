# Bloop Core Engine

A work-in-progress C++17 OpenGL demo with a GLFW window, GLAD, GLM, and Dear ImGui. It renders animated cubes and pyramids, with a flying camera, keyboard/mouse input, and an in-app panel for camera, display, and frame-rate settings. The input handler supports typed keyboard and mouse codes, including wildcard bindings.

Controls: WASD to move, Q/E vertically, mouse buttons to orbit, scroll to zoom, F for wireframe, G for orthographic view, V for VSync, R to reset the camera, and Esc to exit.

## Build and run

This project uses CMake and is configured for MSYS2 UCRT64 on Windows.

1. Install the recommended VS Code extensions when prompted.
2. Set `msys2.root` in your VS Code user settings to your MSYS2 installation folder (for example, `C:\msys2`). CMake Tools uses that installation directly; no environment variable or global `PATH` change is required. If MSYS2 is installed elsewhere, update `cmake.cmakePath` and the MSYS2 `PATH` entry in `.vscode/settings.json` to match.
3. Run **MSYS2: Set Up Environment** once to install the required tools and libraries.
4. Run **CMake: Configure** to configure the `default` preset.
5. Press **Ctrl+Shift+B** or run **CMake: Build** to build.
6. Run **Run Engine** to build and launch the selected CMake launch target.

## Run unit tests

Run **CMake: Run Tests** to build the project and run the tests from the `default` CTest preset. CMake Tools also exposes the discovered tests in its Test Explorer.
