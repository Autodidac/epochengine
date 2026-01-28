module;


#include <include/acontext.vulkan.hpp>

// Include Vulkan-Hpp after config.
#include <vulkan/vulkan.hpp>

#include <src/stb/stb_image.h>

module acontext.vulkan.context:texture;

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
} // namespace almondnamespace::vulkancontext
