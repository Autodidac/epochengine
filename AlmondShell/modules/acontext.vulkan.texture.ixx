module;

#include <include/aengine.config.hpp>

export module acontext.vulkan.texture;

import <atomic>;
import <cstdint>;
import <iostream>;
import <mutex>;
import <span>;
import <stdexcept>;
import <string>;
import <unordered_map>;
import <vector>;

import aatlas.manager;
import aatlas.texture;
import aengine.context.type;
import aimage.loader;
import atexture;
import aspritehandle;
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

#if defined(ALMOND_USING_VULKAN)

export namespace almondnamespace::vulkantextures
{
    using Handle = std::uint32_t;

    struct AtlasGPU
    {
        std::uint64_t version = static_cast<std::uint64_t>(-1);
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    struct TextureAtlasPtrHash
    {
        size_t operator()(const TextureAtlas* atlas) const noexcept
        {
            return std::hash<const TextureAtlas*>{}(atlas);
        }
    };

    struct TextureAtlasPtrEqual
    {
        bool operator()(const TextureAtlas* lhs, const TextureAtlas* rhs) const noexcept
        {
            return lhs == rhs;
        }
    };

    inline std::unordered_map<const TextureAtlas*, AtlasGPU, TextureAtlasPtrHash, TextureAtlasPtrEqual>
        vulkan_gpu_atlases{};

    inline std::atomic_uint8_t s_generation{ 1 };

    [[nodiscard]]
    inline Handle make_handle(int atlasIdx, int localIdx) noexcept
    {
        return (Handle(s_generation.load(std::memory_order_relaxed)) << 24)
            | ((Handle(atlasIdx) & 0xFFFu) << 12)
            | (Handle(localIdx) & 0xFFFu);
    }

    [[nodiscard]]
    inline ImageData ensure_rgba(const ImageData& img)
    {
        const size_t pixelCount = static_cast<size_t>(img.width) * img.height;
        const size_t channels = img.pixels.size() / pixelCount;

        if (channels == 4) return img;

        if (channels != 3)
            throw std::runtime_error("vulkantextures::ensure_rgba(): Unsupported channel count: " + std::to_string(channels));

        std::vector<std::uint8_t> rgba(pixelCount * 4);
        const std::uint8_t* src = img.pixels.data();
        std::uint8_t* dst = rgba.data();

        for (size_t i = 0; i < pixelCount; ++i) {
            dst[4 * i + 0] = src[3 * i + 0];
            dst[4 * i + 1] = src[3 * i + 1];
            dst[4 * i + 2] = src[3 * i + 2];
            dst[4 * i + 3] = 255;
        }

        return { std::move(rgba), img.width, img.height, 4 };
    }

    export inline void ensure_uploaded(const TextureAtlas& atlas)
    {
        auto it = vulkan_gpu_atlases.find(&atlas);
        if (it != vulkan_gpu_atlases.end())
        {
            if (it->second.version == atlas.version)
                return;
        }

        AtlasGPU gpu{};
        gpu.version = atlas.version;
        gpu.width = atlas.width;
        gpu.height = atlas.height;

        vulkan_gpu_atlases[&atlas] = gpu;
    }

    export inline void clear_gpu_atlases() noexcept
    {
        vulkan_gpu_atlases.clear();
        s_generation.fetch_add(1, std::memory_order_relaxed);
    }

    export inline Handle load_atlas(const TextureAtlas& atlas, int atlasIndex = -1)
    {
        atlasmanager::ensure_uploaded(atlas);
        const int resolvedIndex = (atlasIndex >= 0) ? atlasIndex : atlas.get_index();
        return make_handle(resolvedIndex, 0);
    }

    export inline Handle atlas_add_texture(TextureAtlas& atlas, const std::string& id, const ImageData& img)
    {
        auto rgba = ensure_rgba(img);

        Texture texture{
            .width = static_cast<std::uint32_t>(rgba.width),
            .height = static_cast<std::uint32_t>(rgba.height),
            .pixels = std::move(rgba.pixels)
        };

        auto addedOpt = atlas.add_entry(id, texture);
        if (!addedOpt)
            throw std::runtime_error("vulkantextures::atlas_add_texture: Failed to add texture: " + id);

        atlasmanager::ensure_uploaded(atlas);
        return make_handle(atlas.get_index(), addedOpt->index);
    }

    export inline std::uint32_t add_texture(TextureAtlas& atlas, std::string name, const ImageData& img)
    {
        return static_cast<std::uint32_t>(atlas_add_texture(atlas, name, img));
    }

    export inline std::uint32_t add_atlas(const TextureAtlas& atlas)
    {
        atlasmanager::ensure_uploaded(atlas);
        atlasmanager::process_pending_uploads(core::ContextType::Vulkan);
        const int idx = atlas.get_index();
        return static_cast<std::uint32_t>(idx >= 0 ? idx + 1 : 1);
    }

    inline bool is_handle_live(const SpriteHandle& handle) noexcept
    {
        return handle.is_valid();
    }

    inline void warn_draw_unimplemented() noexcept
    {
        static std::once_flag warned;
        std::call_once(warned, []()
            {
                std::cerr << "[Vulkan] draw_sprite hook is active but no Vulkan sprite pipeline is wired yet.\n";
            });
    }

    export inline void draw_sprite(
        SpriteHandle handle,
        std::span<const TextureAtlas* const> atlases,
        float /*x*/, float /*y*/, float /*width*/, float /*height*/) noexcept
    {
        if (!is_handle_live(handle))
            return;

        const int atlasIdx = int(handle.atlasIndex);
        const int localIdx = int(handle.localIndex);

        if (atlasIdx < 0 || atlasIdx >= int(atlases.size()))
            return;

        const TextureAtlas* atlas = atlases[atlasIdx];
        if (!atlas)
            return;

        AtlasRegion region{};
        if (!atlas->try_get_entry_info(localIdx, region))
            return;

        ensure_uploaded(*atlas);
        warn_draw_unimplemented();
    }
}

#endif
