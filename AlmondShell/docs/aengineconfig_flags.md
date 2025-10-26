# AlmondShell Engine Configuration Flags

This guide summarises the build-time switches exposed from `include/aengineconfig.hpp` and clarifies which combinations are currently supported. Use it as a quick reference before enabling or disabling backends in multi-context builds.

## Entry-Point Controls

| Macro | Default | Purpose | Notes |
| --- | --- | --- | --- |
| `ALMOND_MAIN_HEADLESS` | Off | Opts out of AlmondShell's automatic entry point when embedding the runtime in a host application. | Define when you provide your own `main` and do **not** want the runtime to spawn windows or contexts. |
| `ALMOND_MAIN_HANDLED` | Off | Signals that the consumer will provide a platform-specific entry point. | On Windows the engine falls back to `WinMain` automatically when neither headless nor handled macros are defined. |
| `ALMOND_USING_WINMAIN` | Auto | Windows-only helper enabling the Win32 subsystem entry point. | Set implicitly on `_WIN32` unless the build is explicitly headless. |

## Context Layout

| Macro | Default | Purpose | Supported Values |
| --- | --- | --- | --- |
| `ALMOND_SINGLE_PARENT` | `1` | Indicates whether child contexts share a single top-level parent window. | `1` keeps multi-backend layouts contained inside the main shell window. Set to `0` only when running fully detached platform windows. |

## Backend Availability

| Category | Macro | Default | Support Notes |
| --- | --- | --- | --- |
| Context provider | `ALMOND_USING_SDL` | On | SDL3 context backed by the runtime multiplexer. Works alongside Raylib for tooling scenarios. |
| Context provider | `ALMOND_USING_RAYLIB` | On | Raylib context hosted inside the multiplexer. Disabled only when building ultra-minimal shells. |
| Context provider | `ALMOND_USING_SFML` | Off | SFML support is intentionally disabled until upstream releases a stable v3.0. |
| Renderer | `ALMOND_USING_SOFTWARE_RENDERER` | On | CPU rasteriser used as a fallback and validation layer. |
| Renderer | `ALMOND_USING_OPENGL` | On | Primary GPU renderer. Required for editor-style workflows. |
| Renderer | `ALMOND_USING_VULKAN` | Off | Stubbed configuration. Enabling requires matching updates in the examples and is not supported in AlmondShell today. |
| Renderer | `ALMOND_USING_DIRECTX` | Off | Reserved for AlmondEngine integration. |

## Supported Combinations

The runtime ships with SDL + Raylib contexts active simultaneously, backed by both the OpenGL and software renderers. This configuration is validated by the multiplexer and keeps asset streaming consistent across backends.

Supported mixes:

- **Default multi-backend** – SDL + Raylib contexts with OpenGL and software renderers enabled (`ALMOND_SINGLE_PARENT == 1`).
- **Headless tooling** – Define `ALMOND_MAIN_HEADLESS` and keep renderer macros enabled when running asset generation or script compilation pipelines.
- **Single-context builds** – Undefine either `ALMOND_USING_SDL` or `ALMOND_USING_RAYLIB` when targeting constrained platforms. At least one renderer macro must remain defined for the runtime to upload atlases.

Unsupported mixes:

- **SFML, Vulkan, or DirectX** – These paths are stubbed and intentionally disabled until their respective integrations stabilise.
- **Renderer-less builds** – Disabling both `ALMOND_USING_OPENGL` and `ALMOND_USING_SOFTWARE_RENDERER` prevents the texture managers from initialising and is not supported.

## Change Log

- **v0.62.5** – Logged that Raylib now caches its original WGL handles and only reacquires the context on demand so single-backend builds no longer fail to activate the GL context.
- **v0.62.4** – Recorded that Raylib now seeds its fitted viewport from the live framebuffer during creation so docked GUIs align immediately.
- **v0.62.3** – Noted the Raylib viewport scissor fix and the idempotent software renderer cleanup.
- **v0.62.2** – Clarified that forcing the updater also downloads and expands the source tree so release binaries can bootstrap local development without a fresh clone.
- **v0.62.0** – Documented the `--update`/`--force` flow so command-line runs can check for updates without automatically applying them.
- **v0.58.2** – First publication of this guide in conjunction with the roadmap milestone for documenting configuration options.
