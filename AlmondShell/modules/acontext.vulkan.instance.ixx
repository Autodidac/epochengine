module;


#include <GLFW/glfw3.h>

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
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        if (!glfwExtensions) {
            throw std::runtime_error("glfwGetRequiredInstanceExtensions returned null!");
        }
        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
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
        // Create a raw surface from GLFW
        VkSurfaceKHR rawSurface;
        if (glfwCreateWindowSurface(instance.get(), window, nullptr, &rawSurface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
        // Wrap it in a UniqueSurfaceKHR
        surface = vk::UniqueSurfaceKHR(rawSurface, instance.get());
    }
} // namespace VulkanCube
