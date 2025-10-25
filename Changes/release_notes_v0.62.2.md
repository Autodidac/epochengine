# AlmondShell v0.62.2 Release Notes

## Highlights
- Seeds Raylib's letterbox viewport on the resize dispatch thread so menus start at the correct scale instead of clipping along the right edge.
- Keeps GUI hit testing stable while you resize the window by removing the one-frame drift that previously nudged buttons out of alignment.
- Synchronises the bundled version metadata and README snapshot with the new release.

## Known Issues
- Automated renderer regression scenes remain under development; see `docs/renderer_regression_plan.md` for the intended coverage.
- Crash reporting hooks have not landed yet, so crashes must be reproduced locally with a debugger attached.

## Roadmap Alignment
This release continues the Phase 2 renderer hardening effort by locking Raylib's viewport transforms to the latest resize metrics, preventing UI regressions across DPI and aspect changes.
