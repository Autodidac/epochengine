
<img align="left" src="Images/567.jpg" width="70px"/>


# AlmondShell

**AlmondShell** is a modern **C++20 software engine** that forms the foundation layer for the evolving AlmondEngine project.
It distils over **4,000 hours** of engineering effort invested into getting a multi-context, multi-threaded, fully featured atlas system working—resulting in a flexible runtime that can:

- Substitute AlmondEngine's Vulkan and DirectX layers with multiple 2D graphics back-end contexts.
- Host entire engines such as **SDL3**, **SFML**, and **Raylib** inside a multi-threaded, multi-context environment with fine-grained configuration control.
- Provide its own internal OpenGL, software, and no-op renderers alongside headless execution modes.
- Offer state-of-the-art networking via Steam servers with automatic ASIO fallback.

The runtime is designed for rapid iteration with hot-reloadable scripting, a self-updating launcher capable of downloading and building from source, and the low-level systems that power rendering, scripting, task scheduling, and asset pipelines. Editor automation currently lives in `src/scripts/`, which the runtime watches and reloads on demand.

---

## Architectural Pillars

- 🧱 **Static Linking First**  
  AlmondShell's runtime is delivered as a fully static target, ensuring deterministic deployment, predictable performance, and portability across distribution channels.
- 📚 **Header-Only Core**  
  The primary engine modules live in headers so they can be inlined, composed, and consumed without linker gymnastics, unlocking rapid iteration for integrators.
- 🧠 **Functional Flow**  
  Systems are composed in a functional style that favours pure interfaces and immutable data where possible, simplifying reasoning about complex runtime state.

---

## Current Snapshot (v0.62.0)

- ✅ **Immediate GUI everywhere** – `agui.hpp` seeds a shared font atlas, now covers buttons, image buttons, editable text fields, text boxes, and a turnkey console overlay so every backend renders consistent tooling widgets out of the box.
- ✅ **HiDPI-aware multi-context rendering** – Raylib and SDL3 cache framebuffer metrics and remap input coordinates so docked panes stay pixel-accurate after DPI or window changes.
- ✅ **Configurable menu layouts** – The runtime honours a `--menu-columns` cap to keep atlas-driven menus readable on constrained displays.
- ✅ **Robust hot reload** – The scripting pipeline emits `ScriptLoadReport` diagnostics, prunes completed jobs, and waits synchronously so tooling observes a deterministic post-reload state.

Refer to [`Changes/changelog.txt`](Changes/changelog.txt) for the full history of fixes and enhancements.

---

## Key Features

- 🔄 **Self-updating launcher**
  Designed to automatically fetch the newest release when run, or perform an on-demand check via `--update` before applying builds with `--force`.
  Can also be built directly from source for full control.

- ⚙️ **Modular C++20 engine**
  Built in a **functional, header-only style** with static linkage.
  Context-driven architecture with systems for rendering, windowing, input, scripting, tasks, and asset management.

- 🧪 **Live script reloading**
  Changes to `*.ascript.cpp` files are detected at runtime, recompiled with LLVM/Clang, and seamlessly reloaded.

- 🗂️ **Well-organised codebase**
  - Headers in `include/`
  - Implementation in `src/`
  - Helper scripts under `unix/` plus project-level `.sh` helpers

- 🖼️ **Sprite & atlas management**
  Global registries, unique atlas indexing, and atlas-driven GUI (buttons, highlights, and menus) backed by the multi-context atlas pipeline that has been refined over thousands of development hours.
- 🪟 **Built-in immediate GUI (`agui.hpp`)**
  Shared GUI primitives provision a font atlas automatically, upload it to Raylib and SDL backends, and expose ready-made buttons, text inputs, image buttons, and console widgets so every context can draw the same HUD elements out of the box.

- 🖥️ **Multi-context rendering**
  Pluggable backends: OpenGL, Raylib, SFML, and a software renderer — switchable via thunks and lambdas.
  **Multithreaded** with a state-of-the-art **hybrid coroutine + threaded design** for maximum scalability and efficiency.
  Docked child windows now display backend labels so it's obvious which renderer owns each pane when SDL and Raylib run side by side.

---

## Status

✅ **Actively Developed**
AlmondShell is under **active development** as the software engine base of AlmondEngine.
The current focus is **Phase 2 of the roadmap**: hardening the multi-context runtime, building renderer smoke coverage, and integrating automated reload diagnostics.
See [`Changes/roadmap.txt`](Changes/roadmap.txt) and [`AlmondShell/docs/engine_analysis.md`](AlmondShell/docs/engine_analysis.md) for the detailed plan and analysis notes.

---

## Repository Layout

```
.
├── LICENSE                  # LicenseRef-MIT-NoSell terms for AlmondShell
├── README.md                # Project overview and setup guide (this file)
├── AlmondShell/
│   ├── include/             # Core engine headers
│   ├── src/                 # Engine, updater entry point, and scripts
│   ├── docs/                # Engine analysis, configuration matrix, context audit, tooling notes
│   ├── examples/            # Sample projects and templates
│   └── CMakeLists.txt       # Build script for the updater target
├── AlmondShell.sln          # Visual Studio solution for Windows developers
├── Changes/                 # Roadmap and changelog snapshots
└── Images/                  # Non-code repository assets (logos, promo art, etc.)
    ├── 567.jpg
    └── almondshell.bmp
```

Refer to `AlmondShell/docs/file_structure.txt` for a more exhaustive tour of the available modules and helper tooling, and `AlmondShell/docs/tools_list.txt` for environment prerequisites.

## API Documentation

The repository ships with a [Doxygen](https://www.doxygen.nl/) configuration that indexes every public header beneath `AlmondShell/include/` and emits an HTML reference under `AlmondShell/docs/api/html/`.

- Run the standard build helper (`./AlmondShell/build.sh <gcc|clang> <Debug|Release>`) and the script will invoke the documentation target automatically whenever Doxygen is available on your system.
- Alternatively, generate the reference manually from an existing build tree via `cmake --build <build directory> --target docs`.
- Publish the resulting HTML directory (for example by pointing GitHub Pages at `docs/api/html/`) to keep the roadmap's documentation milestone marked as complete.

---

## Engine Configuration (`include/aengineconfig.hpp`)

AlmondShell centralises its build-time feature flags inside
[`include/aengineconfig.hpp`](AlmondShell/include/aengineconfig.hpp). The file
controls which entry points, contexts, and renderers are compiled into the
runtime:

- **Entry point selection** – Define `ALMOND_MAIN_HANDLED` if you provide your
  own `main` function. On Windows the engine automatically switches to
  `WinMain` (`ALMOND_USING_WINMAIN`) unless you opt into headless mode via
  `ALMOND_MAIN_HEADLESS`.
- **Debug toggles** – Commented-out switches such as `DEBUG_INPUT`,
  `DEBUG_TEXTURE_RENDERING_VERBOSE`, and related macros can be enabled when you
  need additional logging during development.
- **Window topology** – `ALMOND_SINGLE_PARENT` (enabled by default) tells the
  runtime to create child windows beneath a single parent. Set it to `0` to
  allow multiple top-level windows.
- **Context backends** – Enable or disable integrations like `ALMOND_USING_SDL`,
  `ALMOND_USING_SFML`, or `ALMOND_USING_RAYLIB` to embed those engines as
  AlmondShell contexts.
- **Rendering backends** – Choose one or more renderers. OpenGL is enabled out
  of the box via `ALMOND_USING_OPENGL`; additional options include
  `ALMOND_USING_SOFTWARE_RENDERER`, `ALMOND_USING_VULKAN`, and the placeholder
  `ALMOND_USING_DIRECTX` flag.
- **Raylib integration notes** – When `ALMOND_USING_RAYLIB` is enabled the
  configuration defines `RAYLIB_NO_WINDOW` and remaps select symbols (e.g.
  `CloseWindow`, `ShowCursor`, `LoadImageW`) before including `raylib.h` to avoid
  Windows header conflicts.

Review and adjust these switches before building to tailor the engine to your
toolchain and desired runtime footprint.

---

## Prerequisites

To build AlmondShell from source you will need the following tools:

| Requirement            | Notes |
| ---------------------- | ----- |
| A C++20 toolchain      | Visual Studio 2022, clang, or GCC 11+ are recommended. |
| CMake ≥ 3.21           | Used to generate build files and drive the provided presets. |
| Ninja _or_ MSBuild     | Pick the generator that matches your platform. |
| Git                    | Required for cloning the repository and fetching dependencies. |
| [vcpkg](https://vcpkg.io/) | Simplifies acquiring third-party libraries listed in `AlmondShell/vcpkg.json`. |
| Optional: Vulkan SDK   | Needed when working on Vulkan backends listed in `include/avulkan*`. |

### vcpkg manifest dependencies

AlmondShell ships with a [vcpkg manifest](AlmondShell/vcpkg.json) so that CMake can automatically fetch all required third-party
packages when you configure the project in manifest mode (`VCPKG_FEATURE_FLAGS=manifests`). Visual Studio 2022 picks this up automatically when you open the `AlmondShell` folder, restoring the dependencies during the first configure step (ensure **Tools → Options → CMake → vcpkg** has *Use vcpkg manifest mode* enabled if you have previously customised the setting). The manifest currently pulls in:

- [`asio`](https://think-async.com/) – Asynchronous networking primitives used by the updater and runtime services.
- [`fmt`](https://fmt.dev/) – Type-safe, fast formatting for logging and diagnostics.
- [`glad`](https://glad.dav1d.de/) – OpenGL function loader used by the OpenGL backend.
- [`glm`](https://github.com/g-truc/glm) – Mathematics library for vector and matrix operations.
- [`opengl`](https://www.khronos.org/opengl/) – OpenGL utility components supplied by vcpkg.
- [`raylib`](https://www.raylib.com/) – Optional renderer and tooling integrations.
- [`sfml`](https://www.sfml-dev.org/) – Additional windowing and multimedia support.
- [`sdl3`](https://github.com/libsdl-org/SDL) and [`sdl3-image`](https://github.com/libsdl-org/SDL_image) – Cross-platform window,
  input, and image loading.
- [`zlib`](https://zlib.net/) – Compression support for packaged assets and downloads.

If you are using a classic (non-manifest) vcpkg workflow, install the same packages manually before configuring CMake, for example:

```bash
vcpkg install asio fmt glad glm opengl raylib sfml sdl3 sdl3-image zlib
```

When CMake is configured with vcpkg integration enabled, the dependencies will be restored automatically on subsequent builds.

---

## Building from Source

Clone the repository and configure the build with your preferred toolchain:

```bash
git clone https://github.com/Autodidac/AlmondShell.git
cd AlmondShell
```

### Windows (MSVC)

```powershell
cmake --preset x64-release
cmake --build --preset x64-release
```

### Linux (Clang/GCC + Ninja)

```bash
cmake --preset Ninja-Release
cmake --build --preset Ninja-Release
```

### macOS (Clang + Ninja)

```bash
cmake --preset macos-release
cmake --build --preset macos-release
```

Each preset generates binaries under `AlmondShell/bin/<preset>/`. Switch to the corresponding `*-Debug` preset for debug symbols or override generator settings as needed.

---

## Running the Updater & Engine

On launch the updater:
1. Reads the remote configuration targets defined in `include/aupdateconfig.hpp` (for example the `include/config.hpp` manifest in the release repository).
2. Downloads and applies updates when available.
3. Starts the engine runtime, which in turn loads `src/scripts/editor_launcher.ascript.cpp` and watches for changes. Editing the script triggers automatic recompilation within the running session, with `ScriptLoadReport` diagnostics surfacing reload status in the console.

Release binaries are not currently published while the runtime is under active development. Build from source using the steps above until the release automation in the roadmap is complete.

Stop the session with `Ctrl+C` or by closing the console window.

---

## Development Tips

- The hot-reload loop in `src/main.cpp` monitors script timestamps roughly every 200 ms. Keep editor builds incremental to benefit from the fast feedback.
- Consult [`AlmondShell/docs/runtime_operations.md`](AlmondShell/docs/runtime_operations.md) for prerequisites, updater flow, reload diagnostics, and multi-context troubleshooting guidance.
- Utility shell scripts (`build.sh`, `run.sh`, `unix/*.sh`) can streamline development on POSIX systems.
- `AlmondShell/docs/engine_analysis.md` and `AlmondShell/docs/context_audit.md` catalogue current focus areas and cleanup candidates for future contributors.
- Check the remaining `docs/` entries for platform-specific setup guides, tool recommendations, and dependency notes.

---

## Contributing

"We Are Not Accepting PRs At This Time" as Almond is a source available commercial product

For substantial changes, open an issue first to discuss direction.

---

## License

The project uses the `LicenseRef-MIT-NoSell` license variant. See [`LICENSE`](LICENSE) for the full terms, including restrictions on commercial use and warranty disclaimers.


