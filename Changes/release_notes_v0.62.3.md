# AlmondShell v0.62.3 Release Notes

## Highlights

- 🖼️ **Raylib viewport clipping restored** – The Raylib backend now scissor-clamps its fitted viewport using the same offsets that drive sprite placement. GUI overlays respect the letterboxed bounds again even when the window aspect ratio drifts far from the engine's reference resolution.
- 🧹 **Software renderer shutdown noise removed** – Soft renderer cleanup is now idempotent, preventing duplicate "Cleanup complete" logs and avoiding redundant teardown work when the multiplexer and engine both call into the backend.

## Fixes & Improvements

| Area | Description |
| --- | --- |
| Raylib backend | Applies a scissor rectangle that matches the fitted viewport, keeping atlas-driven sprites clipped precisely to the intended surface and eliminating the lingering placement drift. |
| Software renderer | Guards the cleanup path with an atomic flag so redundant shutdown requests turn into no-ops after the first pass, which keeps logs tidy and prevents double destruction. |

## Documentation

- Updated the README snapshot highlights and changelog to reflect the v0.62.3 fixes.

## Upgrade Notes

No breaking changes. Rebuild or relaunch with the updated binaries to pick up the viewport and cleanup fixes.
