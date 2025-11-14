# AlmondShell v0.70.2 Release Notes

## Summary
- Resolved duplicate symbol definitions emitted by mixed glad/Raylib builds so linkers no longer complain when both loaders ship together.
- Relaxed the CMake logic around loader selection so packagers can opt into the standalone glad artefacts without breaking Raylib's defaults.
- Bumped the engine version metadata to v0.70.2 for diagnostics and distribution manifests.

## Upgrade Notes
- Regenerate build trees with `cmake --fresh` or remove your cache to pick up the corrected glad toggles.
- Clean rebuild or relink any custom launchers that bundled the engine static libraries so they incorporate the duplicate-symbol fix.
