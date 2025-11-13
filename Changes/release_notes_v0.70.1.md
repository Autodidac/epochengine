# AlmondShell v0.70.1 Release Notes

## Summary
- Fixed Linux builds by always linking the glad OpenGL loader so Raylib-enabled configurations can resolve `gladLoadGLLoader`.
- Refreshed Linux build instructions with explicit manifest bootstrap and Ninja-based configure steps.
- Bumped the engine version metadata to v0.70.1 so the updater and diagnostics report the new revision.

## Upgrade Notes
- Ensure vcpkg manifest mode is enabled (or install `glad`, `raylib`, `sdl3`, and friends manually) before running CMake.
- Reconfigure existing build directories after pulling the update so the amended glad linkage is applied.
- Regenerate documentation (`cmake --build <build> --target docs`) if you publish the API reference, as the new version number is emitted in the output.
