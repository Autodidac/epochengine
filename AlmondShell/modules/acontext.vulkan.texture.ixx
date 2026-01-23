module;



export module acontext.vulkan.texture;

import acontext.vulkan.context;

namespace VulkanCube {

    inline void Application::createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* pixels = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!");
        }
        vk::DeviceSize imageSize = texWidth * texHeight * 4;
        // Create staging buffer and copy pixel data
        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;
        std::tie(stagingBuffer, stagingBufferMemory) = createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );
        auto mapResult = device->mapMemory(*stagingBufferMemory, 0, imageSize);
        if (mapResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to map memory!");
        }
        void* data = mapResult.value;

        memcpy(data, pixels, static_cast<size_t>(imageSize));
        device->unmapMemory(*stagingBufferMemory);
        stbi_image_free(pixels);
        // Create optimal tiled image
        vk::ImageCreateInfo imageInfo(
            {}, vk::ImageType::e2D, vk::Format::eR8G8B8A8Srgb,
            vk::Extent3D(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1),
            1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::SharingMode::eExclusive
        );
        auto imageResult = device->createImageUnique(imageInfo);
        if (imageResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create texture image!");
        }
        textureImage = std::move(imageResult.value);
        vk::MemoryRequirements memReq = device->getImageMemoryRequirements(*textureImage);
        uint32_t memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        if (memoryTypeIndex == UINT32_MAX) {
            throw std::runtime_error("Failed to find suitable memory type for texture image!");
        }
        vk::MemoryAllocateInfo allocInfo(memReq.size, memoryTypeIndex);
        auto memResult = device->allocateMemoryUnique(allocInfo);
        if (memResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to allocate texture image memory!");
        }
        textureImageMemory = std::move(memResult.value);
        (void)device->bindImageMemory(*textureImage, *textureImageMemory, 0);
        // Transition image layout, copy staging buffer data, then transition to shader use
        transitionImageLayout(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(*stagingBuffer, *textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        // Free staging resources
        stagingBufferMemory.reset();
        stagingBuffer.reset();
    }

    inline void Application::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
        vk::UniqueCommandBuffer commandBuffer = beginSingleTimeCommands();
        vk::ImageMemoryBarrier barrier(
            {}, {}, oldLayout, newLayout,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            image, vk::ImageSubresourceRange(
                (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor),
                0, 1, 0, 1
            )
        );
        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        }
        // Set access masks for layout transitions
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        }
        else {
            throw std::runtime_error("Unsupported layout transition!");
        }
        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;
        if (newLayout == vk::ImageLayout::eTransferDstOptimal) {
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else {
            sourceStage = vk::PipelineStageFlagBits::eAllCommands;
            destinationStage = vk::PipelineStageFlagBits::eAllCommands;
        }
        commandBuffer->pipelineBarrier(sourceStage, destinationStage, {}, 0, nullptr, 0, nullptr, 1, &barrier);
        endSingleTimeCommands(commandBuffer);
    }

    inline void Application::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
        vk::UniqueCommandBuffer commandBuffer = beginSingleTimeCommands();
        vk::BufferImageCopy region(
            0, 0, 0,
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
            vk::Offset3D{ 0, 0, 0 }, vk::Extent3D(width, height, 1)
        );
        commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
        endSingleTimeCommands(commandBuffer);
    }

    inline void Application::createTextureImageView() {
        textureImageView = createImageViewUnique(*textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    }

    inline void Application::createTextureSampler() {
        vk::SamplerCreateInfo samplerInfo(
            {}, vk::Filter::eLinear, vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
            0.0f, VK_FALSE, 1.0f, VK_FALSE, vk::CompareOp::eAlways,
            0.0f, 0.0f, vk::BorderColor::eIntOpaqueBlack, VK_FALSE
        );
        auto result = device->createSamplerUnique(samplerInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
        textureSampler = std::move(result.value);
    }

} // namespace VulkanCube
