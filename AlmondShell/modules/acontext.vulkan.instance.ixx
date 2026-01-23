module;

#if defined(ALMOND_VULKAN_STANDALONE)
#include <GLFW/glfw3.h>
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

export module acontext.vulkan.instance;

import acontext.vulkan.context;

namespace VulkanCube {

    inline bool Application::checkValidationLayerSupport() {
        auto layerProps = vk::enumerateInstanceLayerProperties();
        if (layerProps.result != vk::Result::eSuccess) {
            return false;
        }
        for (const char* layerName : validationLayers) {
            bool found = std::any_of(
                layerProps.value.begin(), layerProps.value.end(),
                [&](const vk::LayerProperties& prop) {
                    return strcmp(prop.layerName, layerName) == 0;
                }
            );
            if (!found) return false;
        }
        return true;
    }

    inline std::vector<const char*> Application::getRequiredExtensions() {
#if defined(ALMOND_VULKAN_STANDALONE)
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions) {
            throw std::runtime_error("glfwGetRequiredInstanceExtensions returned null!");
        }
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#else
        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME
        };
#if defined(_WIN32)
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
        extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(__APPLE__)
        extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif
#endif
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }


    inline void Application::createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("Validation layers requested, but not available!");
        }
        vk::ApplicationInfo appInfo(
            "VulkanCubeApp",        // app name
            1,                      // app version
            "No Engine",            // engine name
            1,                      // engine version
            VK_API_VERSION_1_0      // request Vulkan 1.0
        );

        auto extensions = getRequiredExtensions();
        vk::InstanceCreateInfo createInfo(
            {},
            &appInfo,
            enableValidationLayers
            ? static_cast<uint32_t>(validationLayers.size())
            : 0,
            enableValidationLayers
            ? validationLayers.data()
            : nullptr,
            static_cast<uint32_t>(extensions.size()),
            extensions.data()
        );

        // Create the instance
        auto result = vk::createInstanceUnique(createInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create Vulkan instance!");
        }
        instance = std::move(result.value);

        // Initialize default dispatcher for instance-level calls
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance.get());

        // Set up the debug messenger (if validation layers are enabled)
        //setupDebugMessenger();
    }

    inline void Application::createSurface() {
#if defined(ALMOND_VULKAN_STANDALONE)
        VkSurfaceKHR rawSurface;
        if (glfwCreateWindowSurface(instance.get(), window, nullptr, &rawSurface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        surface = vk::UniqueSurfaceKHR(rawSurface, instance.get());
#elif defined(_WIN32)
        if (!nativeWindowHandle) {
            throw std::runtime_error("No native window handle available for Vulkan surface.");
        }
        vk::Win32SurfaceCreateInfoKHR createInfo(
            {},
            ::GetModuleHandleW(nullptr),
            static_cast<HWND>(nativeWindowHandle)
        );
        auto result = instance->createWin32SurfaceKHRUnique(createInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create Win32 Vulkan surface!");
        }
        surface = std::move(result.value);
#else
        throw std::runtime_error("Vulkan surface creation is not implemented for this platform.");
#endif
    }
} // namespace VulkanCube
