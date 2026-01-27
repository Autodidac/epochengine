module;

#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <include/acontext.vulkan.hpp>

export module acontext.vulkan.context:buffers;

import :shared_context;
import :meshcube;

namespace almondnamespace::vulkancontext
{
    void Application::createVertexBuffer()
    {
        const auto vertices = cube_vertices();
        if (vertices.empty())
            throw std::runtime_error("[Vulkan] No vertex data available.");

        const vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        auto [stagingBuffer, stagingBufferMemory] = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        void* mapped = nullptr;
        (void)device->mapMemory(*stagingBufferMemory, 0, bufferSize, {}, &mapped);
        std::memcpy(mapped, vertices.data(), static_cast<std::size_t>(bufferSize));
        device->unmapMemory(*stagingBufferMemory);

        auto [vb, vbMem] = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        copyBuffer(*stagingBuffer, *vb, bufferSize);

        stagingBuffer.reset();
        stagingBufferMemory.reset();

        vertexBuffer = std::move(vb);
        vertexBufferMemory = std::move(vbMem);
    }

    void Application::createIndexBuffer()
    {
        const auto indices = cube_indices();
        if (indices.empty())
            throw std::runtime_error("[Vulkan] No index data available.");

        const vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        auto [stagingBuffer, stagingBufferMemory] = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        void* mapped = nullptr;
        (void)device->mapMemory(*stagingBufferMemory, 0, bufferSize, {}, &mapped);
        std::memcpy(mapped, indices.data(), static_cast<std::size_t>(bufferSize));
        device->unmapMemory(*stagingBufferMemory);

        auto [ib, ibMem] = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        copyBuffer(*stagingBuffer, *ib, bufferSize);

        stagingBuffer.reset();
        stagingBufferMemory.reset();

        indexBuffer = std::move(ib);
        indexBufferMemory = std::move(ibMem);
        indexCount = static_cast<std::uint32_t>(indices.size());
    }
}
