module; // REQUIRED global module fragment (macros + platform headers live here)

// ============================================================
// Almond Entry Point / Platform Configuration
// ============================================================

// Almond Entry Point No-Op
// #define ALMOND_MAIN_HEADLESS

#ifndef ALMOND_MAIN_HANDLED
#ifndef ALMOND_MAIN_HEADLESS
#ifdef _WIN32
#define ALMOND_USING_WINMAIN
#endif
#endif
#endif

// ------------------------------------------------------------
// Debug / inspection toggles (unchanged)
// ------------------------------------------------------------
// #define RUN_CODE_INSPECTOR
// #define DEBUG_INPUT
// #define DEBUG_INPUT_OS
// #define DEBUG_MOUSE_MOVEMENT
// #define DEBUG_MOUSE_COORDS
// #define DEBUG_TEXTURE_RENDERING_VERBOSE
// #define DEBUG_TEXTURE_RENDERING_VERY_VERBOSE
// #define DEBUG_WINDOW_VERBOSE

// ------------------------------------------------------------
// Engine Context Config
// ------------------------------------------------------------

#define ALMOND_SINGLE_PARENT 1

#define ALMOND_USING_SDL 1
// #define ALMOND_USING_RAYLIB 1
#define ALMOND_USING_SOFTWARE_RENDERER 1
#define ALMOND_USING_OPENGL 1

#if defined(ALMOND_FORCE_DISABLE_SDL)
#undef ALMOND_USING_SDL
#endif
#if defined(ALMOND_FORCE_ENABLE_SDL)
#undef ALMOND_USING_SDL
#define ALMOND_USING_SDL 1
#endif

#if defined(ALMOND_FORCE_DISABLE_RAYLIB)
#undef ALMOND_USING_RAYLIB
#endif
#if defined(ALMOND_FORCE_ENABLE_RAYLIB)
#undef ALMOND_USING_RAYLIB
#define ALMOND_USING_RAYLIB 1
#endif

#if defined(ALMOND_FORCE_DISABLE_SOFTWARE_RENDERER)
#undef ALMOND_USING_SOFTWARE_RENDERER
#endif
#if defined(ALMOND_FORCE_ENABLE_SOFTWARE_RENDERER)
#undef ALMOND_USING_SOFTWARE_RENDERER
#define ALMOND_USING_SOFTWARE_RENDERER 1
#endif

#if defined(ALMOND_FORCE_DISABLE_OPENGL)
#undef ALMOND_USING_OPENGL
#endif
#if defined(ALMOND_FORCE_ENABLE_OPENGL)
#undef ALMOND_USING_OPENGL
#define ALMOND_USING_OPENGL 1
#endif

// ============================================================
// Includes (verbatim, order preserved)
// ============================================================

#ifdef ALMOND_USING_SFML
#define SFML_STATIC
#include <SFML/Graphics.hpp>
#endif

#ifdef ALMOND_USING_WINMAIN
#include "../include/aframework.hpp"
#endif

#ifdef ALMOND_USING_RAYLIB
#include <glad/glad.h>

#if defined(_WIN32)
#include <GL/wglext.h>
#endif

#define RAYLIB_NO_WINDOW
#define RAYLIB_STATIC

#define CloseWindow Raylib_CloseWindow
#define ShowCursor Raylib_ShowCursor
#define LoadImageW Raylib_LoadImageW
#define DrawTextW Raylib_DrawTextW
#define DrawTextExW Raylib_DrawTextExW
#define Rectangle Raylib_Rectangle
#define PlaySoundW Raylib_PlaySoundW

#include <raylib.h>

#undef CloseWindow
#undef ShowCursor
#undef LoadImageW
#undef DrawTextW
#undef DrawTextExW
#undef Rectangle
#undef PlaySoundW
#endif

#ifdef ALMOND_USING_OPENGL
#include <glad/glad.h>

#if defined(_WIN32)
#if defined(__has_include) && __has_include(<glad/glad_wgl.h>)
#include <glad/glad_wgl.h>
#elif defined(__has_include) && __has_include(<GL/wglext.h>)
#include <GL/wglext.h>
#else
#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#endif
#ifndef WGL_CONTEXT_MINOR_VERSION_ARB
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#endif
#ifndef WGL_CONTEXT_PROFILE_MASK_ARB
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#endif
#ifndef WGL_CONTEXT_CORE_PROFILE_BIT_ARB
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif
#ifndef PFNWGLCREATECONTEXTATTRIBSARBPROC
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
#endif
#endif
#endif
#endif

#ifdef ALMOND_USING_SDL
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_version.h>

#ifndef ALMOND_MAIN_HEADLESS
#define SDL_MAIN_HANDLED
#endif

#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_opengl.h>
#endif

#ifdef ALMOND_USING_VOLK
#define VK_NO_PROTOTYPES
#include <volk.h>
#define ALMOND_USING_GLM
#endif

#ifdef ALMOND_USING_VULKAN_WITH_GLFW
#define GLFW_INCLUDE_VULKAN
#define ALMOND_USING_VULKAN
#endif

#ifdef ALMOND_USING_GLM
#include <glm/glm.hpp>
#endif

#ifdef ALMOND_USING_VULKAN
#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif
#endif

// ============================================================
// Named module (empty export surface by design)
// ============================================================

export module aengine.config;
