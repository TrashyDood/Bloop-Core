# Engine

This project opens a GLFW window with an OpenGL 3.3 Core context and renders a
colored 2D house by repeatedly drawing one immutable triangle buffer. Press
Space to change the roof color. The vertex and fragment shaders are separate
files in `shaders/` and are copied beside the executable during the build.

## Build on this PC

The repository includes an **MSYS2 UCRT64** terminal profile and build tasks
for VS Code. Set the `msys2.root` setting once in your VS Code **User
Settings (JSON)**:

```json
{
  "msys2.root": "C:\\msys64"
}
```

Set the value to the MSYS2 installation directory containing
`msys2_shell.cmd`. You do not need to add MSYS2 to your Windows `PATH`. The
setting is personal and should not be added to the workspace settings file.
Open **Terminal → New Terminal** and select **MSYS2 UCRT64** to start a shell
with the UCRT64 environment.

For a new MSYS2 installation, choose **Terminal → Run Task… → MSYS2: Set Up
Environment**. It updates MSYS2 and installs GCC, CMake, Ninja,
pkg-config, GLFW, GLM, and the GLAD generator. If the update asks you to close
the terminal, restart VS Code and run the setup task again.

To build in VS Code, choose **Terminal → Run Build Task** (or press
**Ctrl+Shift+B**) and select **MSYS2: Build Engine**. The task configures the
CMake preset first, then builds the executable. To build and launch the window,
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

The terminal profile and MSYS2 tasks launch `msys2_shell.cmd` directly using
your `msys2.root` setting. CMake Tools automatic configuration is disabled so
configuration and builds happen inside that environment via VS Code tasks.
The `default` preset selects GCC and Ninja. CMake links GLFW and the GCC
runtimes statically, so **Run Engine** launches the executable directly; MSYS2
is not needed at runtime. Windows system DLLs, including UCRT, remain runtime
dependencies. CMake writes generated build files under `build/`; IntelliSense
reads the generated `build/compile_commands.json`.

## Where the libraries come from

- **MSYS2** provides the compiler, CMake, Ninja, GLFW, and GLM. CMake links
  GLM through its `glm::glm` target. The **MSYS2: Update** and
  **MSYS2: Install Dependencies** tasks manage them from its UCRT64 package
  repository.
- **GLAD** is already included under `third_party/glad`. Its OpenGL 3.3 loader
  was generated from Khronos specifications and is built with the project.
  The **MSYS2: Install Dependencies** task also installs the GLAD generator
  if you need to regenerate those files; choose **Terminal → Run Task… →
  MSYS2: Generate GLAD** to regenerate them. This overwrites the vendored
  loader files.
