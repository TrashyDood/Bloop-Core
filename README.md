# Engine

This project opens a GLFW window with an OpenGL 3.3 Core context and renders a
colored 2D house by repeatedly drawing one immutable triangle buffer. Press
Space to change the roof color; press Escape to close the window. The vertex
and fragment shaders are separate files in `shaders/` and are copied beside
the executable during configuration.

## Build on this PC

The repository includes an optional **MSYS2 UCRT64** terminal profile in VS
Code. To use it, set the Windows user environment variable `MSYS2_ROOT` to
your MSYS2 installation directory (the directory containing `usr\bin\bash.exe`),
then fully close and reopen VS Code. For example, if MSYS2 is installed at
`C:\msys64`, set `MSYS2_ROOT` to `C:\msys64`. The path is your local setting;
it is not stored in the repository. Open **Terminal → New Terminal** and
select **MSYS2 UCRT64**. Your usual terminal remains the default.

If the tools or GLFW are missing, install them once:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-glfw
```

To build in VS Code, choose **Terminal → Run Build Task** (or press
**Ctrl+Shift+B**) and select **Build Engine**. The task configures the CMake
preset first, then builds the executable. To build and launch the window,
choose **Terminal → Run Task… → Run Engine**.

To assign a keyboard shortcut to **Run Engine**, open the Command Palette,
choose **Preferences: Open Keyboard Shortcuts (JSON)**, and add this entry to
your personal `keybindings.json`:

```json
{
  "key": "ctrl+alt+f5",
  "command": "workbench.action.tasks.runTask",
  "args": "Run Engine"
}
```

Keyboard shortcuts are stored in your VS Code user settings, not in this
workspace, so this shortcut is personal and is not shared with other clones.

The tasks use the `MSYS2_ROOT` user environment variable and the `default`
preset in `CMakePresets.json`. The preset selects GCC and Ninja; no compiler
flags or extra arguments are needed. CMake writes generated build files under
`build/`.

## Where the libraries come from

- **MSYS2** provides the compiler, CMake, Ninja, and GLFW. The `pacman` command
  above installs GLFW from MSYS2's UCRT64 package repository.
- **GLAD** is already included under `third_party/glad`. Its OpenGL 3.3 loader
  was generated from Khronos specifications and is built with the project;
  you do not need to download or install GLAD separately.
