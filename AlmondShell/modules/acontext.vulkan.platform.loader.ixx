module;

// VulkanLoader.hpp - Cross-platform Vulkan Loader
#pragma once
#include <vulkan/vulkan.h>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

export module acontext.vulkan.paltform.loader;

namespace epoch::vulkan {

    constexpr auto LoadLibrary() noexcept -> void* {
#ifdef _WIN32
        return LoadLibraryA("vulkan-1.dll");
#else
        return dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
#endif
    }

    constexpr auto FreeLibrary(void* lib) noexcept -> void {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(lib));
#else
        dlclose(lib);
#endif
    }

    constexpr auto LoadFunction(void* lib, const char* name) noexcept -> void* {
#ifdef _WIN32
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(lib), name));
#else
        return dlsym(lib, name);
#endif
    }

}
