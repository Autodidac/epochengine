
<img align="left" src="images/567.jpg" width="70px"/>


# AlmondShell

**AlmondShell** is a modern **C++20 software engine** that forms the foundation layer for the evolving AlmondEngine project.
It focuses on delivering a flexible runtime that can:

- Substitute AlmondEngine's Vulkan and DirectX layers with multiple 2D graphics back-end contexts.
- Host entire engines such as **SDL3**, **SFML**, and **Raylib** inside a multi-threaded, multi-context environment with fine-grained configuration control.
- Provide its own internal OpenGL, software, and no-op renderers alongside headless execution modes.
- Offer state-of-the-art networking via Steam servers with automatic ASIO fallback.

The runtime is designed for rapid iteration with hot-reloadable scripting, a self-updating launcher capable of downloading and building from source, and the low-level systems that power rendering, scripting, task scheduling, and asset pipelines. Editor automation currently lives in `src/scripts/`, which the runtime watches and reloads on demand.

- **End users** can download the prebuilt binary, drop it into an empty directory, and let AlmondShell populate the latest files automatically.

---

## Key Features

- 🔄 **Self-updating launcher**  
  Designed to automatically fetch the newest release when run, ensuring users always stay up to date.  
  Can also be built directly from source for full control.  
  *(Currently disabled while under active development.)*  

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
  Global registries, unique atlas indexing, and atlas-driven GUI (buttons, highlights, and menus).  

- 🖥️ **Multi-context rendering**  
  Pluggable backends: OpenGL, Raylib, SFML, and a software renderer — switchable via thunks and lambdas.  
  **Multithreaded** with a state-of-the-art **hybrid coroutine + threaded design** for maximum scalability and efficiency.  

---

## Status

✅ **Actively Developed**  
AlmondShell is under **active development** as the software engine base of AlmondEngine.  
It continues to evolve as the **core foundation layer**, ensuring speed, modularity, and cross-platform compatibility with a **static, header-only functional design**.

---

## Repository Layout

```
.
├── LICENSE                  # LicenseRef-MIT-NoSell terms for AlmondShell
├── README.md                # Project overview and setup guide (this file)
├── AlmondShell/
│   ├── include/             # Core engine headers
│   ├── src/                 # Engine, updater entry point, and scripts
│   ├── docs/                # Supplementary documentation and setup notes
│   ├── examples/            # Sample projects and templates
│   └── CMakeLists.txt       # Build script for the updater target
├── AlmondShell.sln          # Visual Studio solution for Windows developers
└── images/                  # Non-code repository assets (logos, promo art, etc.)
    ├── 567.jpg
    └── almondshell.bmp
```

Refer to `AlmondShell/docs/file_structure.txt` for a more exhaustive tour of the available modules.

---

## Prerequisites

To build AlmondShell from source you will need the following tools:

| Requirement            | Notes |
| ---------------------- | ----- |
| A C++20 toolchain      | Visual Studio 2022, clang, or GCC 11+ are recommended. |
| CMake ≥ 3.10           | Used to generate build files. |
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
cmake -B build -S . -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Linux & macOS (Clang/GCC + Ninja)

```bash
cmake -B build -S . -G Ninja
cmake --build build
```

Both configurations produce the `updater` executable inside the `build` directory. Adjust `--config` or the generator settings to create Debug builds when needed.

---

## Running the Updater & Engine

### Using a Release Binary
1. Create an empty working directory.
2. Download the appropriate `almondshell` binary from the [Releases](https://github.com/Autodidac/AlmondShell/releases) page.

### Running From Source

On launch the updater:
1. Reads the remote configuration targets defined in `include/aupdateconfig.hpp` (for example the `include/config.hpp` manifest in the release repository).
2. Downloads and applies updates when available.
3. Starts the engine runtime, which in turn loads `src/scripts/editor_launcher.ascript.cpp` and watches for changes. Editing the script triggers automatic recompilation within the running session.

Stop the session with `Ctrl+C` or by closing the console window.

---

## Development Tips

- The hot-reload loop in `src/main.cpp` monitors script timestamps roughly every 200 ms. Keep editor builds incremental to benefit from the fast feedback.
- Utility shell scripts (`build.sh`, `run.sh`, `unix/*.sh`) can streamline development on POSIX systems.
- Check the `docs/` folder for platform-specific setup guides, tool recommendations, and dependency notes.

---

## Contributing

"We Are Not Accepting PRs At This Time" as Almond is a source available commercial product

For substantial changes, open an issue first to discuss direction.

---

## License

The project uses the `LicenseRef-MIT-NoSell` license variant. See [`LICENSE`](LICENSE) for the full terms, including restrictions on commercial use and warranty disclaimers.


