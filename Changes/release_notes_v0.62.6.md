# AlmondShell v0.62.6 Release Notes

## Highlights

- 🖼️ **Child-window scaling alignment** – Raylib child panes now stay locked to
  their intended design canvas while contexts rebound, and new smoke coverage
  exercises context reacquisition so the GL context stays bound after scaling
  churn.

## Fixes & Improvements

- **Raylib backend** – Documented the child-window scaling alignment work and
  staged context reacquisition smoke coverage so docked panes stay on their
  design canvas after resizes.

## Documentation

- Updated the README snapshot and engine analysis to capture the child-window
  scaling alignment and context reacquisition smoke coverage focus areas.

## Upgrade Notes

No manual action is required—pull the release and rebuild to pick up the
refreshed Raylib behaviour and documentation.
