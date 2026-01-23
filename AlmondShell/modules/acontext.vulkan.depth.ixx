module;



export module acontext.vulkan.depth;

import acontext.vulkan.context;

namespace VulkanCube {

    inline vk::Format Application::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
        for (vk::Format format : candidates) {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);
            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("Failed to find supported format!");
    }

    inline vk::Format Application::findDepthFormat() {
        return findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    }

    inline void Application::createDepthResources() {
        vk::Format depthFormat = findDepthFormat();
        vk::ImageCreateInfo imageInfo(
            {}, vk::ImageType::e2D, depthFormat,
            vk::Extent3D(swapChainExtent.width, swapChainExtent.height, 1),
            1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::SharingMode::eExclusive
        );
        auto imageResult = device->createImageUnique(imageInfo);
        if (imageResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create depth image!");
        }
        depthImage = std::move(imageResult.value);
        vk::MemoryRequirements memReq = device->getImageMemoryRequirements(*depthImage);
        uint32_t memTypeIndex = findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        if (memTypeIndex == UINT32_MAX) {
            throw std::runtime_error("Failed to find suitable memory type for depth image!");
        }
        vk::MemoryAllocateInfo allocInfo(memReq.size, memTypeIndex);
        auto memResult = device->allocateMemoryUnique(allocInfo);
        if (memResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to allocate depth image memory!");
        }
        depthImageMemory = std::move(memResult.value);
        (void)device->bindImageMemory(*depthImage, *depthImageMemory, 0);
        depthImageView = createImageViewUnique(*depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
    }

    inline void Application::createFramebuffers() {
        framebuffers.resize(swapChainImageViews.size());
        for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
            std::array<vk::ImageView, 2> attachments = { *swapChainImageViews[i], *depthImageView };
            vk::FramebufferCreateInfo framebufferInfo(
                {}, *renderPass,
                static_cast<uint32_t>(attachments.size()), attachments.data(),
                swapChainExtent.width, swapChainExtent.height, 1
            );
            auto fbResult = device->createFramebufferUnique(framebufferInfo);
            if (fbResult.result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
            framebuffers[i] = std::move(fbResult.value);
        }
    }

} // namespace VulkanCube
