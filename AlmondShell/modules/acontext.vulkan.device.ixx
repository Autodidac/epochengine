module;



export module acontext.vulkan.device;

import acontext.vulkan.context;
import acontext.vulkan.swapchain;

namespace VulkanCube {

    inline vk::PhysicalDevice Application::pickPhysicalDevice() {
        auto devices = instance->enumeratePhysicalDevices().value;  // Returns a vector directly

        if (devices.empty()) {  // No `.result` or `.value` needed
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        for (const auto& dev : devices) {  // No `.value`
            if (isDeviceSuitable(dev)) {
                return dev;
            }
        }

        throw std::runtime_error("Failed to find a suitable GPU!");
    }


    inline bool Application::checkDeviceExtensionSupport(vk::PhysicalDevice device) {
        auto availableExts = device.enumerateDeviceExtensionProperties();
        if (availableExts.result != vk::Result::eSuccess) return false;
        std::set<std::string> requiredExts(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& ext : availableExts.value) {
            requiredExts.erase(ext.extensionName);
        }
        return requiredExts.empty();
    }

    inline QueueFamilyIndices Application::findQueueFamilies(vk::PhysicalDevice device) {
        QueueFamilyIndices indices;
        auto queueProps = device.getQueueFamilyProperties();
        uint32_t i = 0;
        for (const auto& queueFamily : queueProps) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }
            // Check presentation support
            VkBool32 presentSupport = false;
            (void)device.getSurfaceSupportKHR(i, *surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
            if (indices.isComplete()) break;
            i++;
        }
        return indices;
    }

    inline bool Application::isDeviceSuitable(vk::PhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapDetails = Application::querySwapChainSupport(device);
            swapChainAdequate = !swapDetails.formats.empty() && !swapDetails.presentModes.empty();
        }
        // (Optional) check for specific features like samplerAnisotropy if needed
        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    inline void Application::createLogicalDevice() {
        physicalDevice = pickPhysicalDevice();
        queueFamilyIndices = findQueueFamilies(physicalDevice);
        // Prepare queue creation info for each unique queue family (graphics, present)
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueFamilies = {
            queueFamilyIndices.graphicsFamily.value(),
            queueFamilyIndices.presentFamily.value()
        };
        float queuePriority = 1.0f;
        for (uint32_t family : uniqueFamilies) {
            queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo({}, family, 1, &queuePriority));
        }
        vk::PhysicalDeviceFeatures deviceFeatures{};
        // If anisotropy needed:
        // if (physicalDevice.getFeatures().samplerAnisotropy) deviceFeatures.samplerAnisotropy = VK_TRUE;
        vk::DeviceCreateInfo createInfo(
            {},
            static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
            enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
            enableValidationLayers ? validationLayers.data() : nullptr,
            static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(),
            &deviceFeatures
        );
        auto result = physicalDevice.createDeviceUnique(createInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create logical device!");
        }
        device = std::move(result.value);
        // Load device-level function pointers
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device.get());
        // Retrieve queue handles
        graphicsQueue = device->getQueue(queueFamilyIndices.graphicsFamily.value(), 0);
        presentQueue = device->getQueue(queueFamilyIndices.presentFamily.value(), 0);
    }

    inline void Application::createCommandPool() {
        if (!queueFamilyIndices.graphicsFamily.has_value()) {
            throw std::runtime_error("Graphics family index not set!");
        }
        vk::CommandPoolCreateInfo poolInfo({}, queueFamilyIndices.graphicsFamily.value());
        auto result = device->createCommandPoolUnique(poolInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create command pool!");
        }
        commandPool = std::move(result.value);
    }

} // namespace VulkanCube
