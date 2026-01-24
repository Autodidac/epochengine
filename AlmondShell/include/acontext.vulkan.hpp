#pragma once
#include <include/aengine.config.hpp>
// ---- Vulkan-Hpp configuration must be identical everywhere ----
#ifdef ALMOND_USING_VULKAN
#	ifdef _WIN32

#if defined(_WIN32) && !defined(VK_USE_PLATFORM_WIN32_KHR)
#  define VK_USE_PLATFORM_WIN32_KHR
#endif


// If anyone enabled dynamic dispatch, kill the build immediately.
#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC) && (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC != 0)
#  error "Do not enable VULKAN_HPP_DISPATCH_LOADER_DYNAMIC; use static dispatch across all module units."
#endif

// Pin it off (some headers check for definition, not just value).
#ifndef VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
#  define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 0
#endif

#ifdef VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#  undef VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

#ifndef VULKAN_HPP_NO_EXCEPTIONS
#  define VULKAN_HPP_NO_EXCEPTIONS
#endif

// Stop Vulkan-Hpp from generating old-style ctors
//#ifndef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
//#   define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS 1
//#endif

#include <vulkan/vulkan.hpp>

#	endif // _WIN32
#endif // ALMOND_USING_VULKAN