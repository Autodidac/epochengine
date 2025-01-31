#pragma once

#ifdef _WIN32 // Automatically use WinMain on Windows
	#define ALMOND_USING_WINMAIN // if using WinMain instead of int main
#endif

// --------------------
// Engine Context Config
// --------------------

// Rendering backend (Uncomment ONLY ONE)
//#define ALMOND_USING_OPENGL
#define ALMOND_USING_VULKAN  // You must also set the context in the example
//#define ALMOND_USING_DIRECTX  // Currently Not Supported In AlmondShell, See AlmondEngine...

// Windowing backend (Uncomment ONLY ONE)
#define ALMOND_USING_GLFW
//#define ALMOND_USING_VOLK
//#define ALMOND_USING_SDL

//////////////////////////////////////// Do Not Edit/Configure Below This Line //////////////////////////////////////////////////
// 
// 
// 
// 
// 
// 
// 
// 
// --------------------
// Include Libraries
// --------------------
#ifdef ALMOND_USING_WINMAIN
	#include "aframework.h"
#endif

#ifdef ALMOND_USING_OPENGL
	#include <glad/glad.h>
#endif

#ifdef ALMOND_USING_VOLK
	#define VK_NO_PROTOTYPES
	#include <volk.h>
	#define ALMOND_USING_GLM
#endif

#ifdef ALMOND_USING_GLFW // if using glfw
#ifdef ALMOND_USING_VULKAN // if using glfw and vulkan
	#define GLFW_INCLUDE_VULKAN // This is not an Almond Macro, This is a GLFW Macro Required to use GLFW with Vulkan SDK
#endif
	#define GLFW_EXPOSE_NATIVE_WIN32  // Make sure to define this before including glfw3native.h
	#include <GLFW/glfw3.h>
	#include <GLFW/glfw3native.h>  // Add this to access platform-specific functions like glfwGetWin32Window
	#define ALMOND_USING_GLM// if using glm 
#endif

#ifdef ALMOND_USING_GLM
	#include <glm/glm.hpp>
#endif

#ifdef ALMOND_USING_VULKAN
#include <vulkan/vulkan.h>
	#ifdef _WIN32
	#include <vulkan/vulkan_win32.h> // for winmain
	#endif
#endif

#ifdef ALMOND_USING_SDL
	#define SDL_MAIN_HANDLED
	#include <SDL3/SDL.h>
#endif

// --------------------
// Compile-Time Guards
// --------------------
#if defined(ALMOND_USING_OPENGL) + defined(ALMOND_USING_VULKAN) + defined(ALMOND_USING_DIRECTX) + defined(ALMOND_USING_RAYLIB)  + defined(ALMOND_USING_SFML) != 1
#error "Define exactly one rendering backend (e.g., ALMOND_USING_OPENGL)."
#endif
// --------------------
#if defined(ALMOND_USING_SDL) + defined(ALMOND_USING_GLFW) + defined(ALMOND_USING_VOLK) + defined(ALMOND_USING_RAYLIB) + defined(ALMOND_USING_SFML) != 1
#error "Define exactly one windowing backend (e.g., ALMOND_USING_SDL)."
#endif
