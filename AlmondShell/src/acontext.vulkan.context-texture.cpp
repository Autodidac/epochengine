module;


#include <include/acontext.vulkan.hpp>

// Include Vulkan-Hpp after config.
#include <vulkan/vulkan.hpp>

module acontext.vulkan.context:texture;

import <array>;
import <cstdint>;
import <cstring>;
import <filesystem>;
import <sstream>;
import <stdexcept>;
import <utility>;

import aimage.loader;
import :shared_vk;

namespace almondnamespace::vulkancontext
{
    namespace
    {
        std::filesystem::path resolve_texture_path()
        {
            namespace fs = std::filesystem;
            const fs::path target = "texture.ppm";
            const std::array<fs::path, 6> candidates = {
                target,
                fs::path("AlmondShell") / target,
                fs::path("..") / "AlmondShell" / target,
                fs::path("assets") / "vulkan" / target,
                fs::path("AlmondShell") / "assets" / "vulkan" / target,
                fs::path("..") / "AlmondShell" / "assets" / "vulkan" / target,
            };

            for (const auto& path : candidates)
            {
                fs::path absolute = fs::absolute(path).lexically_normal();
                if (fs::exists(absolute) && fs::is_regular_file(absolute))
                    return absolute;
            }

            std::ostringstream message;
            message << "Failed to load texture image! Tried paths:";
            for (const auto& path : candidates)
                message << "\n  - " << fs::absolute(path).lexically_normal().string();
            throw std::runtime_error(message.str());
        }
    }

    void Application::createTextureImage()
    {
        const std::filesystem::path texturePath = resolve_texture_path();
        ImageData texture{};
        try
        {
            texture = a_loadImage(texturePath, true);
        }
        catch (const std::exception& ex)
        {
            std::ostringstream message;
            message << "Failed to load texture image at '" << texturePath.string() << "': " << ex.what();
            throw std::runtime_error(message.str());
        }

        const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texture.pixels.size());

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
        std::memcpy(data, texture.pixels.data(), static_cast<std::size_t>(imageSize));
        device->unmapMemory(*stagingBufferMemory);

        vk::ImageCreateInfo imageInfo{};
        imageInfo.flags = {};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = vk::Format::eR8G8B8A8Srgb;
        imageInfo.extent = vk::Extent3D{
            static_cast<std::uint32_t>(texture.width),
            static_cast<std::uint32_t>(texture.height),
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
            static_cast<std::uint32_t>(texture.width),
            static_cast<std::uint32_t>(texture.height));

        transitionImageLayout(*textureImage, vk::Format::eR8G8B8A8Srgb,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    }
} // namespace almondnamespace::vulkancontext
