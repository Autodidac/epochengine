module;



export module acontext.vulkan.swapchain;

import acontext.vulkan.context;

namespace VulkanCube {

    // Convert the old free function into an Application:: method so that 'surface'
    // is recognized as 'this->surface'
    inline SwapChainSupportDetails Application::querySwapChainSupport(vk::PhysicalDevice device)
    {
        // Query surface capabilities
        auto capabilitiesResult = device.getSurfaceCapabilitiesKHR(*surface);
        if (capabilitiesResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to get surface capabilities!");
        }
        vk::SurfaceCapabilitiesKHR capabilities = capabilitiesResult.value;

        // Query surface formats
        auto formatsResult = device.getSurfaceFormatsKHR(*surface);
        if (formatsResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to get surface formats!");
        }
        std::vector<vk::SurfaceFormatKHR> formats = formatsResult.value;

        // Query present modes
        auto presentModesResult = device.getSurfacePresentModesKHR(*surface);
        if (presentModesResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to get surface present modes!");
        }
        std::vector<vk::PresentModeKHR> presentModes = presentModesResult.value;

        return { capabilities, formats, presentModes };
    }

    // Example of choosing a swap surface format
    inline vk::SurfaceFormatKHR Application::chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        for (auto& fmt : availableFormats) {
            if (fmt.format == vk::Format::eB8G8R8A8Srgb &&
                fmt.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return fmt;
            }
        }
        return availableFormats[0]; // fallback
    }

    inline vk::PresentModeKHR Application::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& modes) {
        for (const auto& mode : modes) {
            if (mode == vk::PresentModeKHR::eMailbox) return mode;
        }
        return vk::PresentModeKHR::eFifo;
    }

    inline vk::Extent2D Application::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else {
            int width = get_framebuffer_width();
            int height = get_framebuffer_height();
            return vk::Extent2D{
                std::clamp(static_cast<uint32_t>(width),
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width),
                std::clamp(static_cast<uint32_t>(height),
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height)
            };
        }
    }

    // Finally, the createSwapChain method also belongs to the Application class
    inline void Application::createSwapChain()
    {
        // Query the device swapchain support via the method we just declared
        SwapChainSupportDetails details = querySwapChainSupport(physicalDevice);

        vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
        vk::PresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
        vk::Extent2D extent = chooseSwapExtent(details.capabilities);

        // number of images
        uint32_t imageCount = details.capabilities.minImageCount + 1;
        if (details.capabilities.maxImageCount > 0 &&
            imageCount > details.capabilities.maxImageCount) {
            imageCount = details.capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo(
            {},
            *surface,                     // use our vk::UniqueSurfaceKHR
            imageCount,
            surfaceFormat.format,
            surfaceFormat.colorSpace,
            extent,
            1, // imageArrayLayers
            vk::ImageUsageFlagBits::eColorAttachment
        );

        uint32_t familyIndices[] = {
            queueFamilyIndices.graphicsFamily.value(),
            queueFamilyIndices.presentFamily.value()
        };
        if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = familyIndices;
        }
        else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        }

        createInfo.preTransform = details.capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        // create the swapchain (unique)
        auto scResult = device->createSwapchainKHRUnique(createInfo);
        if (scResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create swap chain!");
        }
        swapChain = std::move(scResult.value);

        // get the images
        auto imagesResult = device->getSwapchainImagesKHR(*swapChain);
        if (imagesResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to get swap chain images!");
        }
        swapChainImages = imagesResult.value;

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    inline vk::UniqueImageView Application::createImageViewUnique(
        vk::Image image,
        vk::Format format,
        vk::ImageAspectFlags aspectFlags)
    {
        // typical usage for color: vk::ImageAspectFlagBits::eColor
        // for depth: vk::ImageAspectFlagBits::eDepth
        vk::ImageViewCreateInfo viewInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            format,
            {},
            vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1)
        );

        auto ivResult = device->createImageViewUnique(viewInfo);
        if (ivResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create image view!");
        }
        return std::move(ivResult.value);
    }

    inline void Application::createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); ++i) {
            swapChainImageViews[i] = createImageViewUnique(
                swapChainImages[i],
                swapChainImageFormat,
                vk::ImageAspectFlagBits::eColor
            );
        }
    }

} // namespace VulkanCube
