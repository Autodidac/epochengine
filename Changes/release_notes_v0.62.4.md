# AlmondShell v0.62.4 Release Notes

## Highlights

- 🖼️ **Viewport bootstrap now matches the live framebuffer** – Raylib seeds its fitted viewport using the actual render target dimensions as soon as the window spins up, so GUI overlays stay aligned even while the child window is still docking into the multiplexer parent.

## Fixes & Improvements

| Area | Description |
| --- | --- |
| Raylib backend | Recomputes the fitted viewport from the active framebuffer whenever the context initialises or coalesces resizes while the window is already live, preventing the lingering GUI clipping seen immediately after startup. |

## Documentation

- Logged the viewport bootstrap change in the configuration guide, changelog, and README snapshot.

## Upgrade Notes

No breaking changes. Rebuild or relaunch to pick up the corrected Raylib viewport bootstrap.
