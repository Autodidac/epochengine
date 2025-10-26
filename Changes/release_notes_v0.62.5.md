# AlmondShell v0.62.5 Release Notes

## Highlights

- 🛠️ **Raylib standalone stability** – The Raylib backend now retains the exact WGL device and context handles it bootstraps with, only re-binding them when another backend has stolen focus so running it as the sole context no longer fails to activate OpenGL.

## Fixes & Improvements

| Area | Description |
| --- | --- |
| Raylib backend | Cache the live WGL device/context pair on initialisation and skip redundant `wglMakeCurrent` calls unless a different backend has claimed the thread, preventing activation failures when Raylib is the only running context. |

## Documentation

- Recorded the Raylib context fix in the configuration guide, changelog, and README snapshot.

## Upgrade Notes

No action required beyond rebuilding or relaunching the runtime to pick up the Raylib context activation fix.
