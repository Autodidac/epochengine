# AlmondShell v0.70.3 Release Notes

## Summary
- Fixed Linux link failures by always linking against the standalone glad loader, matching the expectations baked into the shell scripts and VS Code tasks.
- Synced the documentation to reflect the unified loader strategy and noted the new version metadata for packagers.

## Upgrade Notes
- Reconfigure existing build directories (`cmake --fresh` or delete your cache) so the amended glad linkage is applied.
- Ensure distribution manifests include the glad artefacts since AlmondShell now depends on them across every configuration entry point.
