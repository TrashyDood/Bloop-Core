# Engine

This project opens a GLFW window with an OpenGL 3.3 Core context and renders a
colored 2D house by repeatedly drawing one immutable triangle buffer. Press
Space to change the roof color. The vertex and fragment shaders are separate
files in `shaders/` and are copied beside the executable during configuration.

## Build on this PC

The repository includes an optional **MSYS2 UCRT64** terminal profile in VS
Code. Add the MSYS2 UCRT64 `bin` directory (for example, `C:\msys64\ucrt64\bin`)
to your Windows `PATH`, then fully restart VS Code. This lets CMake Tools find
GCC, CMake, and Ninja. Open **Terminal → New Terminal** and select
**MSYS2 UCRT64** to start a shell with the UCRT64 environment.

For a new MSYS2 installation, choose **Terminal → Run Task… → Update MSYS2**.
If the update asks you to close the terminal, restart VS Code and run the task
again. Then choose **Terminal → Run Task… → Install Build Dependencies** to
install GCC, CMake, Ninja, pkg-config, and GLFW.

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

The terminal profile and tasks derive the MSYS2 root from the `ucrt64\bin`
entry on `PATH` to launch `msys2_shell.cmd`; you do not need to add the root
separately. This also lets the install-dependencies task work before GCC is
installed. The `default` preset in `CMakePresets.json` selects GCC and Ninja;
no compiler flags or extra arguments are needed. CMake writes generated build
files under `build/`.

## Where the libraries come from

- **MSYS2** provides the compiler, CMake, Ninja, and GLFW. The **Update MSYS2**
  and **Install Build Dependencies** tasks manage them from its UCRT64 package
  repository.
- **GLAD** is already included under `third_party/glad`. Its OpenGL 3.3 loader
  was generated from Khronos specifications and is built with the project;
  you do not need to download or install GLAD separately.
