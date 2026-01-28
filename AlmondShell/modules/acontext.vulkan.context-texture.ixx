module;

#include <include/acontext.vulkan.hpp>
#include <src/stb/stb_image.h>

export module acontext.vulkan.context:texture;

import <cstdint>;
import <cstring>;
import <stdexcept>;
import <utility>;

import :shared_vk;

namespace almondnamespace::vulkancontext
{
    void Application::createTextureImage()
    {
        int texWidth = 0, texHeight = 0, texChannels = 0;

        stbi_set_flip_vertically_on_load(true);
        stbi_uc* pixels = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels)
            throw std::runtime_error("Failed to load texture image!");

        const vk::DeviceSize imageSize =
            static_cast<vk::DeviceSize>(texWidth) *
            static_cast<vk::DeviceSize>(texHeight) * 4u;

        vk::UniqueBuffer stagingBuffer;
        vk::UniqueDeviceMemory stagingBufferMemory;

        std::tie(stagingBuffer, stagingBufferMemory) = createBuffer(
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        auto [mapRes, data] = device->mapMemory(*stagingBufferMemory, 0, imageSize);
        if (mapRes != vk::Result::eSuccess)
            throw std::runtime_error("Failed to map staging buffer memory.");
        std::memcpy(data, pixels, static_cast<std::size_t>(imageSize));
        device->unmapMemory(*stagingBufferMemory);

        stbi_image_free(pixels);

        vk::ImageCreateInfo imageInfo{};
        imageInfo.flags = {};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = vk::Format::eR8G8B8A8Srgb;
        imageInfo.extent = vk::Extent3D{
            static_cast<std::uint32_t>(texWidth),
            static_cast<std::uint32_t>(texHeight),
            1u
        };
        imageInfo.mipLevels = 1u;
        imageInfo.arrayLayers = 1u;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.queueFamilyIndexCount = 0u;
        imageInfo.pQueueFamilyIndices = nullptr;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;

        auto [imgRes, img] = device->createImageUnique(imageInfo);
        if (imgRes != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create texture image!");
        textureImage = std::move(img);

        const vk::MemoryRequirements memReq = device->getImageMemoryRequirements(*textureImage);
        const std::uint32_t memoryTypeIndex =
            findMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
        if (memoryTypeIndex == UINT32_MAX)
            throw std::runtime_error("Failed to find suitable memory type for texture image.");

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;

        auto [memRes, mem] = device->allocateMemoryUnique(allocInfo);
        if (memRes != vk::Result::eSuccess)
            throw std::runtime_error("Failed to allocate texture image memory!");
        textureImageMemory = std::move(mem);

        (void)device->bindImageMemory(*textureImage, *textureImageMemory, 0);

        transitionImageLayout(*textureImage, vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        copyBufferToImage(*stagingBuffer, *textureImage,
            static_cast<std::uint32_t>(texWidth),
            static_cast<std::uint32_t>(texHeight));

        transitionImageLayout(*textureImage, vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    void Application::transitionImageLayout(vk::Image image, vk::Format /*format*/,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
    {
        vk::UniqueCommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vk::PipelineStageFlags sourceStage{};
        vk::PipelineStageFlags destinationStage{};

        if (oldLayout == vk::ImageLayout::eUndefined &&
            newLayout == vk::ImageLayout::eTransferDstOptimal)
        {
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        }
        else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
            newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        }
        else
        {
            throw std::runtime_error("Unsupported layout transition!");
        }

        commandBuffer->pipelineBarrier(
            sourceStage,
            destinationStage,
            {},
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void Application::copyBufferToImage(vk::Buffer buffer, vk::Image image,
        std::uint32_t width, std::uint32_t height)
    {
        vk::UniqueCommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{ 0, 0, 0 };
        region.imageExtent = vk::Extent3D{ width, height, 1u };

        commandBuffer->copyBufferToImage(
            buffer,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &region
        );

        endSingleTimeCommands(commandBuffer);
    }

    void Application::createTextureImageView()
    {
        textureImageView = createImageViewUnique(
            *textureImage,
            vk::Format::eR8G8B8A8Srgb,
            vk::ImageAspectFlagBits::eColor
        );
    }

    void Application::createTextureSampler()
    {
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.flags = {};
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        auto [r, samp] = device->createSamplerUnique(samplerInfo);
        if (r != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create texture sampler!");

        textureSampler = std::move(samp);
    }
} // namespace almondnamespace::vulkancontext
