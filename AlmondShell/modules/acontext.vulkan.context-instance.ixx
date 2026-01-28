module;

#if defined(ALMOND_VULKAN_STANDALONE)
#   include <GLFW/glfw3.h>
#endif

#if defined(_WIN32)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <include/aframework.hpp>
#endif

#include <include/acontext.vulkan.hpp>

export module acontext.vulkan.context:instance;

import :shared_context;
import :shared_vk;

import <algorithm>;
import <cstring>;
import <stdexcept>;
import <vector>;

export namespace almondnamespace::vulkancontext
{
    bool Application::checkValidationLayerSupport()
    {
        auto [res, props] = vk::enumerateInstanceLayerProperties();
        if (res != vk::Result::eSuccess)
            return false;

        for (const char* layerName : validationLayers)
        {
            const bool found = std::any_of(props.begin(), props.end(),
                [&](const vk::LayerProperties& p)
                {
                    return std::strcmp(p.layerName, layerName) == 0;
                });

            if (!found)
                return false;
        }
        return true;
    }

    std::vector<const char*> Application::getRequiredExtensions()
    {
        std::vector<const char*> extensions;

#if defined(ALMOND_VULKAN_STANDALONE)
        std::uint32_t glfwCount = 0;
        const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwCount);
        if (!glfwExt)
            throw std::runtime_error("glfwGetRequiredInstanceExtensions returned null.");
        extensions.assign(glfwExt, glfwExt + glfwCount);
#else
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#   if defined(_WIN32)
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#   endif
#endif

        if (enableValidationLayers)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    void Application::createInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
            throw std::runtime_error("Validation layers requested, but not available.");

        vk::ApplicationInfo appInfo{};
        appInfo.pApplicationName = "AlmondEngine Vulkan";
        appInfo.applicationVersion = 1;
        appInfo.pEngineName = "AlmondEngine";
        appInfo.engineVersion = 1;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        const auto extensions = getRequiredExtensions();

        vk::InstanceCreateInfo createInfo{};
        createInfo.flags = {};
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<std::uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = nullptr;
        }

        auto [r, inst] = vk::createInstanceUnique(createInfo);
        if (r != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create Vulkan instance.");

        instance = std::move(inst);

        // You are using DispatchLoaderStatic (confirmed by your log). No init() exists.
        // Do NOT call any dispatcher init here.
    }

    void Application::createSurface()
    {
#if defined(ALMOND_VULKAN_STANDALONE)
        VkSurfaceKHR rawSurface{};
        if (glfwCreateWindowSurface(instance.get(), window, nullptr, &rawSurface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create GLFW window surface.");

        surface = vk::UniqueSurfaceKHR(
            vk::SurfaceKHR(rawSurface),
            vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>(
                instance.get(), nullptr, VULKAN_HPP_DEFAULT_DISPATCHER)
        );
#elif defined(_WIN32)
        if (!nativeWindowHandle)
            throw std::runtime_error("No native window handle available for Vulkan surface.");

        vk::Win32SurfaceCreateInfoKHR sci{};
        sci.flags = {};
        sci.hinstance = ::GetModuleHandleW(nullptr);
        sci.hwnd = static_cast<HWND>(nativeWindowHandle);

        auto [sr, s] = instance->createWin32SurfaceKHRUnique(sci);
        if (sr != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create Win32 Vulkan surface.");

        surface = std::move(s);
#else
        throw std::runtime_error("Vulkan surface creation not implemented for this platform.");
#endif
    }

} // namespace almondnamespace::vulkancontext
